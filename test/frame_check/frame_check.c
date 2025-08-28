/* SPDX-License-Identifier: MIT */
/*
 * Copyright (C) 2025 SZ DJI Technology Co., Ltd.
 *  
 * All information contained herein is, and remains, the property of DJI.
 * The intellectual and technical concepts contained herein are proprietary
 * to DJI and may be covered by U.S. and foreign patents, patents in process,
 * and protected by trade secret or copyright law.  Dissemination of this
 * information, including but not limited to data and other proprietary
 * material(s) incorporated within the information, in any form, is strictly
 * prohibited without the express written consent of DJI.
 *
 * If you receive this source code without DJI's authorization, you may not
 * further disseminate the information, and you must immediately remove the
 * source code and notify DJI of its removal. DJI reserves the right to pursue
 * legal actions against you for any loss(es) or damage(s) caused by your
 * failure to do so.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Include CRC calculation functions
#include "../../utils/crc/custom_crc16.h"
#include "../../utils/crc/custom_crc32.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// 预定义的测试帧数据
// Predefined test frame data
static const char* test_frames[] = {
    // 格式1 示例
    // Format 1 example
    "aa 40 0 0 0 0 0 0 29 43 95",
    
    // 格式2 示例  
    // Format 2 example
    "AA, 38, 00, 01, 00, 00, 00, 00, 71, D5, 3C, 40, 1D, 02, 3C, 01, 0E, 03, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 74, 25, 01, 00, 00, 00, 00, 00, BA, 16, 00, 00, 00, 00, 00, 00",
    
    // 其他测试帧
    // Other test frames
    "AA 55 1E 00 02 00 00 00 00 01 00 1A 2C 1D 02 00 00 00 00 00 00 00 00 00 00 11 22 33 44",
    
    // 无效帧示例
    // Invalid frame examples
    "AB 55 30 00 00 00 00 00 00 01 00 1A 2C 1D 02",  // 无效SOF
    "AA 55 30 00 00 00 00 00 00 01 00 FF FF 1D 02",  // 无效CRC
};

static const int num_test_frames = sizeof(test_frames) / sizeof(test_frames[0]);

/**
 * @brief Frame validation result structure
 *        帧校验结果结构体
 */
typedef struct {
    bool overall_valid;         // Overall validation result
                               // 整体校验结果
    bool sof_valid;            // SOF validation result
                               // SOF 校验结果
    bool length_valid;         // Length validation result
                               // 长度校验结果
    bool crc16_valid;          // CRC-16 validation result
                               // CRC-16 校验结果
    bool crc32_valid;          // CRC-32 validation result
                               // CRC-32 校验结果
    
    // Detailed error information
    // 详细错误信息
    uint8_t received_sof;      // Received SOF value
                               // 接收到的 SOF 值
    uint16_t expected_length;  // Expected frame length
                               // 期望的帧长度
    uint16_t actual_length;    // Actual frame length
                               // 实际帧长度
    uint16_t received_crc16;   // Received CRC-16
                               // 接收到的 CRC-16
    uint16_t calculated_crc16; // Calculated CRC-16
                               // 计算得到的 CRC-16
    uint32_t received_crc32;   // Received CRC-32
                               // 接收到的 CRC-32
    uint32_t calculated_crc32; // Calculated CRC-32
                               // 计算得到的 CRC-32
} frame_validation_result_t;

/**
 * @brief Print frame data in hex format
 *        以十六进制格式打印帧数据
 * 
 * @param frame Frame data
 *              帧数据
 * @param length Frame length
 *               帧长度
 */
void print_frame_hex(const uint8_t *frame, size_t length) {
    printf(ANSI_COLOR_CYAN "Frame Data (%zu bytes): " ANSI_COLOR_RESET, length);
    for (size_t i = 0; i < length; i++) {
        printf("%02X", frame[i]);
        if (i < length - 1) {
            printf(" ");
        }
        if ((i + 1) % 16 == 0 && i < length - 1) {
            printf("\n                        ");
        }
    }
    printf("\n");
}

/**
 * @brief Parse frame structure from raw data
 *        从原始数据解析帧结构
 * 
 * @param frame Frame data
 *              帧数据
 * @param length Frame length
 *               帧长度
 * @param expected_data_length Expected DATA segment length (-1 for auto detection)
 *                             预期的 DATA 段长度（-1 表示自动检测）
 */
