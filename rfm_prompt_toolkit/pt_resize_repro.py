#!/usr/bin/env python3
# 最小复现脚本：验证 prompt_toolkit bottom_toolbar 在窗口 resize 时是否残留重复行。
# 用法：在 foot (Wayland/dwl) 下运行，然后用 dwl 的 MOD+Enter 在 master/stack 间切换，
#       观察底部路径行是否每切换一次就多出一行。
# 注意：这里故意把 prompt_toolkit 的事件循环放到子线程（与 rfm 的 readline 线程一致），
#       这会让 prompt_toolkit 关闭 SIGWINCH 处理、退回 0.5s 轮询检测尺寸变化。
import os
import threading

from prompt_toolkit.history import InMemoryHistory
from prompt_toolkit.shortcuts import PromptSession


def run():
    # bottom_toolbar 显示当前路径（等价于 rfm -n 模式下的 rfm_curPath）
    s = PromptSession(
        history=InMemoryHistory(),
        bottom_toolbar=os.getcwd(),
        multiline=False,
    )
    while True:
        try:
            r = s.prompt("PROMPT] ")
        except (KeyboardInterrupt, EOFError):
            break
        print(">>>", r)


if __name__ == "__main__":
    t = threading.Thread(target=run, name="readline")
    t.start()
    t.join()
