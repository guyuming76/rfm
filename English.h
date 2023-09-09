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

#define builtinCMD_Help \
"command prompt:\n" \
"    *> means there is selected file(s) in rfm view\n" \
"    >  means no selected files\n" \
"    prompt won't update when selection changes in rfm view, press Enter to refresh\n" \
"builtin commands for current window:\n" \
"    pwd         get rfm env PWD\n" \
"    setpwd      set rfm env PWD with current directory\n" \
"    cd address  go to address, note that PWD is not changed, just open address in rfm\n" \
"    quit        quit rfm\n" \
"    help        print this message\n" \
"    press Enter key two times (double enter) to refresh rfm view\n" \
"    /           to switch between icon/list view\n" \
"Shell commands:\n" \
"    non-builtin commands will be sent to shell to execute.\n" \
"    if there is ending space in command entered, selected filename(s) will be appended at the end\n" \
"custom builtin commands in config.h:\n" 