void parse_frame_structure(const uint8_t *frame, size_t length, int expected_data_length) {
    if (length < 16) {
        printf(ANSI_COLOR_RED "Frame too short for basic structure (minimum 16 bytes required)\n" ANSI_COLOR_RESET);
        return;
    }
    
    printf(ANSI_COLOR_BLUE "Frame Structure Analysis:\n" ANSI_COLOR_RESET);
    printf("  SOF:        0x%02X\n", frame[0]);
    
    uint16_t ver_length = (frame[2] << 8) | frame[1];
    uint16_t version = ver_length >> 10;
    uint16_t frame_len = ver_length & 0x03FF;
    printf("  Ver/Length: 0x%04X (Version: %u, Length: %u)\n", ver_length, version, frame_len);
    
    printf("  CmdType:    0x%02X\n", frame[3]);
    printf("  ENC:        0x%02X\n", frame[4]);
    printf("  RES:        0x%02X 0x%02X 0x%02X\n", frame[5], frame[6], frame[7]);
    
    uint16_t seq = (frame[9] << 8) | frame[8];
    printf("  SEQ:        0x%04X (%u)\n", seq, seq);
    
    uint16_t crc16 = (frame[11] << 8) | frame[10];
    printf("  CRC-16:     0x%04X\n", crc16);
    
    if (length > 16) {
        size_t data_start = 12;
        size_t data_end, data_len;
        
        if (expected_data_length >= 0) {
            // Use specified DATA length
            // 使用指定的DATA长度
            data_len = (size_t)expected_data_length;
            data_end = data_start + data_len;
            printf("  Data:       ");
            
            // Print DATA segment with specified length
            // 按指定长度打印DATA段
            for (size_t i = data_start; i < data_end && i < length; i++) {
                printf("%02X ", frame[i]);
                if ((i - data_start + 1) % 16 == 0 && i < data_end - 1) {
                    printf("\n              ");
                }
            }
            printf("(%zu bytes, specified length)\n", data_len);
            
            // Check if DATA extends beyond frame
            // 检查DATA是否超出帧长度
            if (data_end > length) {
                printf("  " ANSI_COLOR_YELLOW "⚠️  DATA segment extends %zu bytes beyond frame end\n" ANSI_COLOR_RESET, 
                       data_end - length);
            }
        } else {
            // Auto-detect DATA length: from after CRC-16 (byte 12) to before CRC-32 (last 4 bytes)
            // 自动检测DATA长度：从CRC-16之后（第12字节）到CRC-32之前（最后4字节）
            data_end = length - 4;
            data_len = data_end - data_start;
            
            printf("  Data:       ");
            for (size_t i = data_start; i < data_end; i++) {
                printf("%02X ", frame[i]);
                if ((i - data_start + 1) % 16 == 0 && i < data_end - 1) {
                    printf("\n              ");
                }
            }
            printf("(%zu bytes, auto-detected)\n", data_len);
        }
    }
    
    if (length >= 4) {
        uint32_t crc32 = (frame[length-1] << 24) | (frame[length-2] << 16) | 
                        (frame[length-3] << 8) | frame[length-4];
        printf("  CRC-32:     0x%08X (from last 4 bytes: %zu-%zu)\n", 
               crc32, length-4, length-1);
    }
    printf("\n");
}

/**
 * @brief Validate protocol frame
 *        校验协议帧
 * 
 * @param frame Frame data
 *              帧数据
 * @param length Frame length
 *               帧长度
 * @param result Validation result structure
 *               校验结果结构体
 * @param expected_data_length Expected DATA segment length (-1 for auto detection)
 *                             预期的 DATA 段长度（-1 表示自动检测）
 * @return true if frame is valid, false otherwise
 *         帧有效返回 true，否则返回 false
 */
