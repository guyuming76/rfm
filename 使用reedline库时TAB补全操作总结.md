# 使用reedline库时TAB补全操作总结 #

## 数据源 ##

### 1. 基于当前选中文件 ###
当前有文件被选中，且光标前字符为空时，TAB不提示补全菜单，自动补全当前选中文件（若多选，只有一个有效）。

### 2.基于GNU readline 文件补全 ###
若没有文件选中，使用gnu readline库函数 rl_filename_completion_function 获取TAB补全候选项，在补全菜单显示为绿色。同时补全菜单还会同时包含下面描述的黄色和红色内容。

参见函数代码：rfm_filename_completion

### 3.基于查询结果页面的列标题和值 ###
当前为查询结果页面时，且包含config.h中定义的取值函数为getExtColumnValueFromHashTable的扩展列，当输入行光标前为空的时候，TAB补全菜单会出现红色内容的扩展标题列名。选中红色标题列名补全后，再继续TAB补全，则会出现红色的此列值的补全内容。在补全输入列标题名后，可以手工输入其他符号如等号(=),然后再按TAB补全列值。

参见函数代码： rfm_editor_completion_based_on_search_result

### 4.基于维度定义 ###
在文件目录内加入名为 "rfm维度属性" 的文件，则此目录会被rfm定义唯一维度，其下子目录名都是此维度的成员。
维度名和成员在TAB补全菜单中显示为黄色。
TAB补全完成维度名输入后，接着按TAB键，会出现相应维度成员值补全内容，值前会自动加等号分隔，不要手工输入。

参见函数代码： rfm_completion_based_on_dimensions


## 用到TAB补全的命令状态 ##

### 1. 命令解释器提示状态，如bash提示状态 ###
参见代码：reedline_wrapper/src/lib.rs 中 reedline_readline 函数

### 2. rfmEdit 命令状态 ###
参见代码：reedline_wrapper/src/lib.rs 中 rfm_editor_edit_file 函数

### 3. "维度" 命令状态 ###
暂时"维度"命令后的补全菜单内容没有作单独限制，同1.命令解释器提示状态，

### 4. rfm打开新的终端模拟器窗口后提示输入文件路径状态 ###
这里运行于不同与rfm进程的独立进程，不方便使用上述多种数据源，只使用 "2.基于GNU readline 文件补全"

参见代码：rfmReedline/src/main.rs
