use reedline::{Completer, Reedline, Signal, Suggestion, Span, MenuBuilder, ColumnarMenu, ReedlineMenu, DefaultPrompt, DefaultPromptSegment, EditCommand, KeyCode, KeyModifiers, ReedlineEvent,default_emacs_keybindings,Emacs, FileBackedHistory,Validator,ValidationResult};
use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::env;
use std::fs::OpenOptions;
use std::io::Write;

// 定义外部函数来调用 GNU Readline 的补全功能
#[link(name = "readline")]
unsafe extern "C" {
    fn rl_completion_matches(text: *const c_char, entry_func: unsafe extern "C" fn(*const c_char, i32) -> *mut c_char) -> *mut *mut c_char;
    fn rl_filename_completion_function(text: *const c_char, state: i32) -> *mut c_char;
    fn free(ptr: *mut c_char);
}

struct ReadlineCompleter;

impl Completer for ReadlineCompleter {
    fn complete(&mut self, line: &str, _pos: usize) -> Vec<Suggestion> {
        let mut suggestions = Vec::new();
        let context = CString::new(line).unwrap();

        unsafe {
            let result_ptr = rl_completion_matches(context.as_ptr(), rl_filename_completion_function);

            if !result_ptr.is_null() {
                let mut i = 0;
                while !result_ptr.wrapping_add(i).is_null() && !(*result_ptr.wrapping_add(i)).is_null() {
                    let c_str = CStr::from_ptr(*result_ptr.wrapping_add(i));
                    if let Ok(result) = c_str.to_str() {
                        if !result.is_empty() {
                            suggestions.push(Suggestion {
                                value: result.to_string(),
                                description: None,
                                style: None,
                                extra: None,
                                span: Span::new(0, _pos),
                                append_whitespace: false,
                            });
                        }
                    }
                    free(*result_ptr.wrapping_add(i));
                    i += 1;
                }
                libc::free(result_ptr as *mut _);
            }
        }
        suggestions
    }
}

// 自定义验证器 - 始终返回未完成状态，强制按Enter提交
struct AlwaysIncompleteValidator;
impl Validator for AlwaysIncompleteValidator {
    fn validate(&self, _line: &str) -> ValidationResult {
        ValidationResult::Incomplete
    }
}

fn main() {
    // 获取命令行参数
    let args: Vec<String> = env::args().collect();
    // 检查是否包含 --multiline 参数
    let has_multiline = args.iter().any(|arg| arg == "--multiline");
    // 过滤掉 --multiline 参数，保留其他参数
    let filtered_args: Vec<String> = args.into_iter().filter(|arg| arg != "--multiline").collect();

    if filtered_args.len()<=1 {
        eprintln!("Usage: rfmReedline NamedPipe [HistoryFile] [--multiline]");
        std::process::exit(1);
    }
    let named_pipe_path = &filtered_args[1];
    
    // 创建自定义补全器实例
    let completer = ReadlineCompleter;
    // 配置菜单（即使您不想显示菜单，也需要配置）
    let completion_menu = Box::new(ColumnarMenu::default().with_name("completion_menu"));
    // 配置 Tab 键绑定
    let mut keybindings = default_emacs_keybindings();
    keybindings.add_binding(
	    KeyModifiers::NONE,
    	    KeyCode::Tab,
	    ReedlineEvent::UntilFound(vec![
		ReedlineEvent::Menu("completion_menu".to_string()),
        	ReedlineEvent::MenuNext,
    		]),
    );
    if has_multiline{
        // 添加 Alt+Enter 绑定到插入换行命令
        keybindings.add_binding(
    	    KeyModifiers::ALT,
    	    KeyCode::Enter,
    	    ReedlineEvent::Edit(vec![EditCommand::InsertNewline])
        );
    }
    
    let edit_mode = Box::new(Emacs::new(keybindings));
    // 创建 Reedline 实例
    let mut line_editor = Reedline::create().with_completer(Box::new(completer));
    if has_multiline{
        line_editor = line_editor.with_validator(Box::new(AlwaysIncompleteValidator)); // 使用自定义验证器
    }
    line_editor = line_editor
        .with_edit_mode(edit_mode)
        .with_menu(ReedlineMenu::EngineCompleter(completion_menu))
        .with_quick_completions(true);  // 启用快速补全,只有一个补全选项时，系统会自动选择该选项而不显示菜单
	//.with_hinter(Box::new(CwdAwareHinter::default().with_min_chars(2)));
    // 如果有命令行参数，则使用第一个参数作为历史记录文件
    if filtered_args.len() > 2 {
        let history_path = &filtered_args[2];
        let history_result = FileBackedHistory::with_file(1000, history_path.clone().into());
        
        line_editor = match history_result {
            Ok(history) => line_editor.with_history(Box::new(history)),
            Err(_e) => line_editor,
        };
    }

    //println!("结合 GNU Readline 和 Reedline 的示例");
    //println!("输入路径并按 Tab 键尝试补全 (输入 'exit' 退出)");

    //loop {
        //let prompt = "\x1b[1;32m»\x1b[0m ";
        let prompt = DefaultPrompt {  
            left_prompt: DefaultPromptSegment::Empty,  
            right_prompt: DefaultPromptSegment::Empty,  
        };
        let signal = line_editor.read_line(&prompt);
        
        match signal {
            Ok(Signal::Success(line)) => {
                //if line.trim() == "exit" {
                //    break;
                //}
                //println!("{}", line);
                // 打开命名管道并写入内容
                //Custom { kind: Other, error: "The cursor position could not be read within a normal duration" }
                match OpenOptions::new().write(true).open(named_pipe_path) {
                    Ok(mut pipe) => {
                        if let Err(e) = writeln!(pipe, "{}", line) {
                            eprintln!("写入命名管道失败: {}", e);
                        }
                    }
                    Err(e) => {
                        eprintln!("打开命名管道 {} 失败: {}", named_pipe_path, e);
                    }
                }
            }
            Ok(Signal::CtrlD) => {
                println!("");
            //    break;
            }
            Ok(Signal::CtrlC) => {
                println!("");
            }
            Err(err) => {
                eprintln!("详细错误信息: {:?}", err);
                // 尝试回退到简单输入模式
                eprintln!("尝试使用简单输入模式...");
                let mut line = String::new();
                if std::io::stdin().read_line(&mut line).is_ok() {
                    println!("{}", line.trim_end());
                } else {
                    eprintln!("简单输入也失败了");
                    std::process::exit(2);
                }
            }
        }
    //}
}
