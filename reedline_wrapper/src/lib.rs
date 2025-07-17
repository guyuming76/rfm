use reedline::{ DefaultPrompt, DefaultPromptSegment, EditCommand, Emacs, Keybindings, KeyCode, KeyModifiers,Reedline, Signal, ValidationResult, Validator, ReedlineEvent, FileBackedHistory, default_emacs_keybindings, Completer ,Suggestion, Span, ReedlineMenu, ColumnarMenu, MenuBuilder, CwdAwareHinter };
use nu_ansi_term::{Color, Style};
use libc::c_char;
use std::ffi::{CStr, CString};
use std::ptr;
use std::path::PathBuf;
use dirs::home_dir;

// 全局变量存储当前ReedlineHandle实例
//static mut CURRENT_HANDLE: *mut ReedlineHandle = std::ptr::null_mut();

fn empty_context_for_cursor(line: &str, pos: usize) -> bool {
    if pos > line.len() {
        return false;
    }

    let before_cursor = &line[..pos];
    let after_cursor = &line[pos..];

    // 检查光标前最后一个字符
    let prev_is_space = before_cursor.chars().last() == Some(' ') || before_cursor.is_empty();

    // 检查光标后第一个字符
    let next_is_space = after_cursor.chars().next() == Some(' ') || after_cursor.is_empty();

    prev_is_space && next_is_space
}

// 定义 C 函数指针类型  
type GetCompletionFn = extern "C" fn(*const i8, *mut i32) -> *mut *mut i8;

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
        //let c_input = match CString::new(line) {
        //    Ok(s) => s,
        //    Err(_) => return vec![],
        //};

        if !empty_context_for_cursor(line, pos) { return vec![] }
	let mut count: i32 = 0;
        unsafe {
            // 创建 C 兼容的空字符串
            let empty_cstr = CString::new("").expect("CString::new failed");
	    
            // 调用传入的函数指针
            let result_ptr = (self.get_completion_fn)(empty_cstr.as_ptr(), &mut count);

            if result_ptr.is_null() || result_ptr.wrapping_add(0).is_null() {
                return vec![];
            }

            let c_str = CStr::from_ptr(*(result_ptr.wrapping_add(0)));
            let result = match c_str.to_str() {
                Ok(s) => s.to_string(),
                Err(_) => return vec![],
            };

            if result.is_empty() {
                return vec![];
            }

            vec![Suggestion {
                value: result,
                description: None,
                style: None,
                extra: None,
                span: Span::new(pos, pos),
                append_whitespace: false,
            }]
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
pub extern "C" fn reedline_create(get_completion_fn: GetCompletionFn,
    				cwd: *const c_char
) -> *mut ReedlineHandle {

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

    // 释放现有的handle（如果存在）
    //unsafe {
    //    if !CURRENT_HANDLE.is_null() {
    //        reedline_free(CURRENT_HANDLE);
    //    }
    //}

    // Convert C string to Rust Option<String>
    //The conversion handles:
    //- `NULL` pointers → `None`
    //- Valid C strings → `Some(String)`
    //- Invalid UTF-8 → `None` (via `to_str().ok()`)
    //Note: The unsafe block is necessary for converting raw pointers, but we check for null first to ensure safety.
    let rust_cwd = if cwd.is_null() {
        None
    } else {
        unsafe { CStr::from_ptr(cwd).to_str().ok().map(|s| s.to_string()) }
    };

    // 创建 Reedline 引擎并配置
    let engine = match history_result {
        Ok(history) => Reedline::create().use_bracketed_paste(true)
					.with_completer(Box::new(CCompleter::new(get_completion_fn)))
					.with_menu(ReedlineMenu::EngineCompleter(completion_menu))
					.with_history(Box::new(history))
        				.with_validator(Box::new(AlwaysIncompleteValidator)) // 使用自定义验证器
        				.with_edit_mode(Box::new(Emacs::new(keybindings)))  // 使用自定义键绑定
					.with_quick_completions(true)  // 启用快速补全,只有一个补全选项时，系统会自动选择该选项而不显示菜单
					.with_hinter(Box::new(CwdAwareHinter::default().with_style(Style::new().italic().fg(Color::LightGray)).with_min_chars(2)))
                			.with_cwd(rust_cwd),
        Err(e) => {
            eprintln!("History load error: {}.", e);
            Reedline::create() // Fallback to default history
					.with_completer(Box::new(CCompleter::new(get_completion_fn)))
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
    // 更新全局变量
    //unsafe {
    //    CURRENT_HANDLE = handle;
    //}
    //handle
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
            eprintln!("Reedline error: {}", e);
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
        unsafe {
            // 如果释放的是全局handle，则更新全局状态
            //if handle == CURRENT_HANDLE {
            //    CURRENT_HANDLE = std::ptr::null_mut();
            //}
            Box::from_raw(handle);
        }
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
            println!("History saved successfully to {:?}", handle.history_path);
        }
        Err(e) => {
            eprintln!("Failed to save history to {:?}: {}", handle.history_path, e);
        }
    }
}