bool validate_frame(const uint8_t *frame, size_t length, frame_validation_result_t *result, int expected_data_length) {
    // Initialize result structure
    // 初始化结果结构体
    memset(result, 0, sizeof(frame_validation_result_t));
    result->actual_length = length;
    
    // Check minimum frame length
    // 检查最小帧长度
    if (length < 16) {
        printf(ANSI_COLOR_RED "❌ Frame too short: %zu bytes (minimum 16 bytes required)\n" ANSI_COLOR_RESET, length);
        return false;
    }
    
    // Validate SOF (Start of Frame)
    // 校验帧头 (SOF)
    result->received_sof = frame[0];
    result->sof_valid = (frame[0] == 0xAA);
    if (!result->sof_valid) {
        printf(ANSI_COLOR_RED "❌ Invalid SOF: 0x%02X (expected 0xAA)\n" ANSI_COLOR_RESET, frame[0]);
    } else {
        printf(ANSI_COLOR_GREEN "✅ SOF valid: 0x%02X\n" ANSI_COLOR_RESET, frame[0]);
    }
    
    // Validate frame length
    // 校验帧长度
    uint16_t ver_length = (frame[2] << 8) | frame[1];
    result->expected_length = ver_length & 0x03FF;
    result->length_valid = (result->expected_length == length);
    if (!result->length_valid) {
        printf(ANSI_COLOR_RED "❌ Frame length mismatch: expected %u, got %zu\n" ANSI_COLOR_RESET, 
               result->expected_length, length);
    } else {
        printf(ANSI_COLOR_GREEN "✅ Frame length valid: %zu bytes\n" ANSI_COLOR_RESET, length);
    }
    
    // Validate CRC-16 (covers from SOF to SEQ, bytes 0-9)
    // 校验 CRC-16（覆盖从 SOF 到 SEQ，第 0-9 字节）
    result->received_crc16 = (frame[11] << 8) | frame[10];
    result->calculated_crc16 = calculate_crc16(frame, 10);
    result->crc16_valid = (result->received_crc16 == result->calculated_crc16);
    
    // Print the bytes used for CRC-16 calculation
    // 打印用于CRC-16计算的字节
    printf(ANSI_COLOR_CYAN "CRC-16 calculation bytes (0-9): " ANSI_COLOR_RESET);
    for (int i = 0; i < 10; i++) {
        printf("%02X", frame[i]);
        if (i < 9) printf(" ");
    }
    printf("\n");
    
    if (!result->crc16_valid) {
        printf(ANSI_COLOR_RED "❌ CRC-16 mismatch:\n" ANSI_COLOR_RESET);
        printf("   Received:   0x%04X\n", result->received_crc16);
        printf("   Calculated: 0x%04X\n", result->calculated_crc16);
        printf("   " ANSI_COLOR_YELLOW "CRC-16 covers bytes 0-9 (SOF to SEQ)" ANSI_COLOR_RESET "\n");
    } else {
        printf(ANSI_COLOR_GREEN "✅ CRC-16 valid: 0x%04X\n" ANSI_COLOR_RESET, result->received_crc16);
    }
    
    // Validate CRC-32 (covers from SOF to DATA, excluding last 4 bytes)
    // 校验 CRC-32（覆盖从 SOF 到 DATA，排除最后 4 字节）
    if (expected_data_length >= 0) {
        // Use specified DATA length for CRC-32 calculation
        // 使用指定的 DATA 长度进行 CRC-32 计算
        size_t expected_data_length_uz = (size_t)expected_data_length; // Convert to size_t to avoid sign comparison warnings
        size_t min_required_length = 12 + 2 + expected_data_length_uz + 4; // Header(12) + CRC16(2) + DATA + CRC32(4)
        
        printf(ANSI_COLOR_YELLOW "Using specified DATA length: %d bytes\n" ANSI_COLOR_RESET, expected_data_length);
        printf(ANSI_COLOR_YELLOW "Minimum required frame length: %zu bytes\n" ANSI_COLOR_RESET, min_required_length);
        
        // Check if we have enough data for the specified DATA length
        // 检查是否有足够的数据用于指定的DATA长度
        size_t min_data_length = 12 + 2 + expected_data_length_uz; // Header + CRC16 + specified DATA
        
        if (length >= min_data_length) {
            // Force use the specified DATA length, even if it overlaps with CRC-32
            // 强制使用指定的DATA长度，即使与CRC-32重叠
            size_t crc32_coverage_length = min_data_length; // Header + CRC16 + specified DATA length
            
            printf(ANSI_COLOR_YELLOW "Using specified DATA length: %d bytes\n" ANSI_COLOR_RESET, expected_data_length);
            
            // Always extract CRC-32 from the last 4 bytes of the frame
            // 总是从帧的最后4字节提取CRC-32
            if (length >= 4) {
                result->received_crc32 = (frame[length-1] << 24) | (frame[length-2] << 16) | 
                                        (frame[length-3] << 8) | frame[length-4];
                printf(ANSI_COLOR_YELLOW "CRC-32 extracted from last 4 bytes (bytes %zu-%zu)\n" ANSI_COLOR_RESET, 
                       length-4, length-1);
                
                // Check if DATA and CRC-32 overlap
                // 检查DATA和CRC-32是否重叠
                size_t data_end_pos = min_data_length; // Position after DATA segment
                size_t crc32_start_pos = length - 4;   // Position of CRC-32 start
                
                if (data_end_pos > crc32_start_pos) {
                    size_t overlap_bytes = data_end_pos - crc32_start_pos;
                    printf(ANSI_COLOR_YELLOW "⚠️  DATA segment overlaps with CRC-32 by %zu bytes\n" ANSI_COLOR_RESET, 
                           overlap_bytes);
                    printf(ANSI_COLOR_YELLOW "    DATA covers bytes 12-%zu, CRC-32 at bytes %zu-%zu\n" ANSI_COLOR_RESET, 
                           data_end_pos - 1, crc32_start_pos, length - 1);
                }
            } else {
                result->received_crc32 = 0;
                printf(ANSI_COLOR_RED "❌ Frame too short to extract CRC-32\n" ANSI_COLOR_RESET);
            }
            
            result->calculated_crc32 = calculate_crc32(frame, crc32_coverage_length);
            result->crc32_valid = (result->received_crc32 == result->calculated_crc32);
            
            // Print the bytes used for CRC-32 calculation
            // 打印用于CRC-32计算的字节
            printf(ANSI_COLOR_CYAN "CRC-32 calculation bytes (0-%zu): " ANSI_COLOR_RESET, crc32_coverage_length - 1);
            for (size_t i = 0; i < crc32_coverage_length && i < length; i++) {
                printf("%02X", frame[i]);
                if (i < crc32_coverage_length - 1) printf(" ");
                if ((i + 1) % 16 == 0 && i < crc32_coverage_length - 1) {
                    printf("\n                                  ");
                }
            }
            printf("\n");
            
            if (!result->crc32_valid) {
                printf(ANSI_COLOR_RED "❌ CRC-32 mismatch (using specified DATA length):\n" ANSI_COLOR_RESET);
                printf("   Received:   0x%08X (from last 4 bytes)\n", result->received_crc32);
                printf("   Calculated: 0x%08X\n", result->calculated_crc32);
                printf("   " ANSI_COLOR_YELLOW "CRC-32 covers bytes 0-%zu (Header + CRC16 + DATA[%d bytes])" ANSI_COLOR_RESET "\n", 
                       crc32_coverage_length - 1, expected_data_length);
            } else {
                printf(ANSI_COLOR_GREEN "✅ CRC-32 valid (using specified DATA length %d): 0x%08X\n" ANSI_COLOR_RESET, 
                       expected_data_length, result->received_crc32);
            }
            
            // Check frame length consistency
            // 检查帧长度一致性
            if (length < min_required_length) {
                printf(ANSI_COLOR_YELLOW "⚠️  Frame shorter than expected: got %zu bytes, expected %zu bytes\n" ANSI_COLOR_RESET, 
                       length, min_required_length);
            } else if (length > min_required_length) {
                printf(ANSI_COLOR_YELLOW "⚠️  Frame longer than expected: got %zu bytes, expected %zu bytes\n" ANSI_COLOR_RESET, 
                       length, min_required_length);
            }
        } else {
            result->crc32_valid = false;
            printf(ANSI_COLOR_RED "❌ Frame too short for specified DATA length: got %zu bytes, need at least %zu bytes for DATA[%d]\n" ANSI_COLOR_RESET, 
                   length, min_data_length, expected_data_length);
        }
    } else {
        // Auto-detect based on actual frame length: Data = frame_length - Header(12) - CRC16(2) - CRC32(4)
        // 基于实际帧长度自动检测：Data = 帧长度 - 帧头(12) - CRC16(2) - CRC32(4)
        if (length >= 16) {
            size_t data_length = length - 16; // Total frame - Header(12) - CRC16(2) - CRC32(4)
            size_t crc32_coverage_length = length - 4; // Everything except CRC-32
            
            result->received_crc32 = (frame[length-1] << 24) | (frame[length-2] << 16) | 
                                    (frame[length-3] << 8) | frame[length-4];
            result->calculated_crc32 = calculate_crc32(frame, crc32_coverage_length);
            result->crc32_valid = (result->received_crc32 == result->calculated_crc32);
            
            // Print the bytes used for CRC-32 calculation (auto-detect mode)
            // 打印用于CRC-32计算的字节（自动检测模式）
            printf(ANSI_COLOR_CYAN "CRC-32 calculation bytes (0-%zu): " ANSI_COLOR_RESET, crc32_coverage_length - 1);
            for (size_t i = 0; i < crc32_coverage_length && i < length; i++) {
                printf("%02X", frame[i]);
                if (i < crc32_coverage_length - 1) printf(" ");
                if ((i + 1) % 16 == 0 && i < crc32_coverage_length - 1) {
                    printf("\n                                  ");
                }
            }
            printf("\n");
            
            if (!result->crc32_valid) {
                printf(ANSI_COLOR_RED "❌ CRC-32 mismatch:\n" ANSI_COLOR_RESET);
                printf("   Received:   0x%08X\n", result->received_crc32);
                printf("   Calculated: 0x%08X\n", result->calculated_crc32);
                printf("   " ANSI_COLOR_YELLOW "CRC-32 covers bytes 0-%zu (Header + CRC16 + DATA[%zu bytes])" ANSI_COLOR_RESET "\n", 
                       crc32_coverage_length - 1, data_length);
            } else {
                printf(ANSI_COLOR_GREEN "✅ CRC-32 valid: 0x%08X\n" ANSI_COLOR_RESET, result->received_crc32);
            }
        } else {
            result->crc32_valid = false;
            printf(ANSI_COLOR_RED "❌ Frame too short for CRC-32 validation (minimum 16 bytes required)\n" ANSI_COLOR_RESET);
        }
    }
    
    // Overall validation result
    // 整体校验结果
    result->overall_valid = result->sof_valid && result->length_valid && 
                           result->crc16_valid && result->crc32_valid;
    
    return result->overall_valid;
}

