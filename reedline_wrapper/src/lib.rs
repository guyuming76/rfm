//GPL-3许可 by guyuming

//这个程序做为库被rfm执行文件加载，接受rfm传入的补全函数指针，并调用

use reedline::{ DefaultPrompt, DefaultPromptSegment, EditCommand, Emacs, KeyCode, KeyModifiers,Reedline, Signal, ValidationResult, Validator, ReedlineEvent, FileBackedHistory, default_emacs_keybindings, Completer ,Suggestion, Span, ReedlineMenu, ColumnarMenu, MenuBuilder, CwdAwareHinter, DefaultCompleter, Result, LineBuffer, UndoBehavior };
use nu_ansi_term::{Color, Style};
use libc::c_char;
use std::ffi::{CStr, CString};
use std::ptr;
use std::path::{PathBuf, Path};
use std::os::unix::fs::PermissionsExt;
use dirs::home_dir;
use log::{warn, debug}; // 导入日志宏
use glib_sys::{GPtrArray};
use std::io::{stdout, Write};
use crossterm::execute;
use crossterm::terminal::supports_keyboard_enhancement;
use crossterm::event::{PushKeyboardEnhancementFlags, KeyboardEnhancementFlags, PopKeyboardEnhancementFlags};
// deepwiki 回答说 line 只包含光标前的字符串，光标及其后面字符都不包含，输入abcd,然后光标向左一道c下，则line只包含ab. 也就是说代码里检查cursor_and_after后面的部分貌似目前都没有用，但还是留着。
fn context_for_cursor(line: &str, pos: usize, completion_start_pos: &mut usize) -> CString {
    // 创建 C 兼容的空字符串
    let empty_cstr = CString::new("").expect("CString::new failed");

    // 找到最后一个换行符的位置（如果有的话）
    let last_newline = line[..pos].rfind('\n').map(|n| n + 1).unwrap_or(0);
    let current_line = &line[last_newline..];
    let current_line_pos = pos - last_newline;

    if current_line_pos > current_line.len() {
        return empty_cstr;
    }
    //debug!("current_line:{} ,current_line_pos:{}", current_line, current_line_pos);
    // 将字符串分割为光标前后的两部分（仅处理当前行）
    let before_cursor = &current_line[..current_line_pos];  // 光标前的子串
    //debug!("before_cursor:{}",before_cursor);
    let char_before_cursor = before_cursor.chars().last();
    //debug!("char_before_cursor:{:?}",char_before_cursor);
    // 检查光标前最后一个字符. 如果是空格 或 光标前无字符（行首） -> true
    let char_before_cursor_is_space_or_none = char_before_cursor == Some(' ') || before_cursor.is_empty();
    //debug!("char_before_cursor_is_space_or_none:{}",char_before_cursor_is_space_or_none);

    let cursor_and_after = &line[pos..];   // 光标后的子串（含光标位置字符
    //debug!("cursor_and_after:{}",cursor_and_after);
    let char_at_cursor = cursor_and_after.chars().nth(0);
    //debug!("char_at_cursor:{:?}",char_at_cursor);
    // 光标处必须是空格
    let char_at_cursor_is_space = char_at_cursor == Some(' ') || cursor_and_after.is_empty();
    //debug!("char_at_cursor_is_space:{}",char_at_cursor_is_space);

    let char_after_cursor = cursor_and_after.chars().next();
    //debug!("char_after_cursor:{:?}",char_after_cursor);
    // 检查光标后第一个字符. 如果是空格 或 光标后无字符（行尾） -> true
    let char_after_cursor_is_space_or_none = char_after_cursor == Some(' ') || cursor_and_after.is_empty();
    //debug!("char_after_cursor_is_space_or_none:{}",char_after_cursor_is_space_or_none);

    if char_before_cursor_is_space_or_none && char_at_cursor_is_space && char_after_cursor_is_space_or_none {
        *completion_start_pos = pos;
	return empty_cstr;
    } else {
        // TODO:LineBuffer.current_word_range() 可以用吗？
	// 从后往前找第一个空格的位置
        let last_space_pos = before_cursor.rfind(' ').map(|p| p + 1).unwrap_or(0);
        // 计算完整字符串中的补全起始位置 = 当前行起始位置 + 行内最后一个空格的位置
        *completion_start_pos = last_newline + last_space_pos;
        // 截取从最后一个空格到光标位置的子串，不包含空格
        let word = &before_cursor[last_space_pos..];
        CString::new(word).unwrap_or_else(|_| CString::new("").expect("CString failed"))
    }
}

