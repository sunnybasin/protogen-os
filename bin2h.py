#!/usr/bin/env python3
"""
Converts a binary file (e.g. a .gif) into a C header file containing the
raw bytes as a const array, so it can be compiled directly into firmware
instead of needing a runtime filesystem.

Usage:
    python bin2h.py silence.gif silence_gif silence_gif.h
    python bin2h.py talking.gif talking_gif talking_gif.h
"""
import sys

def bin2header(filename, varname, outname):
    with open(filename, 'rb') as f:
        data = f.read()
    with open(outname, 'w') as f:
        f.write(f'// Auto-generated from {filename} - do not edit by hand\n')
        f.write('#pragma once\n\n')
        f.write(f'const unsigned char {varname}[] = {{\n')
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            f.write('  ' + ', '.join(f'0x{b:02x}' for b in chunk) + ',\n')
        f.write('};\n')
        f.write(f'const unsigned int {varname}_len = {len(data)};\n')
    print(f"Wrote {outname}: {len(data)} bytes as '{varname}'")

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print("Usage: python bin2h.py <input.gif> <variable_name> <output.h>")
        sys.exit(1)
    bin2header(sys.argv[1], sys.argv[2], sys.argv[3])