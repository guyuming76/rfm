# Python prompt_toolkit 替代 Rust reedline — 任务列表

## 阶段 0：环境准备与依赖

### 任务 0.1：确认 prompt_toolkit 可用性
- [x] 检查系统是否已有 `prompt_toolkit` (`python3 -c "import prompt_toolkit"`)
- [x] 确认 `prompt_toolkit` 版本 ≥ 3.0（支持 `PromptSession`）

### 任务 0.2：config.mk 新增选项
- [x] 添加 `READLINE_LIB = -Dprompt_toolkit` 选项（注释掉 reedline 和 GNU_readline）
- [x] 确保 `prompt_toolkit` 选项自动检测 `PythonEmbedded`（Makefile 中强制要求）
- [x] 在 Makefile 中添加对应的条件编译分支

---

## 阶段 1：核心 Python 模块 — 基础功能

### 任务 1.1：创建 Python 模块骨架
- [x] 创建 `rfm_prompt_toolkit/` 目录
- [x] 创建 `rfm_prompt_toolkit/__init__.py` — 导出主要接口
- [x] 创建 `rfm_prompt_toolkit/core.py` — 核心 readline 循环 + Completer

### 任务 1.2：实现 readline() 函数
- [x] 使用 `PromptSession` 实现同步阻塞的 `readline(prompt_str)`
- [x] 支持多行编辑 (`multiline=True`, 通过 Alt+Enter 插入换行)
- [x] Enter 提交，Ctrl+C / Ctrl+D 处理
- [x] 返回类型与 C 侧约定一致（字符串 / NULL / "reedline_interrupted"）

### 任务 1.3：实现历史记录
- [x] 使用 `FileHistory` 保存到 `~/.rfm_prompt_toolkit_history.txt`
- [x] 实现 `save_history()` 接口（对应 `pt_save_history`）
- [x] 实现历史搜索（prompt_toolkit 内置 Ctrl+R 等）

---

## 阶段 2：C-Python 桥接层

### 任务 2.1：创建 C 头文件
- [x] 创建 `prompt_toolkit_wrapper.h`（对标 `reedline_wrapper.h`）
- [x] 定义相同的 opaque handle 类型和函数签名
- [x] 共享 `REEDLINE_INTERRUPTED` 宏和 `rfmFunctions` 结构体

### 任务 2.2：实现 C 胶水代码
- [x] 创建 `rfm_pt_wrapper.c`
- [x] `pt_create()` → 初始化 Python 状态、导入模块、创建 session
- [x] `pt_readline()` → 获取 GIL → 调用 Python `readline()` → 释放 GIL
- [x] `pt_interrupt()` → 发送 SIGINT 信号
- [x] `pt_request_repaint()` → 请求重绘提示符
- [x] `pt_set_selection_star()` → 设置选中标记
- [x] `pt_save_history()` / `pt_free()` / `string_free()`
- [x] `pt_editor_edit_file()` → rfmEdit 编辑模式

### 任务 2.3：C 补全回调传递
- [x] 使用 `PyCapsule` 将 `rfmFunctions` 函数指针传递给 Python
- [x] Python 侧用 `ctypes.pythonapi.PyCapsule_GetPointer` + `ctypes.CFUNCTYPE` 解包
- [x] 在 `RfmCompleter` 中调用 C 补全函数获取候选项
- [x] 处理四种补全源：文件名(蓝)、菜单命令(黄)、搜索结果(红)、维度(黄)
- [x] ★ GIL 安全：ctypes 调用 C 函数时自动释放 GIL

---

## 阶段 3：功能对等实现

### 任务 3.1：TAB 补全 + 交互菜单
- [x] 实现 `RfmCompleter`：先调 C 补全函数，fallback 到 `PathCompleter`
- [x] 实现 F2 菜单补全模式（`_completion_mode = "menu"`）
- [x] 实现 Shift+Tab 递归补全模式（`_completion_mode = "shift_tab"`）
- [x] 颜色编码与 reedline 一致（|红色, }黄色, =黄色, *黄色, #绿色, 默认蓝色）

### 任务 3.2：键绑定
- [x] Emacs 模式（默认）
- [x] Alt+Enter → 插入换行
- [x] Tab → 触发补全菜单
- [x] Shift+Tab → 递归补全切换
- [x] F2 → 菜单命令补全
- [x] Alt+Up → "Up" 命令 (rfm 上一级目录)
- [x] Alt+Left → "Previous" 命令
- [x] Alt+Right → "Next" 命令
- [x] Esc → 触发 C 回调 `cmd_handle_escape_key()`

### 任务 3.3：动态提示符（选中星号）
- [x] 在 `readline()` 中动态构建 prompt（插入 `*` 标记）
- [x] 实现 `request_repaint()` — 通过 `Application.invalidate()`

### 任务 3.4：自动提示 (Hinter)
- [x] 使用 `AutoSuggestFromHistory`
- [x] 灰色斜体样式

### 任务 3.5：rfmEdit 编辑器模式
- [x] 实现 `edit_file(filename)` — 加载文件到独立 `PromptSession`
- [x] Ctrl+O → 保存并退出
- [x] Ctrl+X → 不保存退出
- [x] Esc → 清除选中