// 定义 C 函数指针类型  
type GetCompletionFn = extern "C" fn(*const i8, *mut i32) -> *const GPtrArray;
type EscapeHandler = extern "C" fn();
type ToggleRecursiveCompletionFn = extern "C" fn();
//type GNUFileCompleteWrapperFn = extern "C" fn(*const i8) -> *mut *mut i8;
// 添加 repr(C) 确保内存布局与C兼容
#[repr(C)]
pub struct rfmFunctions {
    pub get_completion_fn:GetCompletionFn,
    pub get_menu_completion_fn:GetCompletionFn,
    // 其他函数...
    pub g_idle_add_once_handle_escape:EscapeHandler,
    pub toggle_recursive_completion_fn:ToggleRecursiveCompletionFn,
}

// c 语言tab补全器，补全rfm当前选中文件
pub struct CCompleter {
    get_completion_fn: GetCompletionFn,
    use_relative_path: Option<String>,
}

impl CCompleter {
    pub fn new(rfm_functions: &rfmFunctions, use_relative_path: Option<String>) -> Self {
        Self {
            get_completion_fn: (*rfm_functions).get_completion_fn,
            use_relative_path
        }
    }
}

// c 语言菜单补全器，补全菜单和工具栏命令
pub struct MenuCompleter {
    get_menu_completion_fn: GetCompletionFn,
}

impl MenuCompleter {
    pub fn new(rfm_functions: &rfmFunctions) -> Self {
        Self {
            get_menu_completion_fn: (*rfm_functions).get_menu_completion_fn,
        }
    }
}

// Shift+Tab 补全器，用于临时切换递归补全模式
pub struct ShiftTabCompleter {
    c_completer: CCompleter,
    toggle_recursive_completion_fn: ToggleRecursiveCompletionFn,
}

impl ShiftTabCompleter {
    pub fn new(rfm_functions: &rfmFunctions) -> Self {
        Self {
            c_completer: CCompleter::new(rfm_functions, None),
            toggle_recursive_completion_fn: (*rfm_functions).toggle_recursive_completion_fn,
        }
    }
}

impl Completer for ShiftTabCompleter {
    fn complete(&mut self, line: &str, pos: usize) -> Vec<Suggestion> {
        // Toggle recursive completion mode
        (self.toggle_recursive_completion_fn)();
        
        // Call the C completer
        let suggestions = self.c_completer.complete(line, pos);
        
        // Toggle back to restore original mode
        (self.toggle_recursive_completion_fn)();
        
        suggestions
    }
}

