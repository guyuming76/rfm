#!/bin/bash

RFM_LOCAL_C_FILE="$1"
RFM_LOCAL_C_FILE_SUFFIX="$1""$"

locate $RFM_LOCAL_C_FILE |grep $RFM_LOCAL_C_FILE_SUFFIX | xargs grep -l "const enum RFM_ADDON_TYPE addon_type = Global;"
