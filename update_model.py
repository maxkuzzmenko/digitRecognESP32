#!/usr/bin/env python3
"""
Script to replace the TensorFlow Lite model in digit_recogn.h with data from file.txt
"""

import re
import os

# File paths
DOWNLOADS_FILE = os.path.expanduser("~/Downloads/file.txt")
HEADER_FILE = os.path.join(os.path.dirname(__file__), "digit_recogn.h")

def replace_model_in_header(header_path, downloads_file):
    """
    Replace the model section in digit_recogn.h with content from downloads file
    """
    # Read the new model data
    print(f"Reading new model data from {downloads_file}...")
    with open(downloads_file, 'r') as f:
        new_model_content = f.read()

    # Normalize variable name if needed (file.txt uses digits_model_len, header uses model_data_len)
    new_model_content = new_model_content.replace('digits_model_len', 'model_data_len')

    # Count the model size for reporting
    model_len_match = re.search(r'model_data_len\s*=\s*(\d+)', new_model_content)
    model_len = int(model_len_match.group(1)) if model_len_match else 0

    # Read the current header file
    print(f"Reading current header file {header_path}...")
    with open(header_path, 'r') as f:
        header_content = f.read()

    # Find and replace the pattern:
    # From "const unsigned char digits_model[]" to "const int model_data_len = ...;"
    pattern = r'const unsigned char digits_model\[\][^;]*?DATA_ALIGN_ATTRIBUTE\s*=\s*\{.*?\};\s*const int model_data_len\s*=\s*\d+;'

    # Check if pattern exists
    match = re.search(pattern, header_content, re.DOTALL)
    if not match:
        print("Error: Could not find model declaration pattern in header file")
        print("Looking for pattern: const unsigned char digits_model[] ... const int model_data_len = ...;")
        return False

    old_len_match = re.search(r'model_data_len\s*=\s*(\d+)', match.group(0))
    old_len = int(old_len_match.group(1)) if old_len_match else 0

    # Extract just the model declaration from new content (from const unsigned char to the semicolon after model_data_len)
    new_model_match = re.search(
        r'const unsigned char digits_model\[\][^;]*?DATA_ALIGN_ATTRIBUTE\s*=\s*\{.*?\};\s*const int model_data_len\s*=\s*\d+;',
        new_model_content,
        re.DOTALL
    )

    if not new_model_match:
        print("Error: Could not extract model declaration from new file")
        return False

    new_model_section = new_model_match.group(0)

    # Perform the replacement
    print(f"Replacing model in header file...")
    print(f"  Old model size: {old_len:,} bytes")
    print(f"  New model size: {model_len:,} bytes")

    new_header_content = re.sub(pattern, new_model_section, header_content, count=1, flags=re.DOTALL)

    # Write the updated header file
    with open(header_path, 'w') as f:
        f.write(new_header_content)

    return True

def main():
    # Check if files exist
    if not os.path.exists(DOWNLOADS_FILE):
        print(f"Error: {DOWNLOADS_FILE} not found")
        return 1

    if not os.path.exists(HEADER_FILE):
        print(f"Error: {HEADER_FILE} not found")
        return 1

    print("=" * 60)
    print("TensorFlow Lite Model Replacement Script")
    print("=" * 60)

    # Replace in header file
    if replace_model_in_header(HEADER_FILE, DOWNLOADS_FILE):
        print("\n" + "=" * 60)
        print("✓ Model replacement complete!")
        print("=" * 60)
        return 0
    else:
        print("\n" + "=" * 60)
        print("✗ Model replacement failed!")
        print("=" * 60)
        return 1

if __name__ == "__main__":
    exit(main())
