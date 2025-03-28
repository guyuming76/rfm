#!/bin/bash

# Create temporary file (auto-cleaned on exit)
tmpfile=$(mktemp 2>/dev/null || mktemp -t 'rftmp') || exit 1
trap 'rm -f "$tmpfile" 2>/dev/null' EXIT HUP INT TERM

# Save piped input to temporary file
cat > "$tmpfile"

# Validate input content
if [[ ! -s "$tmpfile" ]]; then
    echo "Error: Empty input detected!" >&2
    exit 1
fi

# Interactive filename prompt
while :; do
    read -p "Enter filename [default: output.txt]: " filename </dev/tty
    filename="${filename:-output.txt}"

    # Validate filename format
    if [[ "$filename" =~ ^[[:space:]]*$ ]]; then
        echo "Error: Filename cannot be empty!" >&2
        continue
    fi

    # Handle existing files
    if [[ -e "$filename" ]]; then
        read -p "Warning: '$filename' exists. Overwrite? (y/N) " confirm </dev/tty
        if [[ "${confirm,,}" =~ ^(y|yes)$ ]]; then
            rm -f "$filename"
            break
        else
            continue
        fi
    else
        break
    fi
done

# Atomic file save operation
if mv -f "$tmpfile" "$filename" 2>/dev/null; then
    file_size=$(wc -c < "$filename" | awk '{print $1}')
    formatted_size=$(numfmt --to=iec "$file_size" 2>/dev/null || echo "${file_size} bytes")
    echo "Success: File '$filename' saved ($formatted_size)"
else
    echo "Error: Failed to save file!" >&2
    exit 1
fi