/**
 * @brief 通用的十六进制字符串解析函数
 *        Universal hex string parsing function
 * 
 * 支持以下特性：
 * - 忽略大小写
 * - 去除字母和数字以外的标点符号
 * - 保留空格分隔
 * - 0 和 00 都算作 0
 * - 支持单个字符或双字符十六进制数
 * 
 * Features supported:
 * - Case insensitive
 * - Remove punctuation except letters and numbers
 * - Keep space separation
 * - Both 0 and 00 count as 0
 * - Support single or double character hex numbers
 * 
 * @param hex_string Input hex string (various formats)
 *                   输入的十六进制字符串（多种格式）
 * @param output Output byte array
 *               输出字节数组
 * @param max_length Maximum output length
 *                   最大输出长度
 * @return Number of bytes parsed
 *         解析的字节数
 */
size_t parse_hex_string_universal(const char *hex_string, uint8_t *output, size_t max_length) {
    size_t count = 0;
    const char *ptr = hex_string;
    char current_hex[3] = {0}; // 当前十六进制数字缓冲区
    int hex_char_count = 0;    // 当前十六进制数字的字符数
    
    // printf(ANSI_COLOR_YELLOW "Parsing input: %s\n" ANSI_COLOR_RESET, hex_string);
    
    while (*ptr && count < max_length) {
        char c = *ptr;
        
        // 跳过非十六进制字符（除了字母和数字）
        // Skip non-hex characters (except letters and numbers)
        if (!isxdigit(c)) {
            // 如果当前有未完成的十六进制数字，先处理它
            // If there's an incomplete hex number, process it first
            if (hex_char_count > 0) {
                // 转换当前十六进制数字
                // Convert current hex number
                char *endptr;
                long value = strtol(current_hex, &endptr, 16);
                if (endptr > current_hex) {
                    output[count++] = (uint8_t)value;
                    // printf("  Parsed: \"%s\" -> 0x%02X\n", current_hex, (uint8_t)value);
                }
                hex_char_count = 0;
                memset(current_hex, 0, sizeof(current_hex));
            }
            ptr++;
            continue;
        }
        
        // 转换为大写
        // Convert to uppercase
        c = toupper(c);
        
        // 添加到当前十六进制数字缓冲区
        // Add to current hex number buffer
        if (hex_char_count < 2) {
            current_hex[hex_char_count++] = c;
        }
        
        // 如果达到2个字符或者下一个字符不是十六进制字符，处理当前数字
        // If reached 2 characters or next character is not hex, process current number
        if (hex_char_count == 2 || !isxdigit(*(ptr + 1))) {
            char *endptr;
            long value = strtol(current_hex, &endptr, 16);
            if (endptr > current_hex) {
                output[count++] = (uint8_t)value;
                // printf("  Parsed: \"%s\" -> 0x%02X\n", current_hex, (uint8_t)value);
            }
            hex_char_count = 0;
            memset(current_hex, 0, sizeof(current_hex));
        }
        
        ptr++;
    }
    
    // 处理最后可能剩余的十六进制数字
    // Process any remaining hex number
    if (hex_char_count > 0 && count < max_length) {
        char *endptr;
        long value = strtol(current_hex, &endptr, 16);
        if (endptr > current_hex) {
            output[count++] = (uint8_t)value;
            // printf("  Parsed: \"%s\" -> 0x%02X\n", current_hex, (uint8_t)value);
        }
    }
    
    printf("Total parsed: %zu bytes\n\n", count);
    return count;
}