impl Completer for CCompleter {
    fn complete(&mut self, line: &str, pos: usize) -> Vec<Suggestion> {
        let mut count: i32 = 0;
        let before_cursor = &line[..pos];
        let _last_space_pos = before_cursor.rfind(' ').map(|p| p + 1).unwrap_or(0);

        unsafe {
            let mut start_pos: usize = 0;
            let context = context_for_cursor(line, pos, &mut start_pos);
            let result_ptr = (self.get_completion_fn)(context.as_ptr(), &mut count);

            if result_ptr.is_null() {
                return vec![];
            }

            let result_array = &*result_ptr;
            let len = result_array.len as usize;
            let mut suggestions = Vec::new();

            // 遍历所有补全建议
            for i in 0..len {
                let item_ptr = *result_array.pdata.add(i) as *const c_char;
                if item_ptr.is_null() { continue; }

                let c_str = CStr::from_ptr(item_ptr);
                if let Ok(result) = c_str.to_str() {
                    if !result.is_empty() {
                        if result.starts_with('|') {//在c语言的rfm_editor_completion_based_on_search_result函数，非路径返回结果以|开头
                            suggestions.push(Suggestion {
                                value: result.chars().skip(1).collect(), //去掉开头的|字符
                                description: None,
                                style: Some(Style::new().fg(Color::Red)),
                                extra: None,
                                span: Span::new(pos, pos),
                                append_whitespace: false,
                                ..Default::default()
                            });
                        }else if result.starts_with('}') { //这个是维度名
                            suggestions.push(Suggestion {
                                value: result.chars().enumerate().map(|(index, c)| if index == 0 { ' ' } else { c }).collect(), //开头'}' 换成空格
                                description: None,
                                style: Some(Style::new().fg(Color::Yellow)),
                                extra: None,
                                span: Span::new(pos, pos),
                                append_whitespace: false,
                                ..Default::default()
                            });
                        }else if result.starts_with('=') { //这个是维度值
                            suggestions.push(Suggestion {
                                value: result.to_string(),
                                description: None,
                                style: Some(Style::new().fg(Color::Yellow)),
                                extra: None,
                                span: Span::new(pos, pos),
                                append_whitespace: false,
                                ..Default::default()
                            });
                        }else{
                            //对于文件路径补全，context包含的是文件名前缀，补全的返回内容result是完整的路径，所以start_pos 等于0; 而非路径补全，目前用于rfm_editor_completion_based_on_search_result, 补全的返回内容不包含context,suggestion 应该是 context 合并上 result
                            let final_path = match &self.use_relative_path {
                                Some(file_path) => {
                                    let abs_path = Path::new(result);
                                    // 获取文件所在目录作为基准路径
                                    let base_dir = Path::new(file_path).parent()
                                        .unwrap_or_else(|| Path::new(".")); // 如果无法获取目录则使用当前目录
                                    
                                    match pathdiff::diff_paths(abs_path, base_dir) {
                                        Some(rel_path) => rel_path.to_string_lossy().into_owned(),
                                        None => result.to_string(),
                                    }
                                }
                                None => result.to_string(),
                            };
                            // Determine color based on file type (ls-like coloring)
                            let style = {
                                let path = Path::new(result);
                                if path.is_dir() {
                                    Style::new().fg(Color::Blue) // Blue for directories
                                } else if path.symlink_metadata().map_or(false, |m| m.file_type().is_symlink()) {
                                    Style::new().fg(Color::Cyan) // Cyan for symlinks
                                } else if path.metadata().map_or(false, |m| {
                                    m.permissions().mode() & 0o111 != 0
                                }) {
                                    Style::new().fg(Color::Green) // Green for executables
                                } else if let Some(ext) = path.extension() {
                                    let ext_str = ext.to_str().unwrap_or("").to_lowercase();
                                    match ext_str.as_str() {
                                        "txt" | "md" | "log" | "conf" => 
                                            Style::new().fg(Color::White), // White for text files
                                        "jpg" | "jpeg" | "png" | "gif" | "bmp" => 
                                            Style::new().fg(Color::Magenta), // Magenta for image files
                                        "pdf" => 
                                            Style::new().fg(Color::Red), // Red for PDFs
                                        "zip" | "tar" | "gz" | "bz2" | "rar" => 
                                            Style::new().fg(Color::Yellow), // Yellow for archives
                                        _ => 
                                            Style::new().fg(Color::Green) // Default color
                                    }
                                } else {
                                    Style::new().fg(Color::Green) // Default color
                                }
                            };
                            suggestions.push(Suggestion {
                                value: final_path,
                                description: None,
                                style: Some(style),
                                extra: None,
                                span: Span::new(start_pos, pos),
                                append_whitespace: false,
                                ..Default::default()
                            });
                        }
                    }
                }
            }

            suggestions
        }
    }
}

