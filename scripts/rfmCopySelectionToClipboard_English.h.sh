#!/bin/bash

set -x

/bin/echo $@ | wl-copy

read -p "press enter to close this window"