/**
 * @brief Print validation summary
 *        打印校验摘要
 * 
 * @param result Validation result
 *               校验结果
 */
void print_validation_summary(const frame_validation_result_t *result) {
    printf("\n" ANSI_COLOR_MAGENTA "=== Validation Summary ===" ANSI_COLOR_RESET "\n");
    
    if (result->overall_valid) {
        printf(ANSI_COLOR_GREEN "🎉 Frame is VALID - All checks passed!\n" ANSI_COLOR_RESET);
    } else {
        printf(ANSI_COLOR_RED "❌ Frame is INVALID - Found issues:\n" ANSI_COLOR_RESET);
        
        if (!result->sof_valid) {
            printf("   • SOF error: got 0x%02X, expected 0xAA\n", result->received_sof);
        }
        
        if (!result->length_valid) {
            printf("   • Length error: frame declares %u bytes, but actual length is %u bytes\n",
                   result->expected_length, result->actual_length);
        }
        
        if (!result->crc16_valid) {
            printf("   • CRC-16 error: got 0x%04X, calculated 0x%04X\n",
                   result->received_crc16, result->calculated_crc16);
            printf("     (CRC-16 should cover bytes 0-9: SOF to SEQ)\n");
        }
        
        if (!result->crc32_valid) {
            printf("   • CRC-32 error: got 0x%08X, calculated 0x%08X\n",
                   result->received_crc32, result->calculated_crc32);
            printf("     (CRC-32 should cover all bytes except the last 4 CRC-32 bytes)\n");
        }
    }
    printf("\n");
}

