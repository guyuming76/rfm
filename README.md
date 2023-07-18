
在rfm的基础上，我作了定制，使用参数-p,接收管道输入文件名，以大图标查看图片，选中文件按回车用IMV查看完整图片。比如命令：

`locate 菱锰|grep .jpg|rfm`

![接收管道输入的图片文件名，以大图标查看图片](20230401_12h02m34s_grim.png)

在config.def.h, config.h里配置使用IMV软件看图

![回车键用IMV查看完整图片](20230401_11h33m26s_grim.png)

如果希望选中图片回车打开每次都使用新的IMV窗口，而不是在同一个IMV窗口里切换图片，可以使用如下命令：

`locate 菱锰|grep .jpg|rfm & exit`


一些我自己常用的操作流程：比如截屏后，把图片复制或移动到相应文档目录，然后 git stage 这些图片和文本信息，再git commit。就可以先cd进入相应文档目录，让后通过find 或locate找到最近的截图文件，用rfm显示出来，再多选要复制的图片后，在rfm 里面使用文件上下文菜单完成到当前目录的复制，当要选择复制的图片数量稍多时，比字符终端输 cp命令方便

```
find ~ -cmin -5 2>/dev/null|grep png|rfm
```

相比Rodney的rfm,我这里的一个大的改动是去除了文件复制，移动删除等操作的gtk对话窗口，改用脚本里面的对话实现，如下图的文件删除界面。我的总体思路是不想做复杂精细的图形界面（图形文件管理器已经很多了），而是把简单的图形界面和脚本的配合使用作为特点。
![rfm 上下文菜单删除文件操作](20230627_19h01m58s_grim.png)

又比如移动文件，可以选中文件后在上下文菜单里选择“移动”，然后在脚本对话里输入目的地址，也可以在上下文菜单里选择“复制文件名至剪贴板”。然后导航到目的目录后使用工具栏上的“移动至”功能（粘贴按钮对应的是复制功能），从而移动剪贴板上的文件名列表到当前目录。

至于在目录间导航，除了使用工具栏上的“父”目录按钮，还可以在启动rfm的命令窗口(rfm程序的标准输入文件)里输入 `cd 目录完整路径` 命令,如下图，你可以输入 `cd /tmp` 进入 /tmp 目录，然后再输入 `cd .` 回到原目录， 注意，这里的`.`指代的是启动rfm的命令窗口里的当前路径，而不是rfm窗口的当前路径。
![rfm标准输入文件cd命令进入指定目录操作](20230627_19h31m53s_grim.png)

当只需要当前目录下的文件时，可以用 `locate $(pwd)` 返回当前目录下所有文件的完整路径;可以建立一个alias,方便今后输入：

```
alias locatepwd='locate $(pwd)'
```

然后可以用如下命令选出当前目录下所有 .md结尾的文件，用rfm 展示并在根据MIME type 用相应应用打开：

```
 locatepwd|grep .md$|rfm
```

更新20230715:可以用如下命令打开当前目录下(包含子目录)的文件. 
```
find |rfm -p100 -l
```

也可以用[其他方法获取文件完整路径](https://blog.csdn.net/yaxuan88521/article/details/128172956),比如：

```
ls|xargs readlink -f|rfm
ls|xargs realpath|rfm
```

更新20230706:接受相对路径文件名作为管道输入(代码调用canonicalize_file_name函数,同上面realpath命令):
```
ls | rfm
#有时ls会多列输出,要用参数 -w1 指明显示一列
ls -w1 |rfm
```


多张图片git stage 操作也比较稍嫌麻烦，当然我可以用类似如下命令一次stage多个当天的图片文件：

```
ls *20230410*|xargs git stage 
```

但现在可以在rfm里多选多个文件，然后通过鼠标右键菜单选择Stage操作。

rfm 原来只实现了用icon_view显示内容，这里怎加了-l 参数，使用列表视图显示内容，类似 ls -l 的显示内容。也可以在rfm界面里面使用 MOD+l 组合键切换icon或list视图。MOD键默认定义为Win键，可以在config.h里重新定义为Alt或别的键

![rfm -l](20230410_12h55m39s_grim.png)

在rfm列表视图里，若想不用鼠标选中多个文件，可以按住ctrl键后用上下键移到到待选文件，按空格选中。但我用fcitx5拼音输入法，默认ctrl+space是开启/禁用输入法按键。我不知道如何更换gtk视图的快捷键，但可以通过fcitx5-configtool更改拼音输入法的快捷键解决冲突。


如果在多个tty分别启动了wayland和xorg显示服务，可以用如下命令决定rfm这样的gtk应用窗口显示在那里：

```
WAYLAND_DISPLAY=wayland-1 rfm
GDK_BACKEND=x11 rfm

```


Firefox 下载界面有个"open containning folder" 按钮，用文件管理器打开下载目录。我图形界面用DWL,没有KDE啥的设置默认文件管理器的功能。可以编辑 /usr/share/applications/mimeinfo.cache 文件，找到 inode/directory= 这一行，然后把 rfm.desktop 设置为等号后面第一项。我的rfm Makefile里面包含了rfm.desktop文件的安装。[参见](https://askubuntu.com/questions/267514/open-containing-folder-in-firefox-does-not-use-my-default-file-manager)



#一些TroubleShooting记录#

##显示调试信息##

```
export G_MESSAGES_DEBUG=rfm
#或者 export G_MESSAGES_DEBUG=all
```
对于 G_LOG_DOMAIN rfm, 我没有做进一步细分，如果需要过滤可以 grep, 比如需要看包含g_spawn 的调试输出：

```
G_MESSAGES_DEBUG=rfm rfm|grep g_spawn
```

参见：
https://blog.gtk.org/2017/05/04/logging-and-more/

## gentoo上gdk-pixbuf这个包及其USE flag ##

我在gentoo笔记本上.jpg文件图标无法生成：
```thumbnail null, GError code:3, GError msg:Couldn’t recognize the image file format for file “/home/guyuming/plant/IMG_20230513_145210.jpg”```
源于 gdk_pixbuf_new_from_file_at_scale 函数

网上搜不到答案，只提到有个 gdk-pixbuf-query-loaders 工具可以在shell里运行，但我gentoo上也没找到这个工具，估计没装，在寻找这个工具安装过程中发现有gdk-pixbuf这个USE flag. 

后来 `euse -i gdk-pixbuf` 发现有 x11-libs/gdk-pixbuf 这个包，包上有个jpeg USE flag没打开，打开后 .jpg文件就可以产生缩略图了。

