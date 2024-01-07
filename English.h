#define PageUp  "PageUp"
#define PageDown  "PageDown"
#define Up "UP"
#define SwitchView "List/Icon"
#define Menu "Menu"
#define RunActionCopy "Copy"
#define RunActionMove "Move(rename)"
#define RunActionDelete "Delete"
#define RunActionGitStage "git stage"
#define RunActionCopySelection "copy file names to clipboard"
#define Copy "CopyFile"
#define Paste "Paste"
#define MoveTo "MoveToHere"
#define RunActionChangeOwner "ChangeOwner"
#define SwitchPipeDir  "Pipe/Dir"

#define PipeTitle "Current:%d/Total:%d, PageSize:%d"

#define builtinCMD_Help \
"system views:\n" \
"    with difference in data source, two types of view can be shown, one is current directory view, with the left-most toolbar button showing its path(and git branch). The other is search result view. You can switch between these two views with the left-most toolbar button.\n" \
"    with difference in layout, there are also two types of view: icon view and list view, icons such as picture thumbnails are shown in icon view.\n" \
"command prompt:\n" \
"    b*> means there is selected file(s) in rfm view, and current command interpreter is Bash, which can be configured in config.h\n" \
"        press TAB with empty prefix will insert the current selected file fullpath; press TAB with non-empty prefix will call readline default completion function.\n" \
"    b>  means no selected files\n" \
"    b?> means in refreshing and file selection cannot be determined, try press enter after refresh.\n" \
"    prompt won't update when selection changes in rfm view, press Enter to refresh\n" \
"builtin commands for current window:\n" \
"    cd address  go to address, note that PWD is not changed, just open address in rfm.\n" \
"    cd ..       go up to parent directory.\n" \
"    cd -        go to directory in OLDPWD environment variable, that is, previously opened directory.\n" \
"    cd          cd followed by space key with directory selected will enter that directory, and will enter parent directory of a file if file selected.\n" \
"                cd without parameter or space just output current directory, which can be different from the output of pwd command.\n" \
"    quit        quit rfm\n" \
"    help        print this message\n" \
"    press Enter key two times (double enter) to refresh rfm view\n" \
"    /           to switch between icon/list view, data not refreshed\n" \
"    //          to switch between current directory and search result view, with data refreshed\n" \
"    pwd         get rfm env PWD\n" \
"    setpwd      set rfm env PWD with current directory\n" \
"    pagesize    set files shown per page in search result view. for example, pagesize 100\n" \
"    thumbnailsize  set thumbnail size in icon view. \n" \
"    showcolumn  show or hide column (if currently in listview); current treeview column layout will be added in to command history if no parameter follows showcolumn\n" \
"    toggleInotifyHandler  stop/start auto refresh\n" \
"Shell commands:\n" \
"    non-builtin commands will be sent to shell to execute.\n" \
"    if there is ending space in command entered, selected filename(s) will be appended at the end. for example, you can view currently selected maildir mail file with `mu view `, with ending space before return.\n" \
"    continue with the example above, if you want to view with less, you can use `mu view %s|less `. although %s in the command is replaced with selected filename, you still have to end the whole command line with space to trigger the filename replacing. one %s for one selected filename, if you choose multiple filenames, you can add more %s."
"    append >0 to commands that output filename list, for example: locate 202309|grep png >0 , it will be displayed in rfm, the same effect as starting rfm after pipeline: locate 202309|grep png|rfm\n" \
"custom builtin commands in config.h:\n" 

#define rfmLaunchHelp \
"This is the help for command line argumants you can use to launch program with, there is another help in command window for commands you can use there.\n" \
"Usage: %s [-h] [-H] [-d <full path>] [-i] [-v] [-l] [-p<custom pagesize>] [-p] [-s] [-t] [-T<thumbnailsize>]\n" \
"       press q in rfm window to quit.\n" \
"-p       read file name list from StdIn, through pipeline, this -p can be omitted, for example:\n           locate 20230420|grep .png|rfm\n" \
"-px      when read filename list from pipeline, show only x number of items in a batch, for example: -p9. you can also set this in command window with builtin cmd pagesize\n" \
"-d       specify full path to show, such as -d /home/somebody/maildir, instead of default current working directory. If file name follows -d, the directory of the file will be opened and the file selected in view\n" \
"-i       show mime type\n" \
"-l       open with listview instead of iconview,you can also switch view with toolbar button or builtin cmd /\n" \
"-s       specify columns to show in listview and their order, refer to builtin command showcolumn for detail.\n" \
"-r       followed by named pipe through which file selection is returned. Used when rfm works as file chooser, such as with program file open menu.\n" \
"         if no named pipe follows, selection will be returned through printf, and rfm will start without readline thread, as if -t parameter is also added.\n" \
"-t       start rfm without readline thread, neither show command prompt nor accept command line input.\n" \
"-T       set RFM_THUMBNAIL_SIZE, same as thumbnailsize command above.\n" \
"-h       show this help\n" \
"-H       stop auto refresh\n"

#define SHOWCOLUMN_USAGE \
"showncolumn command can have mulitple arguments deliminated with space, argument can be positive or negative column number, or (negative)column numbers connected with ',' or ';'. showcolumn without any argument display column name and number mapping, and current displaying status. Note that numbers in showcolumn argument are column number for column name, not the displaying position.\n\n" \
"Usage example: showcolumn 10,9,-12   means to move column 9 right after 10, and hide 12 right after 9\n" \
"               showcolumn 1,3 12     means to move column 3 right after 1, and set column 12 visible without changing its position\n" \
"               showcolumn ,2         means show column 2 as the first column\n" \
"               showcolumn 2,         means hide columns after 2\n"

#define VALUE_MAY_NOT_LOADED \
"Value for column %d(%s) may has not been loaded yet, refresh needed. Note that if you just switch list/icon view, the column will appear, but with empty value before data refreshed\n"