impl Completer for MenuCompleter {
    fn complete(&mut self, line: &str, pos: usize) -> Vec<Suggestion> {
        let mut count: i32 = 0;
        let before_cursor = &line[..pos];
        let _last_space_pos = before_cursor.rfind(' ').map(|p| p + 1).unwrap_or(0);

        unsafe {
            let mut start_pos: usize = 0;
            let context = context_for_cursor(line, pos, &mut start_pos);
            let result_ptr = (self.get_menu_completion_fn)(context.as_ptr(), &mut count);

            if result_ptr.is_null() {
                return vec![];
            }

            let result_array = &*result_ptr;
            let len = result_array.len as usize;
            let mut suggestions = Vec::new();

            // 遍历所有补全建议
            for i in 0..len {
                let item_ptr = *result_array.pdata.add(i) as *const c_char;
                if item_ptr.is_null() { continue; }

                let c_str = CStr::from_ptr(item_ptr);
                if let Ok(result) = c_str.to_str() {
                    if !result.is_empty() {
                        // 根据前缀决定颜色：* = 黄色 (toolbar/file menu), # = 绿色 (builtin command), 无 = 蓝色 (默认)
                        let (display_value, style) = if result.starts_with('*') {
                            (&result[1..], Style::new().fg(Color::Yellow))
                        } else if result.starts_with('#') {
                            (&result[1..], Style::new().fg(Color::Green))
                        } else {
                            (result.as_ref(), Style::new().fg(Color::Blue))
                        };
                        suggestions.push(Suggestion {
                            value: display_value.to_string(),
                            description: None,
                            style: Some(style),
                            extra: None,
                            span: Span::new(start_pos, pos),
                            append_whitespace: false,
                            ..Default::default()
                        });
                    }
                }
            }

            suggestions
        }
    }
}

// 定义组合补全器
pub struct CombinedCompleter {
    c_completer: CCompleter,
    reedline_default_completer: DefaultCompleter
}

impl CombinedCompleter {
    pub fn new(c_completer: CCompleter) -> Self {
        Self {
            c_completer,
	    reedline_default_completer: DefaultCompleter::default(),
        }
    }
}

impl Completer for CombinedCompleter {
    fn complete(&mut self, line: &str, pos: usize) -> Vec<Suggestion> {
        // 1. 先调用 C 补全器
        let c_suggestions = self.c_completer.complete(line, pos);

        if !c_suggestions.is_empty() {
            return c_suggestions;
        }else{
	    // 调用 reedline DefaultCompleter
	    return self.reedline_default_completer.complete(line,pos);
	}
    }
}

// 自定义验证器 - 始终返回未完成状态，强制按Enter提交
struct AlwaysIncompleteValidator;
impl Validator for AlwaysIncompleteValidator {
    fn validate(&self, _line: &str) -> ValidationResult {
        ValidationResult::Incomplete
    }
}

// 定义不透明结构体（C中通过指针访问）
pub struct ReedlineHandle {
    inner: Reedline,
    history_path: PathBuf,
}

