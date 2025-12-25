//GPL-3 许可by guyuming

//这个程序用于rfm(通过命令结尾&)打开的新的终端模拟器窗口内补全
use reedline::{Completer, Reedline, Signal, Suggestion, Span, MenuBuilder, ColumnarMenu, ReedlineMenu, DefaultPrompt, DefaultPromptSegment, KeyCode, KeyModifiers, ReedlineEvent,default_emacs_keybindings,Emacs, FileBackedHistory};
use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::env;
use std::fs::OpenOptions;
use std::io::Write;

// 定义外部函数来调用 GNU Readline 的补全功能
#[link(name = "readline")]
extern "C" {
    fn rl_completion_matches(text: *const c_char, entry_func: unsafe extern "C" fn(*const c_char, i32) -> *mut c_char) -> *mut *mut c_char;
    fn rl_filename_completion_function(text: *const c_char, state: i32) -> *mut c_char;
    fn free(ptr: *mut c_char);
}

// extern "C" {
//     fn DimensionCompletion(text: *const c_char, start: i32, end: i32) -> *mut *mut c_char;
// }

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
                                ..Default::default()
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

//struct DimensionCompleter;

fn main() {
    // 获取命令行参数
    let args: Vec<String> = env::args().collect();

    if args.len()<=1 {
        eprintln!("Usage: rfmReedline NamedPipe [HistoryFile]");
        std::process::exit(1);
    }
    let named_pipe_path = &args[1];
    
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
    let edit_mode = Box::new(Emacs::new(keybindings));
    // 创建 Reedline 实例
    let mut line_editor = Reedline::create().with_completer(Box::new(completer))
        .with_edit_mode(edit_mode)
        .with_menu(ReedlineMenu::EngineCompleter(completion_menu))
        .with_quick_completions(true);  // 启用快速补全,只有一个补全选项时，系统会自动选择该选项而不显示菜单
	//.with_hinter(Box::new(CwdAwareHinter::default().with_min_chars(2)));
    // 如果有命令行参数，则使用第一个参数作为历史记录文件
    if args.len() > 2 {
        let history_path = &args[2];
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
            Ok(Signal::Interrupted) => {
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



// ## `rl_completion_matches` 函数的作用

// `rl_completion_matches` 是一个核心的补全匹配生成函数，它接受一个生成器函数作为参数来产生补全匹配列表。 [1](#1-0) 

// 该函数的签名为：
// ```c
// char **rl_completion_matches (const char *text, rl_compentry_func_t *entry_function)
// ```

// ## 与 `rl_completion_matches` 配合的补全函数签名

// 与 `rl_completion_matches` 配合使用的自定义补全函数类型是 `rl_compentry_func_t`，其签名为： [2](#1-1) 

// ```c
// typedef char *rl_compentry_func_t (const char *, int);
// ```

// 具体来说，这种函数接受两个参数：
// - `text`: 要补全的部分文本
// - `state`: 状态参数，首次调用时为0，后续调用为非零值

// 函数返回一个 `char *`，表示一个匹配项，当没有更多匹配时返回 `NULL`。 [3](#1-2) 

// ## 两种补全函数类型的区别

// ### `rl_compentry_func_t` (生成器函数)
// - **签名**: `char *function(const char *text, int state)`
// - **用途**: 逐个生成补全匹配项
// - **调用方式**: 被 `rl_completion_matches` 反复调用直到返回 `NULL`
// - **返回值**: 单个匹配字符串

// ### `rl_completion_func_t` (高级补全函数)  
// - **签名**: `char **function(const char *text, int start, int end)` [4](#1-3) 
// - **用途**: 直接返回完整的补全匹配数组
// - **调用方式**: 被补全系统直接调用一次
// - **返回值**: 完整的匹配字符串数组

// rl_completion_matches函数使用state参数的迭代器模式是为了支持增量式匹配生成，这种设计有几个重要优势
//     内存效率: 不需要一次性生成所有可能的匹配项
//     响应性: 可以在生成过程中响应信号中断
//     灵活性: 支持动态数据源（如目录遍历、数据库查询等）
