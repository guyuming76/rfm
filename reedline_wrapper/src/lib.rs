use reedline::{Reedline, Signal, DefaultPrompt,DefaultPromptSegment};
use libc::c_char;
use std::ffi::{CStr, CString};
use std::ptr;

// 定义不透明结构体（C中通过指针访问）
pub struct ReedlineHandle {
    inner: Reedline,
}

#[no_mangle]
pub extern "C" fn reedline_create() -> *mut ReedlineHandle {
    let engine = Reedline::create();
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
