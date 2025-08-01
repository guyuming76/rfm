use reedline::{ DefaultPrompt, DefaultPromptSegment, EditCommand, Emacs, Keybindings, KeyCode, KeyModifiers,Reedline, Signal, ValidationResult, Validator, ReedlineEvent, FileBackedHistory, default_emacs_keybindings, Completer ,Suggestion, Span, ReedlineMenu, ColumnarMenu, MenuBuilder, CwdAwareHinter, DefaultCompleter, Result, ReedlineError, LineBuffer, UndoBehavior };
use nu_ansi_term::{Color, Style};
use libc::c_char;
use std::ffi::{CStr, CString};
use std::ptr;
use std::path::PathBuf;
use dirs::home_dir;
use log::{error, warn, debug}; // 导入日志宏
use std::io::Write;

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
type GetCompletionFn = extern "C" fn(*const i8, *mut i32) -> *mut *mut i8;
//type GNUFileCompleteWrapperFn = extern "C" fn(*const i8) -> *mut *mut i8;

// c 语言tab补全器，补全rfm当前选中文件
pub struct CCompleter {
    get_completion_fn: GetCompletionFn,
}

impl CCompleter {
    pub fn new(get_completion_fn: GetCompletionFn) -> Self {
        Self { get_completion_fn }
    }
}

impl Completer for CCompleter {
    fn complete(&mut self, line: &str, pos: usize) -> Vec<Suggestion> {
        let mut count: i32 = 0;
        let before_cursor = &line[..pos];
        let last_space_pos = before_cursor.rfind(' ').map(|p| p + 1).unwrap_or(0);
        
        unsafe {
            let mut start_pos: usize = 0;
            let context = context_for_cursor(line, pos, &mut start_pos);
            let result_ptr = (self.get_completion_fn)(context.as_ptr(), &mut count);

            if result_ptr.is_null() {
                return vec![];
            }

            let mut suggestions = Vec::new();
            let mut i = 0;
            
            // 遍历所有补全建议直到遇到空指针
            while !result_ptr.wrapping_add(i).is_null() && !(*result_ptr.wrapping_add(i)).is_null() {
                let c_str = CStr::from_ptr(*result_ptr.wrapping_add(i));
                if let Ok(result) = c_str.to_str() {
                    if !result.is_empty() {
                        suggestions.push(Suggestion {
                            value: result.to_string(),
                            description: None,
                            style: None,
                            extra: None,
                            span: Span::new(start_pos, pos),
                            append_whitespace: false,
                        });
                    }
                }
                i += 1;
            }

            // 释放C端分配的内存
            libc::free(result_ptr as *mut libc::c_void);

            suggestions
        }
    }
}

// 定义组合补全器
pub struct CombinedCompleter {
    c_completer: CCompleter,
    reedline_DefaultCompleter: DefaultCompleter
}

