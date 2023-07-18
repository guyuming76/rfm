#!/bin/bash

set -x

git status

read -p "是否运行 git diff --staged? y/N" -r diffstaged

[[ "$diffstaged" == "y" ]]  && git diff --staged

read -p "是否运行 git commit? Y/n" -r gitCommit

[[ "$gitCommit" == "n" ]] && exit

git commit
