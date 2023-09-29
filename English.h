#define PageUp  "PageUp"
#define PageDown  "PageDown"
#define Up "UP"
#define SwitchView "List/Icon"
#define Menu "Menu"
#define RunActionCopy "Copy"
#define RunActionMove "Move"
#define RunActionDelete "Delete"
#define RunActionGitStage "git stage"
#define RunActionCopySelection "copy file names to clipboard"
#define Copy "Copy"
#define Paste "Paste"
#define MoveTo "MoveToHere"
#define RunActionChangeOwner "ChangeOwner"
#define SwitchPipeDir  "Pipe/Dir"

#define PipeTitle "Current:%d/Total:%d, PageSize:%d"

#define builtinCMD_Help \
"command prompt:\n" \
"    *> means there is selected file(s) in rfm view\n" \
"    >  means no selected files\n" \
"    prompt won't update when selection changes in rfm view, press Enter to refresh\n" \
"builtin commands for current window:\n" \
"    cd address  go to address, note that PWD is not changed, just open address in rfm\n" \
"    quit        quit rfm\n" \
"    help        print this message\n" \
"    press Enter key two times (double enter) to refresh rfm view\n" \
"    /           to switch between icon/list view\n" \
"    //          to switch between current directory and pipe stdin\n" \
"    pwd         get rfm env PWD\n" \
"    setpwd      set rfm env PWD with current directory\n" \
"    pagesize    set files shown per page when displaying filenane list from pipe stdin. for example, pagesize 100\n" \
"Shell commands:\n" \
"    non-builtin commands will be sent to shell to execute.\n" \
"    if there is ending space in command entered, selected filename(s) will be appended at the end\n" \
"    append >0 to commands that output filename list, for example: locate 202309|grep png >0 , it will be displayed in rfm, the same effect as starting rfm after pipeline: locate 202309|grep png|rfm\n" \
"custom builtin commands in config.h:\n" 

#define rfmLaunchHelp \
"This is the help for command line argumants you can use to launch program with, there is another help in command window for commands you can use there.\n" \
"Usage: %s [-h] || [-d <full path>] [-i] [-v] [-l] [-p<custom pagesize>] [-p] [-m]\n" \
"-p       read file name list from StdIn, through pipeline, this -p can be omitted, for example:\n           locate 20230420|grep .png|rfm\n" \
"-px      when read filename list from pipeline, show only x number of items in a batch, for example: -p9. you can also set this in command window with builtin cmd pagesize\n" \
"-d       specify full path, such as /home/somebody/documents, instead of default current working directory\n" \
"-i       show mime type\n" \
"-l       open with listview instead of iconview,you can also switch view with toolbar button or builtin cmd /\n" \
"-h       show this help\n"