impl CombinedCompleter {
    pub fn new(c_completer: CCompleter) -> Self {
        Self {
            c_completer,
	    reedline_DefaultCompleter: DefaultCompleter::default(),
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
	    return self.reedline_DefaultCompleter.complete(line,pos);
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
pub extern "C" fn reedline_create(get_completion_fn: GetCompletionFn) -> *mut ReedlineHandle {
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

    // 修改历史文件创建部分
    let history_path = home_dir()
        .map(|mut path| {
            path.push(".rfm_reedline_history.txt");
            path
        })
        .unwrap_or_else(|| "rfm_reedline_history.txt".into());

    let history_result = FileBackedHistory::with_file(1000, history_path.clone());

    // 配置菜单（即使您不想显示菜单，也需要配置）
    let completion_menu = Box::new(ColumnarMenu::default().with_name("completion_menu"));

    // 配置 Tab 键绑定
    keybindings.add_binding(
	KeyModifiers::NONE,
    	KeyCode::Tab,
	ReedlineEvent::UntilFound(vec![
		ReedlineEvent::Menu("completion_menu".to_string()),
        	ReedlineEvent::MenuNext,
    		]),
    	);


    // 创建 Reedline 引擎并配置
    let engine = match history_result {
        Ok(history) => Reedline::create().use_bracketed_paste(true)
					.with_completer(Box::new(CombinedCompleter::new(CCompleter::new(get_completion_fn)))) // 使用组合补全器
					.with_menu(ReedlineMenu::EngineCompleter(completion_menu))
					.with_history(Box::new(history))
        				.with_validator(Box::new(AlwaysIncompleteValidator)) // 使用自定义验证器
        				.with_edit_mode(Box::new(Emacs::new(keybindings)))  // 使用自定义键绑定
					.with_quick_completions(true)  // 启用快速补全,只有一个补全选项时，系统会自动选择该选项而不显示菜单
					.with_hinter(Box::new(CwdAwareHinter::default().with_style(Style::new().italic().fg(Color::LightGray)).with_min_chars(2))),
        Err(e) => {
            warn!("History load error: {}", e);
            Reedline::create().with_completer(Box::new(CombinedCompleter::new(CCompleter::new(get_completion_fn)))) // 使用组合补全器
					.with_menu(ReedlineMenu::EngineCompleter(completion_menu))
        				.with_validator(Box::new(AlwaysIncompleteValidator)) // 使用自定义验证器
        				.with_edit_mode(Box::new(Emacs::new(keybindings)))  // 使用自定义键绑定
					.with_quick_completions(true)  // 启用快速补全,只有一个补全选项时，系统会自动选择该选项而不显示菜单
        }
    };

    Box::into_raw(Box::new(ReedlineHandle {
        inner: engine,
        history_path, // 存储路径到句柄
    }))
}

#[no_mangle]
pub extern "C" fn reedline_readline(
    handle: *mut ReedlineHandle,
    prompt: *const c_char,
) -> *mut c_char {
   assert!(!handle.is_null());
   let handle = unsafe { &mut *handle };
   let prompt_str = unsafe { CStr::from_ptr(prompt).to_string_lossy() };

   // 创建符合 Prompt trait 的 DefaultPrompt 对象
   let left_prompt = DefaultPromptSegment::Basic(prompt_str.to_string());
   let prompt_obj = DefaultPrompt::new(left_prompt, DefaultPromptSegment::Empty);

   match handle.inner.read_line(&prompt_obj) {
        Ok(Signal::Success(line)) => CString::new(line).unwrap().into_raw(),
        Ok(Signal::CtrlD | Signal::CtrlC) => ptr::null_mut(), // 处理Ctrl-C/Ctrl-D
        Err(e) => {
            warn!("Reedline error: {}", e);
            ptr::null_mut()
        }
    }
}

#[no_mangle]
pub extern "C" fn string_free(s: *mut c_char) {
    if !s.is_null() {
       unsafe { let _ = std::ffi::CString::from_raw(s); };
    }
}

#[no_mangle]
pub extern "C" fn reedline_free(handle: *mut ReedlineHandle) {
    if !handle.is_null() {
       unsafe { Box::from_raw(handle) };
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
}

impl RfmEditor {
    fn new(get_completion_fn: GetCompletionFn) -> Self {
        let mut keybindings = default_emacs_keybindings();
        // 配置菜单（即使您不想显示菜单，也需要配置）
        let completion_menu = Box::new(ColumnarMenu::default().with_name("completion_menu"));

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
        let edit_mode = Box::new(Emacs::new(keybindings));
        let line_editor = Reedline::create()
            .with_validator(Box::new(AlwaysIncompleteValidator)) // 缺少这行，在编辑多行文本时就没法中中间插入行，新行会被加到最后
            .with_edit_mode(edit_mode)
            .use_bracketed_paste(true)
            .with_menu(ReedlineMenu::EngineCompleter(completion_menu))
	    .with_completer(Box::new(CombinedCompleter::new(CCompleter::new(get_completion_fn)))) // 使用组合补全器;
            .with_quick_completions(true);  // 启用快速补全,只有一个补全选项时，系统会自动选择该选项而不显示菜单
        Self {
            line_editor,
            current_file: None,
            modified: false,
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
        let mut line_buffer = LineBuffer::from(content.as_str());
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
	
	// 5. 编辑循环
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
			_ => {
                            // 处理其他编辑命令  
                            self.modified = true;
			}
                    }
		}
		Signal::CtrlC | Signal::CtrlD => {
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
pub extern "C" fn rfm_editor_new(get_completion_fn: GetCompletionFn) -> *mut RfmEditor {
    Box::into_raw(Box::new(RfmEditor::new(get_completion_fn)))
}

#[no_mangle]
pub extern "C" fn rfm_editor_edit_file(
    editor: *mut RfmEditor,
    filename: *const libc::c_char,
    error: *mut *mut libc::c_char,
) -> libc::c_int {
    let editor = unsafe { &mut *editor };
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

#[no_mangle]
pub extern "C" fn rfm_editor_free(editor: *mut RfmEditor) {
    if !editor.is_null() {
        unsafe { Box::from_raw(editor) };
    }
}
