CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
INCLUDES = -I../../utils/crc -I../../protocol
SRCDIR = ../..
SOURCES = frame_check.c $(SRCDIR)/utils/crc/custom_crc16.c $(SRCDIR)/utils/crc/custom_crc32.c
TARGET = frame_check

# Build the frame checker
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET) $(SOURCES)

# Clean build artifacts
clean:
	rm -f $(TARGET)

# Install (optional)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

.PHONY: clean install test run help

# Run with all predefined test frames
test:
	@echo "Running all predefined test frames..."
	./$(TARGET) --test

# Run without arguments (same as test)
run:
	@echo "Running frame checker with predefined frames..."
	./$(TARGET)

# Test with specific frame formats
test-format1:
	@echo "Testing Format 1: 'aa 40 0 0 0 0 0 0 29 43 95'"
	./$(TARGET) "aa 40 0 0 0 0 0 0 29 43 95"

test-format2:
	@echo "Testing Format 2: 'AA, 38, 00, 01, ...'"
	./$(TARGET) "AA, 38, 00, 01, 00, 00, 00, 00, 71, D5, 3C, 40, 1D, 02, 3C, 01, 0E, 03, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 74, 25, 01, 00, 00, 00, 00, 00, BA, 16, 00, 00, 00, 00, 00, 00"

test-mixed:
	@echo "Testing mixed format..."
	./$(TARGET) "AA 55,1E 00 02,00 00 00"

test-single-digits:
	@echo "Testing single digit format..."
	./$(TARGET) "a 4 0 0 0 0 0 0"

# Test with -datalen parameter
test-datalen:
	@echo "Testing with specified DATA length (2 bytes)..."
	./$(TARGET) -datalen 2 "aa 40 0 0 0 0 0 0 29 43 95 1D 02 11 22 33 44"

test-datalen-zero:
	@echo "Testing with DATA length 0 (no DATA segment)..."
	./$(TARGET) -datalen 0 "aa 10 0 0 0 0 0 0 29 43 95 1D 02 11 22 33 44"

test-datalen-long:
	@echo "Testing with longer DATA length (8 bytes)..."
	./$(TARGET) -datalen 8 "aa 18 0 0 0 0 0 0 29 43 95 1D 02 AA BB CC DD EE FF 00 11 22 33 44 55"

test-datalen-mismatch:
	@echo "Testing DATA length mismatch scenario..."
	./$(TARGET) -datalen 4 "aa 20 0 0 0 0 0 0 29 43 95 1D 02"

# Test all datalen scenarios
test-all-datalen: test-datalen test-datalen-zero test-datalen-long test-datalen-mismatch
	@echo "All datalen tests completed."

# Comprehensive test suite
test-all: test test-format1 test-format2 test-mixed test-single-digits test-all-datalen
	@echo "All tests completed."

help:
	@echo "Available targets:"
	@echo "  $(TARGET)            - Build the frame checker"
	@echo "  clean                - Remove build artifacts"
	@echo "  install              - Install to /usr/local/bin"
	@echo "  test                 - Run all predefined test frames"
	@echo "  run                  - Run with predefined frames (same as test)"
	@echo "  test-format1         - Test Format 1 example"
	@echo "  test-format2         - Test Format 2 example"
	@echo "  test-mixed           - Test mixed format"
	@echo "  test-single-digits   - Test single digit format"
	@echo "  test-datalen         - Test with specified DATA length (2 bytes)"
	@echo "  test-datalen-zero    - Test with DATA length 0"
	@echo "  test-datalen-long    - Test with longer DATA length (8 bytes)"
	@echo "  test-datalen-mismatch - Test DATA length mismatch scenario"
	@echo "  test-all-datalen     - Run all datalen tests"
	@echo "  test-all             - Run comprehensive test suite"
	@echo "  help                 - Show this help"
	@echo ""
	@echo "Manual usage:"
	@echo "  ./$(TARGET) \"your_hex_frame_data\""
	@echo "  ./$(TARGET) -datalen <length> \"your_hex_frame_data\""
	@echo "  ./$(TARGET) --help"
	@echo ""
	@echo "Examples:"
	@echo "  ./$(TARGET) \"AA 55 30 00 00 00 00 00 00 01 00 1A 2C 1D 02\""
	@echo "  ./$(TARGET) -datalen 2 \"aa 40 0 0 0 0 0 0 29 43 95 1D 02 11 22 33 44\"" 