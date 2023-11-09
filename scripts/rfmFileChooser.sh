#!/bin/bash

export rfmReturnSelectionDestination=$(tty)
export rfmReturnSelectionNumber=0

eval '/usr/bin/foot --log-level=warning  rfm -r "$@"'

exit $rfmReturnSelectionNumber
