#!/bin/bash

# Frame Checker Test Examples
# 帧校验工具测试示例

echo "=== DJI Protocol Frame Checker Test Examples ==="
echo

# Check if frame_check exists
if [ ! -f "./frame_check" ]; then
    echo "Error: frame_check not found. Please run 'make' first."
    exit 1
fi

echo "1. Testing basic frame (auto-detect DATA length):"
echo "   Command: ./frame_check \"aa 40 0 0 0 0 0 0 29 43 95\""
./frame_check "aa 40 0 0 0 0 0 0 29 43 95"
echo
echo "================================================"
echo

echo "2. Testing frame with specified DATA length (2 bytes):"
echo "   Command: ./frame_check -datalen 2 \"aa 40 0 0 0 0 0 0 29 43 95 1D 02 11 22 33 44\""
./frame_check -datalen 2 "aa 40 0 0 0 0 0 0 29 43 95 1D 02 11 22 33 44"
echo
echo "================================================"
echo

echo "3. Testing frame with DATA length 0 (no DATA segment):"
echo "   Command: ./frame_check -datalen 0 \"aa 10 0 0 0 0 0 0 29 43 95 1D 02 11 22 33 44\""
./frame_check -datalen 0 "aa 10 0 0 0 0 0 0 29 43 95 1D 02 11 22 33 44"
echo
echo "================================================"
echo

echo "4. Testing the longer format from your example:"
echo "   Command: ./frame_check \"AA, 38, 00, 01, 00, 00, 00, 00, 71, D5, 3C, 40, 1D, 02, 3C, 01, 0E, 03, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 74, 25, 01, 00, 00, 00, 00, 00, BA, 16, 00, 00, 00, 00, 00, 00\""
./frame_check "AA, 38, 00, 01, 00, 00, 00, 00, 71, D5, 3C, 40, 1D, 02, 3C, 01, 0E, 03, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 74, 25, 01, 00, 00, 00, 00, 00, BA, 16, 00, 00, 00, 00, 00, 00"
echo
echo "================================================"
echo

echo "5. Testing DATA length mismatch scenario:"
echo "   Command: ./frame_check -datalen 10 \"aa 20 0 0 0 0 0 0 29 43 95 1D 02\""
./frame_check -datalen 10 "aa 20 0 0 0 0 0 0 29 43 95 1D 02"
echo
echo "================================================"
echo

echo "All test examples completed!"
echo
echo "Usage summary:"
echo "  ./frame_check <hex_data>                    # Auto-detect DATA length"
echo "  ./frame_check -datalen <len> <hex_data>     # Specify DATA length"
echo "  ./frame_check --help                        # Show help"
echo "  ./frame_check --test                        # Run built-in tests" 