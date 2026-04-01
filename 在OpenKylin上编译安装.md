OpenKylin version 2.0 SP2 (nile)

因为ostree的原因，默认没法通过apt在OpenKylin上安装软件。要先在 设置->关于 界面点击5次 logo， 会在关于下面出现 维护模式，在维护模式页面打开维护模式后才可以用sudo apt install



sudo apt install git
#设置Git显示中文名
git config --global core.quotepath false

sudo apt install build-essential pkg-config libgtk-3-dev

sudo apt install libjson-glib-dev

sudo apt install python3-dev libreadline-dev libcmark-dev

使用rustup官方网站安装脚本安装rustc和cargo，否则，使用apt安装版本会比较低编译会出错

sudo apt install llvm-17

sudo apt install bat
alisa bat='batcat'

sudo apt install imv
#imv-wayland


export LD_LIBRARY_PATH=/usr/local/lib64
#运行rfm.sh前设置上面环境变量以便找到libreedline_wrapper.so
