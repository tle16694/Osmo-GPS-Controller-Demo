# DJI Camera Connection Request Frame Builder
# DJI 相机连接请求帧构建器

## Overview / 概述

This tool generates connection request command frames for DJI cameras (Osmo 360 / Osmo Action 5 Pro / Osmo Action 4) that can be used with Bluetooth testing tools to trigger the camera's connection verification dialog.

此工具生成用于 DJI 相机（Osmo 360 / Osmo Action 5 Pro / Osmo Action 4）的连接请求命令帧，可与蓝牙测试工具配合使用，触发相机的连接验证对话框。

## Features / 功能特性

- Generates properly formatted DJI R SDK Protocol frames / 生成格式正确的 DJI R SDK 协议帧
- Includes CRC16 and CRC32 checksums / 包含 CRC16 和 CRC32 校验和
- Configurable device parameters / 可配置设备参数
- Detailed frame breakdown output / 详细的帧结构分解输出
- Cross-platform compilation support / 跨平台编译支持

## Build Instructions / 构建说明

### Prerequisites / 前置条件

- GCC compiler / GCC 编译器
- Make utility / Make 工具

### Available Commands / 可用命令

```bash
# Build the executable / 构建可执行文件
make

# Build and run the program / 构建并运行程序
make run

# Clean compiled files / 清理编译文件
make clean

# Show help information / 显示帮助信息
make help
```

### Build Process / 构建过程

The Makefile automatically compiles the following components:
Makefile 会自动编译以下组件：

1. **Main program** / 主程序: `connect_cmd_frame_builder.c`
2. **CRC utilities** / CRC 工具: 
   - `../../utils/crc/custom_crc16.c`
   - `../../utils/crc/custom_crc32.c`

## Usage / 使用方法

### Step 1: Generate Connection Frame / 步骤 1：生成连接帧

```bash
make run
```

This will output a hex string like:
这将输出类似以下的十六进制字符串：

```
AA330002000000000100982F00197856341206383456789ABC00000000000000000000000000000000FE1F00000000796C667C
```

### Step 2: Bluetooth Testing / 步骤 2：蓝牙测试

#### Required Setup / 必需设置

1. **Target Camera** / 目标相机: DJI Osmo Action 4 or Osmo Action 5 Pro
2. **Bluetooth Tool** / 蓝牙工具: Any BLE testing app that can write to characteristics
3. **Service UUID** / 服务 UUID: `0xFFF0`
4. **Write Characteristic** / 写入特性: `0xFFF5`
5. **Notify Characteristic** / 通知特性: `0xFFF4`

#### Testing Steps / 测试步骤

**English:**
1. Turn on your DJI camera and ensure it's in pairing mode
2. Use your Bluetooth testing tool to scan and connect to the camera
3. Navigate to service `0xFFF0`
4. Find write characteristic `0xFFF5`
5. Enable notifications on characteristic `0xFFF4`
6. Copy the generated hex frame from the tool output
7. Send the hex data to characteristic `0xFFF5`
8. **Expected Result**: Camera will display a connection verification dialog
9. Monitor characteristic `0xFFF4` for the camera's response

**中文：**
1. 打开 DJI 相机并确保其处于配对模式
2. 使用蓝牙测试工具扫描并连接到相机
3. 导航到服务 `0xFFF0`
4. 找到写入特性 `0xFFF5`
5. 在特性 `0xFFF4` 上启用通知
6. 复制工具输出的生成的十六进制帧
7. 将十六进制数据发送到特性 `0xFFF5`
8. **预期结果**：相机将显示连接验证对话框
9. 监控特性 `0xFFF4` 以获取相机的响应

## Frame Structure / 帧结构

The generated frame follows the DJI R SDK Protocol Connection Request (0019) format.
生成的帧遵循 DJI R SDK 协议的连接请求（0019）格式。

## Configuration / 配置

### Default Parameters / 默认参数

The tool uses the following default values:
工具使用以下默认值：

- **Device ID** / 设备ID: `0x12345678`
- **MAC Address** / MAC地址: `38:34:56:78:9A:BC`
- **Firmware Version** / 固件版本: `0x00000000`
- **Verify Mode** / 验证模式: `0` (No verification required / 无需验证)
- **Verify Data** / 验证数据: Random 4-digit number / 随机4位数字

### Customization / 自定义

To modify parameters, edit the `main()` function in `connect_cmd_frame_builder.c`:
要修改参数，请编辑 `connect_cmd_frame_builder.c` 中的 `main()` 函数：

```c
uint32_t device_id = 0x12345678;        // Your device ID / 您的设备ID
int8_t mac_addr[6] = {0x38, 0x34, 0x56, 0x78, 0x9A, 0xBC}; // Your MAC / 您的MAC
uint32_t fw_version = 0x00;             // Firmware version / 固件版本
uint8_t verify_mode = 0;                // Verification mode / 验证模式
```

## Expected Camera Response / 预期相机响应

When the frame is successfully sent to `0xFFF5`:
当帧成功发送到 `0xFFF5` 时：

1. **Camera displays verification dialog** / 相机显示验证对话框
2. **User confirms on camera** / 用户在相机上确认
3. **Camera sends response on `0xFFF4`** / 相机在 `0xFFF4` 上发送响应

The response will contain the connection result and any additional handshake data.
响应将包含连接结果和任何额外的握手数据。

## Troubleshooting / 故障排除

### Common Issues / 常见问题

**Build Errors / 构建错误:**
- Ensure GCC is installed / 确保已安装 GCC
- Check that CRC source files exist in `../../utils/crc/` / 检查 CRC 源文件是否存在于 `../../utils/crc/`

**No Camera Response / 相机无响应:**
- Verify camera is in pairing mode / 验证相机处于配对模式
- Check Bluetooth connection is established / 检查蓝牙连接是否已建立
- Ensure notifications are enabled on `0xFFF4` / 确保在 `0xFFF4` 上启用了通知
- Verify the hex data was sent correctly / 验证十六进制数据是否正确发送

**Connection Rejected / 连接被拒绝:**
- Camera may have pairing restrictions / 相机可能有配对限制
- Try different verify_mode values / 尝试不同的 verify_mode 值
- Reset camera's Bluetooth settings / 重置相机的蓝牙设置

## License / 许可证

This tool is part of the [Osmo-GPS-Controller-Demo](https://github.com/dji-sdk/Osmo-GPS-Controller-Demo).
此工具是 [Osmo-GPS-Controller-Demo](https://github.com/dji-sdk/Osmo-GPS-Controller-Demo) 的一部分。

Copyright (C) 2025 SZ DJI Technology Co., Ltd.