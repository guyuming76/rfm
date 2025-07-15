use reedline::{ DefaultPrompt, DefaultPromptSegment, EditCommand, Emacs, Keybindings, KeyCode, KeyModifiers,Reedline, Signal, ValidationResult, Validator, ReedlineEvent, FileBackedHistory, default_emacs_keybindings };
use libc::c_char;
use std::ffi::{CStr, CString};
use std::ptr;

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
}

#[no_mangle]
pub extern "C" fn reedline_create() -> *mut ReedlineHandle {
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

    let history_result = FileBackedHistory::with_file(1000, "~/.rfm_reedline_history.txt".into());

    // 创建 Reedline 引擎并配置
    let engine = match history_result {
        Ok(history) => Reedline::create().use_bracketed_paste(true)
					.with_history(Box::new(history))
        				.with_validator(Box::new(AlwaysIncompleteValidator)) // 使用自定义验证器
        				.with_edit_mode(Box::new(Emacs::new(keybindings))),  // 使用自定义键绑定
        Err(e) => {
            eprintln!("History load error: {}. Using in-memory history.", e);
            Reedline::create() // Fallback to default history
        }
    };

    Box::into_raw(Box::new(ReedlineHandle { inner: engine }))
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
       unsafe { Box::from_raw(handle) };
    }
}