/**
 * @brief Test a single frame
 *        测试单个帧
 * 
 * @param frame_str Frame string
 *                  帧字符串
 * @param frame_index Frame index for display
 *                    用于显示的帧索引
 * @param expected_data_length Expected DATA segment length (-1 for auto detection)
 *                             预期的 DATA 段长度（-1 表示自动检测）
 */
void test_frame(const char *frame_str, int frame_index, int expected_data_length) {
    printf(ANSI_COLOR_CYAN "=== Testing Frame %d ===" ANSI_COLOR_RESET "\n", frame_index + 1);
    printf("Input: %s\n", frame_str);
    if (expected_data_length >= 0) {
        printf("Expected DATA length: %d bytes\n", expected_data_length);
    }
    printf("\n");
    
    // Parse hex string
    // 解析十六进制字符串
    uint8_t frame_data[1024];
    size_t frame_length = parse_hex_string_universal(frame_str, frame_data, sizeof(frame_data));
    
    if (frame_length == 0) {
        printf(ANSI_COLOR_RED "Failed to parse hex string\n" ANSI_COLOR_RESET);
        return;
    }
    
    // Print frame data
    // 打印帧数据
    print_frame_hex(frame_data, frame_length);
    printf("\n");
    
    // Parse frame structure
    // 解析帧结构
    parse_frame_structure(frame_data, frame_length, expected_data_length);
    
    // Validate frame
    // 校验帧
    printf(ANSI_COLOR_BLUE "=== Frame Validation ===" ANSI_COLOR_RESET "\n");
    frame_validation_result_t result;
    validate_frame(frame_data, frame_length, &result, expected_data_length);
    
    // Print summary
    // 打印摘要
    print_validation_summary(&result);
    
    printf("\n" ANSI_COLOR_CYAN "================================================" ANSI_COLOR_RESET "\n\n");
}