#[no_mangle]
pub extern "C" fn reedline_create(rfm_functions: *const rfmFunctions) -> *mut ReedlineHandle {
    // 只需初始化一次
    static INIT: std::sync::Once = std::sync::Once::new();
    INIT.call_once(|| {
	//env_logger::builder()
    	//	.format(|buf, record| {
        //	writeln!(buf, "{}\r", record.args()) // 显式添加 \r
    	//	})
    	//	.init();
        // 如果觉得debug!输出的内容显示效果像是只包含换行符，不包含回车符，可以用上面代码定制
        env_logger::init();
    });

    // 检查指针是否为空
    if rfm_functions.is_null() {
        return ptr::null_mut();
    }

    let rfm_functions = unsafe { &*rfm_functions };
// 这段代码let f = unsafe { &*f };在Rust中用于解引用从C语言传入的原始指针并创建安全引用，具体解析如下：
//     ‌*f‌：首先解引用原始指针f（假设f是*const T或*mut T类型），这会访问指针指向的内存数据。由于Rust无法保证原始指针的安全性（可能为空或悬垂），此操作必须包裹在unsafe块中。
//     ‌&*f‌：解引用后，通过&获取数据的不可变引用（类型为&T）。此时Rust会将其视为安全引用，后续使用时受借用检查器保护。
//     ‌unsafe块‌：整个操作需在unsafe上下文中执行，因为直接解引用原始指针可能引发未定义行为（如指针无效或数据竞争）。开发者需自行确保指针有效性


    if supports_keyboard_enhancement().unwrap_or(false) {
        // 启用键盘增强协议,以便使用Menu键
        execute!(stdout(), PushKeyboardEnhancementFlags(KeyboardEnhancementFlags::DISAMBIGUATE_ESCAPE_CODES));
    }else{ warn!("crossterm KeyboardEnhancement not supported for MENU key"); }
    // 创建自定义键绑定
    let mut keybindings = default_emacs_keybindings();
    
    // 添加 Alt+Enter 绑定到插入换行命令
    keybindings.add_binding(
    	KeyModifiers::ALT,
    	KeyCode::Enter,
    	ReedlineEvent::Edit(vec![EditCommand::InsertNewline])
    );

    // 绑定Enter键到提交操作
    keybindings.add_binding(
        KeyModifiers::NONE,
        KeyCode::Enter,
        ReedlineEvent::Submit
    );

    keybindings.add_binding(KeyModifiers::NONE, KeyCode::Esc,
        ReedlineEvent::UntilFound(vec![ReedlineEvent::Esc, ReedlineEvent::ExecuteHostCommand("esc".to_string())]));

    keybindings.add_binding(KeyModifiers::ALT, KeyCode::Up, ReedlineEvent::ExecuteHostCommand("u".to_string())); //Up命令，Up是Chinese.h里定义的常量，config.h里面正则表达式替换使其可通过u使用
    keybindings.add_binding(KeyModifiers::ALT, KeyCode::Left, ReedlineEvent::ExecuteHostCommand("l".to_string())); //Previous命令
    keybindings.add_binding(KeyModifiers::ALT, KeyCode::Right, ReedlineEvent::ExecuteHostCommand("r".to_string())); //Next命令

    let TAB_menu = ReedlineMenu::WithCompleter {
        menu: Box::new(ColumnarMenu::default().with_name("tab_menu")),
        completer:Box::new(CombinedCompleter::new(CCompleter::new(rfm_functions, None))),
    };
    let MENU_menu = ReedlineMenu::WithCompleter {
        menu: Box::new(ColumnarMenu::default().with_name("menu_menu")),
        completer:Box::new(MenuCompleter::new(rfm_functions)),
    };
    let SHIFT_TAB_menu = ReedlineMenu::WithCompleter {
        menu: Box::new(ColumnarMenu::default().with_name("shift_tab_menu")),
        completer:Box::new(ShiftTabCompleter::new(rfm_functions)),
    };

    // 配置 Tab 键绑定
    keybindings.add_binding(KeyModifiers::NONE, KeyCode::Tab,
	ReedlineEvent::UntilFound(vec![
		ReedlineEvent::Menu("tab_menu".to_string()),
        ReedlineEvent::MenuNext,
    	]),
    );

    // 绑定 Menu 键到菜单补全菜单
    keybindings.add_binding(
        KeyModifiers::NONE,
        KeyCode::Menu,
        ReedlineEvent::Menu("menu_menu".to_string())
    );
    //上面的Menu键不工作，不知道是终端模拟器还是reedline使用的crossterm库的原因，SHIFT+TAB 也不行，这里就用F2做个替代
    keybindings.add_binding(
        KeyModifiers::NONE,
        KeyCode::F(2),
        ReedlineEvent::Menu("menu_menu".to_string())
    );
    // 绑定 Shift+Tab 到递归补全菜单
    keybindings.add_binding(
        KeyModifiers::SHIFT,
        KeyCode::BackTab,
        ReedlineEvent::Menu("shift_tab_menu".to_string())
    );

    let engine_builder = Reedline::create()
        .use_bracketed_paste(true) //复制粘贴处理 \r\n,需要terminal支持
        .with_menu(TAB_menu)
        .with_menu(MENU_menu)
        .with_menu(SHIFT_TAB_menu)
        .with_validator(Box::new(AlwaysIncompleteValidator))
        .with_edit_mode(Box::new(Emacs::new(keybindings)))
        .with_quick_completions(true)
        .with_hinter(Box::new(
            CwdAwareHinter::default()
                .with_style(Style::new().italic().fg(Color::LightGray))
                .with_min_chars(2)
        ));

    // 修改历史文件创建部分
    let history_path = home_dir()
        .map(|mut path| {
            path.push(".rfm_reedline_history.txt");
            path
        })
        .unwrap_or_else(|| ".rfm_reedline_history.txt".into());

    let history_result = FileBackedHistory::with_file(1000, history_path.clone());
    let engine = match history_result {
        Ok(history) => engine_builder.with_history(Box::new(history)),
        Err(e) => {
            warn!("History load error: {}", e);
            engine_builder
        }
    };

    Box::into_raw(Box::new(ReedlineHandle {
        inner: engine,
        history_path, // 存储路径到句柄
    }))
}

