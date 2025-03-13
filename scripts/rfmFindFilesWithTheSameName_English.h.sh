#!/bin/bash

# Get list of selected files
selected_files=("$@")

# Exit if no files selected
if [ ${#selected_files[@]} -eq 0 ]; then
    echo "Please select at least one file"
    exit 1
fi

# Process each selected file
for file in "${selected_files[@]}"; do
    # Get filename without path
    filename=$(basename "$file")
    
    echo "Finding files with the same name as $filename:"
    echo "----------------------------------------"
    
    # Use find command to locate files with same name
    find / -name "$filename" 2>/dev/null | while read found_file; do
        # Skip the currently selected file
        if [ "$found_file" != "$file" ]; then
            echo "$found_file"
        fi
    done
    echo "----------------------------------------"
done

# Wait for user to press enter
read -p "Press Enter to close window..." 