/**
 * @brief Show usage information
 *        显示使用说明
 */
void show_usage(const char *program_name) {
    printf("Usage: %s [options] [hex_frame_data]\n\n", program_name);
    printf("Options:\n");
    printf("  -t, --test         Run all predefined test frames\n");
    printf("  -h, --help         Show this help message\n");
    printf("  -datalen <length>  Specify expected DATA segment length for CRC-32 calculation\n");
    printf("                     指定 DATA 段预期长度用于 CRC-32 校验\n\n");
    printf("Examples:\n");
    printf("  %s \"AA 55 30 00 00 00 00 00 00 01 00 1A 2C 1D 02\"\n", program_name);
    printf("  %s \"AA,55,30,00,00,00,00,00,00,01,00,1A,2C,1D,02\"\n", program_name);
    printf("  %s \"aa 40 0 0 0 0 0 0 29 43 95\"\n", program_name);
    printf("  %s -datalen 2 \"aa 40 0 0 0 0 0 0 29 43 95 1D 02 11 22 33 44\"\n", program_name);
    printf("  %s --test\n", program_name);
    printf("\nSupported formats:\n");
    printf("  - Space separated: \"AA 55 30 00\"\n");
    printf("  - Comma separated: \"AA,55,30,00\"\n");
    printf("  - Mixed format: \"AA 55,30 00\"\n");
    printf("  - Single digits: \"a 4 0 0\"\n");
    printf("  - Case insensitive: \"aa\" or \"AA\"\n");
}

int main(int argc, char *argv[]) {
    printf(ANSI_COLOR_CYAN "=== DJI Protocol Frame Checker ===" ANSI_COLOR_RESET "\n\n");
    
    int expected_data_length = -1;  // Default: auto-detect
    const char *frame_data = NULL;
    
    // Parse command line arguments
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
            printf("Running all predefined test frames...\n\n");
            for (int j = 0; j < num_test_frames; j++) {
                test_frame(test_frames[j], j, -1);
            }
            return 0;
        } else if (strcmp(argv[i], "-datalen") == 0) {
            if (i + 1 < argc) {
                expected_data_length = atoi(argv[i + 1]);
                if (expected_data_length < 0) {
                    printf(ANSI_COLOR_RED "Error: DATA length must be non-negative\n\n" ANSI_COLOR_RESET);
                    show_usage(argv[0]);
                    return 1;
                }
                i++; // Skip the next argument as it's the length value
            } else {
                printf(ANSI_COLOR_RED "Error: -datalen requires a length argument\n\n" ANSI_COLOR_RESET);
                show_usage(argv[0]);
                return 1;
            }
        } else {
            // Treat as frame data
            // 作为帧数据处理
            if (frame_data == NULL) {
                frame_data = argv[i];
            } else {
                printf(ANSI_COLOR_RED "Error: Multiple frame data arguments not supported\n\n" ANSI_COLOR_RESET);
                show_usage(argv[0]);
                return 1;
            }
        }
    }
    
    // Handle different cases
    // 处理不同情况
    if (argc == 1) {
        // No arguments, run all test frames
        // 没有参数，运行所有测试帧
        printf("No arguments provided. Running all predefined test frames...\n\n");
        for (int i = 0; i < num_test_frames; i++) {
            test_frame(test_frames[i], i, -1);
        }
        return 0;
    }
    
    if (frame_data != NULL) {
        // Test the provided frame data
        // 测试提供的帧数据
        test_frame(frame_data, 0, expected_data_length);
        return 0;
    }
    
    // If we get here, there were only options but no frame data
    // 如果执行到这里，说明只有选项但没有帧数据
    printf(ANSI_COLOR_RED "Error: No frame data provided\n\n" ANSI_COLOR_RESET);
    show_usage(argv[0]);
    return 1;
} 