### 任务 3.6：中断处理
- [x] 实现 `pt_interrupt()` — 通过 `os.kill(SIGINT)` 中断
- [x] 返回特殊值 `REEDLINE_INTERRUPTED`（保持兼容）
- [x] C 侧 `CancelReadInput` 支持 prompt_toolkit

---

## 阶段 4：C 主程序集成

### 任务 4.1：rfm.c 条件编译
- [x] `#if defined(reedline) || defined(prompt_toolkit)` 覆盖所有相关块
- [x] `#include "prompt_toolkit_wrapper.h"` 添加
- [x] `pt_handle` 变量声明
- [x] `selectionChanged` 中调用 `pt_set_selection_star` + `pt_request_repaint`
- [x] `cmd_handle_escape_key` 中调用 `pt_set_selection_star`
- [x] `CancelReadInput` 中调用 `pt_interrupt`
- [x] `readlineInSeperateThread` 中调用 `pt_readline` / `pt_set_selection_star`
- [x] `save_current_history` 中调用 `pt_save_history`
- [x] `parse_and_exec_command_in_gtk_thread` 中 `parse_rfmEdit` 和 readline 重启逻辑
- [x] `exec_stdin_command` 中 callback 赋值
- [x] 启动初始化中 `pt_create` + functionPointers 赋值
- [x] 补全函数定义（`rfm_filename_completion`、`rfm_editor_completion_based_on_search_result`、`rfm_completion_based_on_dimensions`、`rfmEdit`、`parse_rfmEdit`）对 prompt_toolkit 可见

### 任务 4.2：Makefile 集成
- [x] 添加 `ifeq (${READLINE_LIB},-Dprompt_toolkit)` 条件编译
- [x] 强制检查 `PythonEmbedded` 已启用
- [x] 编译 `rfm_pt_wrapper.c` → `rfm_pt_wrapper.o`
- [x] 链接 `rfm_pt_wrapper.o` 到 `rfm`
- [x] `make clean` 中清理 `rfm_pt_wrapper.o` 和 `__pycache__`

### 任务 4.3：安装脚本
- [x] `make install` 中复制 `rfm_prompt_toolkit/` Python 模块到 `${DESTDIR}${PREFIX}/bin/rfm_prompt_toolkit/`
- [x] 确保 `sys.path` 能找到模块（C 侧 `pt_create` 中动态注入路径）

---

## 阶段 5：测试与文档

### 任务 5.1：功能测试（待进行）
- [ ] TAB 补全：选中文件、文件名、菜单命令、搜索结果、维度
- [ ] 多行编辑：Alt+Enter 插入换行，粘贴多行文本
- [ ] 历史记录：上下箭头、Ctrl+R 搜索、持久化
- [ ] 中断：命令执行时中断 readline
- [ ] 选中星号：选中/取消文件时提示符更新
- [ ] rfmEdit：编辑保存/不保存
- [ ] 新终端窗口模式：使用 rfmReedlineWithSpecificHistoryFile.sh

### 任务 5.2：性能对比（待进行）
- [ ] 测量启动延迟（Python 导入 vs Rust .so 加载）
- [ ] 测量补全响应时间
- [ ] 测量内存占用

### 任务 5.3：文档更新（待进行）
- [ ] 在 INSTALL/README 中添加 prompt_toolkit 配置说明
- [ ] 添加 `prompt_toolkit` 依赖安装说明

---

## 编译验证

- [x] ✅ `make rfm_pt_wrapper.o` — 通过
- [x] ✅ `make rfm` (config.mk: READLINE_LIB = -Dprompt_toolkit, PythonEmbedded = -DPythonEmbedded) — 通过
- [x] 生成的 `rfm` 为 64-bit ELF，包含 Python 链接

## 新增文件清单

| 文件 | 说明 |
|------|------|
| `prompt_toolkit_wrapper.h` | C 头文件，定义 pt_* 接口（对标 reedline_wrapper.h） |
| `rfm_pt_wrapper.c` | C 胶水代码，Python 嵌入 + GIL 管理 + 字符转换 |
| `rfm_prompt_toolkit/__init__.py` | Python 模块入口 |
| `rfm_prompt_toolkit/core.py` | 核心实现：PromptSession、RfmCompleter、rfmEdit |

## 修改文件清单

| 文件 | 改动内容 |
|------|----------|
| `config.mk` | 新增 `#READLINE_LIB = -Dprompt_toolkit` 注释选项 |
| `Makefile` | 新增 prompt_toolkit 编译/链接/安装/清理规则，强制 PythonEmbedded 检查 |
| `rfm.c` | 新增 `#ifdef prompt_toolkit` 分支 20+ 处，`#if defined(reedline) \|\| defined(prompt_toolkit)` 扩展现有补全函数可见性 |

## 使用方式

```makefile
# config.mk 中设置：
PythonEmbedded = -DPythonEmbedded
READLINE_LIB = -Dprompt_toolkit
#READLINE_LIB = -Dreedline
```

然后 `make && sudo make install`。需要系统安装 `prompt_toolkit >= 3.0`。
