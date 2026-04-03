# 安装环境
OpenKylin version 2.0 SP2 (nile)

因为ostree的原因，默认没法通过apt在OpenKylin上安装软件。要先在 设置->关于 界面点击5次 logo， 会在关于下面出现 维护模式，在维护模式页面打开维护模式后才可以用sudo apt install

sudo apt install build-essential pkg-config libgtk-3-dev libjson-glib-dev python3-dev libreadline-dev libcmark-dev

使用rustup官方网站安装脚本安装rustc和cargo，否则，使用apt安装版本会比较低编译会出错

sudo apt install git

设置Git显示中文名
git config --global core.quotepath false

git clone https://gitee.com/guyuming76/rfm.git
cd rfm

# 安装AI addon（可选）
git submodule update --init AI_model_API_config

#为AI模型配置APIkey：创建一个API_KEY.txt, 内容为key
#cat AI_model_API_config/hosts/deepseek/API_KEY.txt
#sk-xxxxxxxxxxxxxxxxxxxxxx


# 编译系统
make clean; make

# 运行update_deb.sh生成安装包，并按提示安装，注意Recommands 和 Suggests 的依赖项，rfm使用Recommends的依赖项来定位编译插件(addon)，AI addon使用依赖项中的jq
