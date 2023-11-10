#!/bin/bash

# the tty command not always return valid tty filename, for example, when called in g_spawn. So, we need rfm -r to pass in the valid named pipe.
export rfmFileChooserReturnSelectionIntoFilename=$(tty)
export rfmFileChooserResultNumber=0

if [ $# -eq 0 ]; then
	eval '/usr/bin/foot --log-level=warning  rfm -r'
elif [ $# -eq 1 ]; then
	# ${!#} means the last parameter
	eval '/usr/bin/foot --log-level=warning  rfm -r "${!#}"'
else
	# "${@:2}" is bash parameter expansion, means "$2" "$3" "$4" ...
	eval '/usr/bin/foot --log-level=warning  rfm -r "${!#}" -d "${@:1:$#-1}"'
fi

# eval above make the commands run in current process instead of a sub shell process, so that we can get the updated variable value below.
exit $rfmFileChooserResultNumber
