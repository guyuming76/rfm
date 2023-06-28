#!/bin/bash

set -x

# 这个功能目前还没完成！
# $@包含从rfm传过来的git(短)commit id.需要rfm传入commit id, 比如从选中文件的 gitMessage字段前几位获得短commitid.
# 因为可以多选图片然后 imv 打开，这个脚本的功能变得可有可无，而且实现起来也麻烦，所以暂且这样
git show $@ &

git show --name-only --oneline $@|grep .png|imv
