# Python prompt_toolkit 替代 Rust reedline — 任务列表

## 阶段 0：环境准备与依赖

### 任务 0.1：确认 prompt_toolkit 可用性
- [ ] 检查系统是否已有 `prompt_toolkit` (`python3 -c "import prompt_toolkit"`)
- [ ] 在 `rfmEnsurePythonVenv.sh` 中添加 `prompt_toolkit` 自动安装逻辑
- [ ] 确认 `prompt_toolkit` 版本 ≥ 3.0（支持 `PromptSession`）

### 任务 0.2：config.mk 新增选项
- [ ] 添加 `READLINE_LIB = -Dprompt_toolkit` 选项（注释掉 reedline 和 GNU_readline）
- [ ] 确保 `prompt_toolkit` 选项自动启用 `PythonEmbedded`（因为需要 Python 嵌入）
- [ ] 在 Makefile 中添加对应的条件编译分支

---

## 阶段 1：核心 Python 模块 — 基础功能

### 任务 1.1：创建 Python 模块骨架
- [ ] 创建 `rfm_prompt_toolkit/` 目录
- [ ] 创建 `rfm_prompt_toolkit/__init__.py` — 导出主要接口
- [ ] 创建 `rfm_prompt_toolkit/core.py` — 核心 readline 循环
- [ ] 创建 `rfm_prompt_toolkit/completer.py` — 自定义 Completer
- [ ] 创建 `rfm_prompt_toolkit/history.py` — 历史记录管理
- [ ] 创建 `rfm_prompt_toolkit/keys.py` — 键绑定配置

### 任务 1.2：实现 readline() 函数
- [ ] 使用 `PromptSession` 实现同步阻塞的 `readline(prompt_str)` 
- [ ] 支持多行编辑 (`multiline=True`, 通过 Alt+Enter 插入换行)
- [ ] Enter 提交，Ctrl+C / Ctrl+D 处理
- [ ] 返回类型与 C 侧约定一致（字符串 / NULL / "reedline_interrupted"）

### 任务 1.3：实现历史记录
- [ ] 使用 `FileHistory` 保存到 `~/.rfm_prompt_toolkit_history.txt`
- [ ] 实现 `save_history()` 接口（对应 `reedline_save_history`）
- [ ] 实现历史搜索（Ctrl+R 等）

---

## 阶段 2：C-Python 桥接层

### 任务 2.1：创建 C 头文件
- [ ] 创建 `prompt_toolkit_wrapper.h`（对标 `reedline_wrapper.h`）
- [ ] 定义相同的 opaque handle 类型和函数签名

### 任务 2.2：实现 C 胶水代码
- [ ] 创建 `rfm_pt_wrapper.c`（对标 reedline_wrapper Rust 侧的 C 接口）
- [ ] `pt_create()` → 初始化 Python 状态、导入模块、创建 session
- [ ] `pt_readline()` → 获取 GIL → 调用 Python `readline()` → 释放 GIL
- [ ] `pt_interrupt()` → 发送中断信号
- [ ] `pt_request_repaint()` → 请求重绘提示符
- [ ] `pt_set_selection_star()` → 设置选中标记
- [ ] `pt_save_history()` / `pt_free()` / `string_free()`
- [ ] `pt_editor_edit_file()` → rfmEdit 编辑模式

### 任务 2.3：C 补全回调传递
- [ ] 使用 `PyCapsule` 将 `rfmFunctions` 结构体传递给 Python
- [ ] Python 侧用 `ctypes.CFUNCTYPE` 解包 C 函数指针
- [ ] 在 `CCompleter` 中调用 C 补全函数获取候选项
- [ ] 处理三种补全源：文件名(绿)、菜单命令(蓝/黄)、搜索结果(红)

---

## 阶段 3：功能对等实现

### 任务 3.1：TAB 补全 + 交互菜单
- [ ] 实现 `TabCompleter`：先调 C 补全函数，fallback 到 `WordCompleter`
- [ ] 实现 `MenuCompleter`：F2 / Menu 键触发菜单命令补全
- [ ] 实现 `ShiftTabCompleter`：切换递归补全模式
- [ ] 配置 `CompletionMenu` 样式（颜色编码与 reedline 一致）
- [ ] 支持单选项自动补全（`complete_while_typing`）

