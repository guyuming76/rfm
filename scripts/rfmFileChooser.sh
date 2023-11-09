#!/bin/bash

export rfmFileChooserReturnSelectionIntoFilename=$(tty)
export rfmFileChooserResultNumber=0

eval '/usr/bin/foot --log-level=warning  rfm -r "$@"'

exit $rfmFileChooserResultNumber
