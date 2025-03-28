#!/bin/bash

# Loop to prompt for non-empty subdirectory name
while true; do
    read -p "Enter subdirectory name: " dirname
    if [[ -z "$dirname" ]]; then
        echo "Error: Directory name cannot be empty!"
    else
        break
    fi
done

# Attempt to create directory (prevent automatic parent creation)
if ! mkdir "$dirname" 2>/dev/null; then
    echo "Error: Failed to create directory '$dirname' (may already exist or invalid name)" >&2
    exit 1
fi

# Check if file arguments are provided
if [[ $# -eq 0 ]]; then
    echo "Error: No files provided for moving" >&2
    exit 1
fi

# Move files with error handling
moved_count=0
for file in "$@"; do
    if [[ -e "$file" ]]; then
        if mv -v -- "$file" "$dirname/"; then
            ((moved_count++))
        else
            echo "Warning: Failed to move '$file'" >&2
        fi
    else
        echo "Warning: File '$file' does not exist, skipped" >&2
    fi
done

# Output summary
echo "Operation completed! Successfully moved $moved_count file(s) to $dirname/"
exit 0
