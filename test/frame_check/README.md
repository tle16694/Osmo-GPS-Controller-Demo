# DJI Protocol Frame Checker / DJI 协议帧校验工具

A tool for validating DJI protocol frames with detailed CRC analysis. 
用于校验 DJI 协议帧的工具，提供详细的 CRC 分析。

## Features / 功能特性

- **Frame structure analysis** / **帧结构分析**
- **CRC-16 and CRC-32 validation** / **CRC-16 和 CRC-32 校验**
- **Flexible input formats** / **灵活的输入格式**
- **Custom DATA length specification** / **自定义 DATA 段长度指定**
- **Detailed validation output with byte codes** / **详细的校验输出和字节码显示**

## Build / 编译

```bash
cd test/frame_check
make
```

## Usage / 使用方法

### 1. Run all built-in tests / 运行所有内置测试
```bash
./frame_check
# or / 或者
./frame_check --test
```

### 2. Test custom frame / 测试自定义帧
```bash
./frame_check "aa 40 0 0 0 0 0 0 29 43 95"
./frame_check "AA,55,30,00,00,00,00,00,00,01,00,1A,2C"
```

### 3. Specify DATA segment length / 指定 DATA 段长度
```bash
./frame_check -datalen 48 "aa 40 0 0 0 0 0 0 29 43 95 2c 1d 6 1 5..."
./frame_check -datalen 0 "aa 10 0 0 0 0 0 0 29 43 95 1D 02 11 22"
```

### 4. Show help / 显示帮助
```bash
./frame_check --help
```

## Input Formats / 输入格式

The tool supports various hex input formats: 
工具支持多种十六进制输入格式：

```bash
# Space separated / 空格分隔
./frame_check "AA 55 30 00 00 00 00 00"

# Comma separated / 逗号分隔  
./frame_check "AA,55,30,00,00,00,00,00"

# Mixed format / 混合格式
./frame_check "AA 55,30 00-00:00 00 00"

# Single digits / 单字符
./frame_check "a 4 0 0 0 0 0 0"
```

## datalen Parameter / datalen 参数

Use `-datalen` to force a specific DATA segment length: 
使用 `-datalen` 强制指定 DATA 段长度：

### When to use / 何时使用
- **Incomplete frames** / **不完整的帧**
- **Debugging CRC calculation** / **调试 CRC 计算**
- **Known DATA length validation** / **已知 DATA 长度验证**

### Example / 示例
```bash
# Force 48-byte DATA segment / 强制 48 字节 DATA 段
./frame_check -datalen 48 "aa 40 0 0 0 0 0 0 29 43 95 2c 1d 6 1 5..."

# Output shows / 输出显示:
# - DATA segment: 48 bytes (specified) / DATA 段：48 字节（指定）
# - CRC-32 from last 4 bytes / CRC-32 来自最后 4 字节
# - Overlap warning if needed / 如有需要显示重叠警告
```

## Output Example / 输出示例

```
=== Frame Validation ===
✅ SOF valid: 0xAA
❌ Frame length mismatch: expected 64, got 63
✅ CRC-16 valid: 0x2C95
CRC-16 calculation bytes (0-9): AA 40 00 00 00 00 00 00 29 43
Using specified DATA length: 48 bytes
⚠️  DATA segment overlaps with CRC-32 by 1 bytes
CRC-32 calculation bytes (0-61): AA 40 00 00 ... DC BF
❌ CRC-32 mismatch: Received: 0xC7BFDC00, Calculated: 0x0000A3C7
```

## Common Issues / 常见问题

| Error / 错误 | Cause / 原因 | Solution / 解决方案 |
|--------------|--------------|---------------------|
| "Frame too short" | < 16 bytes / < 16 字节 | Provide complete frame / 提供完整帧 |
| "Invalid SOF" | First byte ≠ 0xAA / 首字节 ≠ 0xAA | Check frame start / 检查帧开始 |
| "CRC mismatch" | Calculation error / 计算错误 | Check byte codes / 检查字节码 |
| "DATA overlaps CRC-32" | Normal with -datalen / 使用 -datalen 时正常 | Expected behavior / 预期行为 |

## Return Values / 返回值

- **0**: Success / 成功
- **1**: Validation failed or parameter error / 校验失败或参数错误 