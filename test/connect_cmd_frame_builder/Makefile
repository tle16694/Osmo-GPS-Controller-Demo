# Makefile for DJI Osmo Camera Connection Command Frame Builder
# DJI Osmo 相机连接命令帧构建器的 Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
TARGET = connect_cmd_frame_builder
SOURCE = connect_cmd_frame_builder.c

# Source files from other directories
CRC16_SRC = ../../utils/crc/custom_crc16.c
CRC32_SRC = ../../utils/crc/custom_crc32.c

# Include directories
INCLUDES = -I../../protocol -I../../utils/crc

# Object files
OBJS = connect_cmd_frame_builder.o custom_crc16.o custom_crc32.o

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "正在链接 $(TARGET)... / Linking $(TARGET)..."
	$(CC) $(CFLAGS) -o $@ $^
	@echo "构建完成: $(TARGET) / Build complete: $(TARGET)"

connect_cmd_frame_builder.o: $(SOURCE)
	@echo "正在编译 $(SOURCE)... / Compiling $(SOURCE)..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

custom_crc16.o: $(CRC16_SRC)
	@echo "正在编译 $(CRC16_SRC)... / Compiling $(CRC16_SRC)..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

custom_crc32.o: $(CRC32_SRC)
	@echo "正在编译 $(CRC32_SRC)... / Compiling $(CRC32_SRC)..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

run: $(TARGET)
	@echo "正在运行 $(TARGET)... / Running $(TARGET)..."
	@echo "======================================"
	./$(TARGET)

clean:
	@echo "正在清理... / Cleaning up..."
	rm -f $(OBJS) $(TARGET)
	@echo "清理完成。/ Clean complete."

help:
	@echo "可用目标 / Available targets:"
	@echo "  all    - 构建连接命令帧构建器 / Build the connection command frame builder"
	@echo "  run    - 构建并运行程序 / Build and run the program"
	@echo "  clean  - 移除编译文件 / Remove compiled files"
	@echo "  help   - 显示帮助信息 / Show this help message" 