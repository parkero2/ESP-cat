#!/usr/bin/env python3
"""
Script to decompress/recompress HTML data from ESP32 camera_index.h file
"""

import gzip
import re
import os

def extract_c_array_data(file_path, array_name):
    """Extract byte array data from C header file"""
    with open(file_path, 'r') as f:
        content = f.read()
    
    # Find the array definition
    pattern = rf'const unsigned char {array_name}\[\] = \{{([^}}]+)\}};'
    match = re.search(pattern, content, re.DOTALL)
    
    if not match:
        raise ValueError(f"Array {array_name} not found in file")
    
    # Extract hex values
    hex_data = match.group(1)
    hex_values = re.findall(r'0x[0-9A-Fa-f]{2}', hex_data)
    
    # Convert to bytes
    byte_data = bytes(int(val, 16) for val in hex_values)
    return byte_data

def decompress_html(gzipped_data):
    """Decompress gzipped HTML data"""
    return gzip.decompress(gzipped_data).decode('utf-8')

def compress_html(html_content):
    """Compress HTML content to gzip"""
    return gzip.compress(html_content.encode('utf-8'))

def bytes_to_c_array(byte_data, array_name):
    """Convert bytes to C array format"""
    hex_values = [f"0x{b:02X}" for b in byte_data]
    
    # Format as C array with proper line breaks
    lines = []
    for i in range(0, len(hex_values), 16):
        line = ", ".join(hex_values[i:i+16])
        lines.append(f"  {line}")
    
    array_content = ",\n".join(lines)
    
    return f"""//File: index_ov2640.html.gz, Size: {len(byte_data)}
#define {array_name}_len {len(byte_data)}
const unsigned char {array_name}[] = {{
{array_content}
}};"""

def main():
    # File paths
    header_file = "/Users/oliver/Documents/code/ESP-cat/CWS2/CameraWebServer/CameraWebServer/camera_index.h"
    html_output = "extracted_index.html"
    modified_html = "modified_index.html"
    
    try:
        # Step 1: Extract and decompress the HTML
        print("Extracting gzipped data from camera_index.h...")
        gzipped_data = extract_c_array_data(header_file, "index_ov2640_html_gz")
        
        print("Decompressing HTML...")
        html_content = decompress_html(gzipped_data)
        
        # Save decompressed HTML
        with open(html_output, 'w', encoding='utf-8') as f:
            f.write(html_content)
        print(f"Decompressed HTML saved to: {html_output}")
        
        # Step 2: Check if modified HTML exists, if so, recompress it
        if os.path.exists(modified_html):
            print(f"\nFound {modified_html}, recompressing...")
            
            with open(modified_html, 'r', encoding='utf-8') as f:
                modified_content = f.read()
            
            # Compress the modified HTML
            compressed_data = compress_html(modified_content)
            
            # Generate new C array
            new_c_array = bytes_to_c_array(compressed_data, "index_ov2640_html_gz")
            
            # Save the new C array
            with open("new_camera_index_array.h", 'w') as f:
                f.write(new_c_array)
            
            print("New C array saved to: new_camera_index_array.h")
            print("You can copy this content back into camera_index.h")
        else:
            print(f"\nTo modify the HTML:")
            print(f"1. Edit {html_output}")
            print(f"2. Save your changes as {modified_html}")
            print(f"3. Run this script again to recompress")
    
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()
