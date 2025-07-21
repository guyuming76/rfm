use reedline::{ DefaultPrompt, DefaultPromptSegment, EditCommand, Emacs, Keybindings, KeyCode, KeyModifiers,Reedline, Signal, ValidationResult, Validator, ReedlineEvent, FileBackedHistory, default_emacs_keybindings, Completer ,Suggestion, Span, ReedlineMenu, ColumnarMenu, MenuBuilder, CwdAwareHinter };
use nu_ansi_term::{Color, Style};
use libc::c_char;
use std::ffi::{CStr, CString};
use std::ptr;
use std::path::PathBuf;
use dirs::home_dir;
use log::{error, warn, debug}; // 导入日志宏

use nu_cli::completions::{FileCompletion, Completer, CompletionOptions};
use nu_protocol::{
    engine::{EngineState, Stack, StateWorkingSet},
    Span as NuShellSpan,
};
use std::sync::Arc;
use nu_engine::command_prelude::Value;
use nu_protocol::CompletionSort;

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

// 定义组合补全器
pub struct CombinedCompleter {
    c_completer: CCompleter,
    engine_state: Arc<EngineState>,
    stack: Arc<Stack>,
}

impl CombinedCompleter {
    pub fn new(c_completer: CCompleter) -> Self {
        // 创建默认的 Nushell 上下文
        let mut engine_state = nu_cmd_lang::create_default_context();
        engine_state = nu_command::add_shell_command_context(engine_state);

        // 添加 $nu 常量
        engine_state.generate_nu_constant();

        // 创建栈并设置当前工作目录
        let mut stack = Stack::new();
        let cwd = std::env::current_dir().unwrap_or_default();
        stack.add_env_var(
            "PWD".to_string(),
            Value::string(cwd.to_string_lossy(), NuShellSpan::unknown()),
        );

        // 合并环境变量到引擎状态
        let _ = engine_state.merge_env(&mut stack);

        Self {
            c_completer,
            engine_state: Arc::new(engine_state),
            stack: Arc::new(stack),
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

        // 2. 如果 C 补全为空，调用文件补全
            // 创建 StateWorkingSet
            let working_set = StateWorkingSet::new(&self.engine_state);

            // 创建补全选项
            let options = CompletionOptions {
                case_sensitive: false,
                match_algorithm: nu_cli::completions::MatchAlgorithm::Prefix,
                sort: CompletionSort::Alphabetical,
            };

            // 提取当前输入的前缀（简化处理）
            let prefix = if pos > 0 && pos <= line.len() {
                &line[..pos]
            } else {
                line
            };
            // 调用文件补全
            let mut file_completer = FileCompletion;
            let suggestions = file_completer.fetch(
                &working_set,
                &self.stack,
                prefix,
                NuShellSpan::unknown(),
                0,
                &options,
            );

            // 转换为 reedline 的 Suggestion 格式
            suggestions
                .into_iter()
                .map(|s| s.suggestion)
                .collect()
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
					.with_completer(Box::new(CombinedCompleter::new(get_completion_fn))) // 使用组合补全器
					.with_menu(ReedlineMenu::EngineCompleter(completion_menu))
					.with_history(Box::new(history))
        				.with_validator(Box::new(AlwaysIncompleteValidator)) // 使用自定义验证器
        				.with_edit_mode(Box::new(Emacs::new(keybindings)))  // 使用自定义键绑定
					.with_quick_completions(true)  // 启用快速补全,只有一个补全选项时，系统会自动选择该选项而不显示菜单
					.with_hinter(Box::new(CwdAwareHinter::default().with_style(Style::new().italic().fg(Color::LightGray)).with_min_chars(2))),
        Err(e) => {
            warn!("History load error: {}", e);
            Reedline::create() // Fallback to default history
					.with_completer(Box::new(CombinedCompleter::new(get_completion_fn))) // 使用组合补全器
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