### 任务 3.2：键绑定
- [ ] Emacs 模式（默认）
- [ ] Alt+Enter → 插入换行
- [ ] Tab → 触发补全菜单
- [ ] Shift+Tab → 递归补全切换
- [ ] F2 / Menu → 菜单命令补全
- [ ] Alt+Up → "Up" 命令 (rfm 上一级目录)
- [ ] Alt+Left → "Previous" 命令
- [ ] Alt+Right → "Next" 命令
- [ ] Esc → 触发 `cmd_handle_escape_key()`（清除选中）

### 任务 3.3：动态提示符（选中星号）
- [ ] 创建 `RfmPrompt` 类，继承或包装 prompt_toolkit 的 prompt
- [ ] 支持动态 `*` 标记（通过全局标志或回调）
- [ ] 实现 `request_repaint()` — 利用 `PromptSession` 的异步重绘

### 任务 3.4：自动提示 (Hinter)
- [ ] 使用 `AutoSuggestFromHistory` 或自定义 suggester
- [ ] 灰色斜体样式，与 reedline CwdAwareHinter 一致

### 任务 3.5：rfmEdit 编辑器模式
- [ ] 实现 `edit_file(filename)` — 加载文件到 `PromptSession`
- [ ] Ctrl+O → 保存并退出
- [ ] Ctrl+X → 不保存退出
- [ ] Esc → 清除选中
- [ ] 支持文件路径 TAB 补全

### 任务 3.6：中断处理
- [ ] 实现 `pt_interrupt()` — 让阻塞的 `prompt()` 返回
- [ ] 返回特殊值 `REEDLINE_INTERRUPTED`（保持兼容）
- [ ] 可能方案：通过 `Application.invalidate()` 或 signal

---

## 阶段 4：C 主程序集成

### 任务 4.1：rfm.c 条件编译
- [ ] 在 rfm.c 中添加 `#ifdef prompt_toolkit` 分支
- [ ] 对标 `#ifdef reedline` 的所有位置添加对应处理
- [ ] `readlineInSeperateThread()` 中调用 `pt_readline()`
- [ ] `selectionChanged` / `cmd_handle_escape_key` 中调用 `pt_set_selection_star()`
- [ ] 确保 `pt_interrupt()` 在 GTK 线程中正确调用

### 任务 4.2：Makefile 集成
- [ ] 添加 `ifeq (${READLINE_LIB},-Dprompt_toolkit)` 条件
- [ ] 自动启用 `PythonEmbedded`（如果未设置）
- [ ] 链接 Python 库（复用现有 `python3-config --ldflags --embed`）
- [ ] 编译 `rfm_pt_wrapper.c`
- [ ] `make clean` 中清理 Python `__pycache__`

### 任务 4.3：安装脚本
- [ ] `make install` 中复制 `rfm_prompt_toolkit/` Python 模块
- [ ] 安装到合适位置（如 `${PREFIX}/share/rfm/` 或作为 Python package）
- [ ] 确保 `sys.path` 能找到模块

---

## 阶段 5：测试与文档

### 任务 5.1：功能测试
- [ ] TAB 补全：选中文件、文件名、菜单命令、搜索结果、维度
- [ ] 多行编辑：Alt+Enter 插入换行，粘贴多行文本
- [ ] 历史记录：上下箭头、Ctrl+R 搜索、持久化
- [ ] 中断：命令执行时中断 readline
- [ ] 选中星号：选中/取消文件时提示符更新
- [ ] rfmEdit：编辑保存/不保存
- [ ] 新终端窗口模式：rfmReedline 替代

### 任务 5.2：性能对比
- [ ] 测量启动延迟（Python 导入 vs Rust .so 加载）
- [ ] 测量补全响应时间
- [ ] 测量内存占用

### 任务 5.3：文档更新
- [ ] 更新 `使用reedline库时TAB补全操作总结.md`
- [ ] 在 INSTALL/README 中添加 prompt_toolkit 配置说明
- [ ] 添加 `prompt_toolkit` 依赖安装说明

---

## 优先级建议

| 优先级 | 阶段 | 说明 |
|--------|------|------|
| 🔴 P0 | 0.1, 0.2 | 环境准备，确认可行性 |
| 🔴 P0 | 1.1, 1.2 | 最小可用原型（基本输入） |
| 🔴 P0 | 2.1, 2.2 | C-Python 桥接打通 |
| 🟡 P1 | 3.1 | TAB 补全（最核心功能） |
| 🟡 P1 | 4.1, 4.2 | 集成到主程序 |
| 🟢 P2 | 3.2~3.6 | 其余功能对等 |
| 🟢 P2 | 5.1~5.3 | 测试与文档 |