pub const REEDLINE_INTERRUPTED: &'static str = "reedline_interrupted";

#[no_mangle]
pub extern "C" fn reedline_readline(
    handle: *mut ReedlineHandle,
    prompt: *const c_char,
    rfm_functions: *const rfmFunctions,
) -> *mut c_char {
   assert!(!handle.is_null());
   let handle = unsafe { &mut *handle };
   let prompt_str = unsafe { CStr::from_ptr(prompt).to_string_lossy() };
   //let rfm_functions = unsafe { &*rfm_functions };
   // 创建符合 Prompt trait 的 DefaultPrompt 对象
   let left_prompt = DefaultPromptSegment::Basic(prompt_str.to_string());
   let prompt_obj = DefaultPrompt::new(left_prompt, DefaultPromptSegment::Empty);

   match handle.inner.read_line(&prompt_obj) {
        Ok(Signal::Success(line)) => CString::new(line).unwrap().into_raw(),
        Ok(Signal::CtrlD | Signal::CtrlC) => ptr::null_mut(), // 处理Ctrl-C/Ctrl-D
        Ok(Signal::Interrupted) => CString::new(REEDLINE_INTERRUPTED).unwrap().into_raw(),
        Err(e) => {
            warn!("Reedline error: {}", e);
            ptr::null_mut()
        }
    }
}

#[no_mangle]
pub extern "C" fn reedline_interrupt(handle: *mut ReedlineHandle) {
    assert!(!handle.is_null());
    let handle = unsafe { &mut *handle };
    handle.inner.interrupt();
}

#[no_mangle]
pub extern "C" fn string_free(s: *mut c_char) {
    if !s.is_null() {
       unsafe { let _ = std::ffi::CString::from_raw(s); };
    }
}

#[no_mangle]
pub extern "C" fn reedline_free(handle: *mut ReedlineHandle) {
    if supports_keyboard_enhancement().unwrap_or(false) {
        execute!(stdout(), PopKeyboardEnhancementFlags);
    }
    if !handle.is_null() {
        unsafe { let _ = Box::from_raw(handle); };
    }
}

// 保存历史记录的C接口
#[no_mangle]
pub extern "C" fn reedline_save_history(handle: *mut ReedlineHandle) {
    if handle.is_null() {
        return;
    }
    let handle = unsafe { &mut *handle };
    
    match handle.inner.sync_history() {
        Ok(_) => {
            debug!("History saved successfully to {:?}", handle.history_path);
        }
        Err(e) => {
            warn!("Failed to save history to {:?}: {}", handle.history_path, e);
        }
    }
}



struct RfmEditor {
    line_editor: Reedline,
    current_file: Option<PathBuf>,
    modified: bool,
    rfm_functions: *const rfmFunctions
}

impl RfmEditor {
    fn new(rfm_functions: *const rfmFunctions, filename: String) -> Self {
        let mut keybindings = default_emacs_keybindings();
        // 配置菜单（即使您不想显示菜单，也需要配置）
        let completion_menu = Box::new(ColumnarMenu::default().with_name("completion_menu"));
        let rfm_functions = unsafe { &*rfm_functions };
        // 配置 Tab 键绑定
        keybindings.add_binding(
	    KeyModifiers::NONE,
    	    KeyCode::Tab,
	    ReedlineEvent::UntilFound(vec![
		ReedlineEvent::Menu("completion_menu".to_string()),
        	ReedlineEvent::MenuNext,
    		]),
    	);

        // 添加自定义键绑定
        keybindings.add_binding(KeyModifiers::CONTROL, KeyCode::Char('o'),
                               ReedlineEvent::ExecuteHostCommand("save".to_string()));
        keybindings.add_binding(KeyModifiers::CONTROL, KeyCode::Char('x'),
                               ReedlineEvent::ExecuteHostCommand("exit".to_string()));
        keybindings.add_binding(KeyModifiers::NONE, KeyCode::Esc, ReedlineEvent::UntilFound(vec![ReedlineEvent::Esc, ReedlineEvent::ExecuteHostCommand("esc".to_string())]));
        let edit_mode = Box::new(Emacs::new(keybindings));
        let line_editor = Reedline::create()
            .with_validator(Box::new(AlwaysIncompleteValidator)) // 缺少这行，在编辑多行文本时就没法中中间插入行，新行会被加到最后
            .with_edit_mode(edit_mode)
            .use_bracketed_paste(true)
            .with_menu(ReedlineMenu::EngineCompleter(completion_menu))
	    .with_completer(Box::new(CombinedCompleter::new(CCompleter::new(rfm_functions, Some(filename))))) // 使用组合补全器;
            .with_quick_completions(true);  // 启用快速补全,只有一个补全选项时，系统会自动选择该选项而不显示菜单
        //let functions = unsafe { std::ptr::read_unaligned(rfm_functions) };
        Self {
            line_editor,
            current_file: None,
            modified: false,
            rfm_functions: rfm_functions,
        }
    }


