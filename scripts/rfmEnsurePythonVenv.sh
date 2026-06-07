#!/bin/bash
#
# rfmEnsurePythonVenv.sh — rfm 插件框架通用脚本：
#   确保指定插件目录下存在 Python .venv 虚拟环境并激活
#
# 用法（在插件脚本中 source）:
#   _RFM_ADDON_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
#   . /home/guyuming/rfm1/scripts/rfmEnsurePythonVenv.sh "$_RFM_ADDON_DIR"
#
#   # 然后安装本脚本需要的包:
#   pip install --quiet jieba 2>/dev/null || true
#
# 参数:
#   $1 — 插件根目录路径（必需）
#
# 说明:
#   - 只负责 .venv 的存在性检查和激活，不安装任何具体的 Python 包
#   - 各脚本用 rfmEnsurePipPackages 批量检测安装所需包
#   - venv 创建使用系统 python3（Gentoo 推荐做法）
#   - 激活后做健康检查：如果 VIRTUAL_ENV 不匹配，说明 venv 被移动过，自动重建

# rfmEnsurePipPackages — 检测 Python 包是否已安装，缺失时静默安装
# 用法:  rfmEnsurePipPackages pkg1 pkg2 ...
# 说明:  先用 python3 -c "import pkg" 检测，已安装则跳过；
#        缺失时才 pip install，输出重定向到临时日志文件避免污染 stdout
rfmEnsurePipPackages() {
    [ $# -eq 0 ] && return 0
    local _missing="" _pkg
    for _pkg in "$@"; do
        python3 -c "import ${_pkg//-/_}" 2>/dev/null || _missing="$_missing $_pkg"
    done
    if [ -n "$_missing" ]; then
        local _log=$(mktemp -p /tmp rfmPipInstall.XXXXXXXXXX)
        echo "[${BASH_SOURCE[1]##*/}] 安装依赖: $_missing" >&2
        echo "  tail -f $_log   # 查看安装进度" >&2
        pip install --quiet --log "$_log" $_missing
        echo "[${BASH_SOURCE[1]##*/}] 安装完成" >&2
    fi
}

_RFM_ADDON_DIR="${1:-}"

if [ -z "$_RFM_ADDON_DIR" ]; then
    echo "[rfmEnsureVenv] 错误: 未指定插件根目录" >&2
    echo "[rfmEnsureVenv] 用法: . rfmEnsurePythonVenv.sh <插件根目录>" >&2
    return 1
fi

# 转为绝对路径，避免符号链接干扰
_RFM_ADDON_DIR="$(cd "$_RFM_ADDON_DIR" && pwd)"
_VENV_DIR="$_RFM_ADDON_DIR/.venv"
_VENV_ACTIVATE="$_VENV_DIR/bin/activate"

_recreate_venv() {
    echo "[rfmEnsureVenv] 创建 Python 虚拟环境: $_VENV_DIR" >&2
    python3 -m venv "$_VENV_DIR"
    if [ ! -f "$_VENV_ACTIVATE" ]; then
        echo "[rfmEnsureVenv] 错误: 无法创建虚拟环境" >&2
        return 1
    fi

    # 国内环境默认使用清华 pip 源（可通过 RFM_PIP_NO_MIRROR=1 禁用）
    # 默认禁用 pip 缓存以节省磁盘空间（可通过 RFM_PIP_USE_CACHE=1 启用）
    cat > "$_VENV_DIR/pip.conf" <<EOF
[global]
$([ "${RFM_PIP_NO_MIRROR:-0}" != "1" ] && echo "index-url = https://pypi.tuna.tsinghua.edu.cn/simple")
$([ "${RFM_PIP_USE_CACHE:-0}" != "1" ] && echo "no-cache-dir = true")
EOF
    echo "[rfmEnsureVenv] pip 配置已写入: $(head -10 "$_VENV_DIR/pip.conf")" >&2

    echo "[rfmEnsureVenv] 虚拟环境已创建" >&2
}

if [ ! -f "$_VENV_ACTIVATE" ]; then
    _recreate_venv || return 1
fi

source "$_VENV_ACTIVATE"

# 健康检查：Python venv 不可移动，如果插件目录被移动过，
# activate 脚本中的硬编码路径会指向错误位置，导致 VIRTUAL_ENV 不匹配
if [ "$VIRTUAL_ENV" != "$_VENV_DIR" ]; then
    echo "[rfmEnsureVenv] venv 路径不匹配 (VIRTUAL_ENV=$VIRTUAL_ENV, 期望=$_VENV_DIR)，重建..." >&2
    rm -rf "$_VENV_DIR"
    _recreate_venv || return 1
    source "$_VENV_ACTIVATE"
fi

# 清理内部变量，避免污染调用方环境
unset _RFM_ADDON_DIR _VENV_DIR _VENV_ACTIVATE _recreate_venv