    fn edit_file(&mut self, filename: &str) -> Result<bool> {
	// 1. 加载文件内容
	let file_path = PathBuf::from(filename);
	let content = if file_path.exists() {
            std::fs::read_to_string(&file_path)?
	} else {
            String::new() // 新文件
	};
        let line_buffer = LineBuffer::from(content.as_str());
        //line_buffer.move_to_start();

	// 2. 设置当前文件和修改状态
	self.current_file = Some(file_path);
	self.modified = false;
	
	// 3. 将文件内容加载到编辑器缓冲区
        self.line_editor.set_line_buffer(line_buffer, UndoBehavior::CreateUndoPoint);
	// 4. 创建编辑提示符
	let prompt = DefaultPrompt::new(
            DefaultPromptSegment::Basic("".to_string()),
            DefaultPromptSegment::Basic("".to_string()),
	);
	
	// 5. 显示用户提示信息
	println!("\x1b[33mCtrl+o save and exit; Ctrl+x exit without saving\x1b[0m");
	
	// 6. 编辑循环
	let mut saved = false;
	loop {
            match self.line_editor.read_line(&prompt)? {
		Signal::Success(command) => {
                    match command.as_str() {
			"save" => {
                            saved = self.save_file()?;
                            break;
			}
			"exit" => {
                            if self.modified {
				// 提示保存
				//println!("File modified. Save? (y/n)");
				// 处理用户选择
                            }
                            break;
			}
                        "esc" => {
                            unsafe {((*self.rfm_functions).g_idle_add_once_handle_escape)();}
                            continue;
                        }
			_ => {
                            // 处理其他编辑命令  
                            self.modified = true;
			}
                    }
		}
		Signal::CtrlC | Signal::CtrlD | Signal::Interrupted => {
                    break;
		}
            }
	}
	
	Ok(saved)
    }
    
    fn save_file(&mut self) -> Result<bool> {
	if let Some(ref file_path) = self.current_file {
            let content = self.line_editor.current_buffer_contents();
            std::fs::write(file_path, content)?;
            self.modified = false;
            //println!("File saved: {}", file_path.display());
            Ok(true)
	} else {
            Ok(false)
	}
    }
}


#[no_mangle]
pub extern "C" fn rfm_editor_edit_file(
    rfm_functions: *const rfmFunctions,
    filename: *const libc::c_char,
    error: *mut *mut libc::c_char,
) -> libc::c_int {
    let filename_str = unsafe { CStr::from_ptr(filename).to_string_lossy().into_owned() };
    let mut editor = Box::new(RfmEditor::new(rfm_functions, filename_str)); //rfm执行程序在调用rfmEditor‘内部命令‘时通过 get_completion_fn传 入static char** rfm_filename_completion(const char *text, int *count). 这里是static函数，没问题？
    let filename = unsafe { CStr::from_ptr(filename).to_string_lossy().into_owned() };

    match editor.edit_file(&filename) {
        Ok(true) => 2,  // 保存成功
        Ok(false) => 1, // 正常退出未保存
        Err(e) => {
            if !error.is_null() {
                let msg = CString::new(e.to_string()).unwrap();
                unsafe { *error = msg.into_raw() };
            }
            0
        }
    }
}
