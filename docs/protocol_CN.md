# 协议解析说明文档

## 什么是 DJI R SDK 协议

DJI R SDK 协议是一套简单易用且稳定可靠的通信协议。第三方可通过 DJI R SDK 协议控制手持设备（例如 DJI Osmo 360、DJI Osmo Action 5 Pro 和 DJI Osmo Action 4），并从中获取部分信息。在 DJI R SDK 协议的支持下，手持设备的扩展性得到了提升，也拥有了更丰富的应用场景。

DJI R SDK 帧结构如下所示：

<img title="DJI R SDK Protocol" src="./images/dji_r_sdk_protocol.png" alt="DJI R SDK Protocol" data-align="center" width="711">

其中，CRC-16 的值是对 SOF 到 SEQ 段进行 CRC16 校验后的结果，CRC-32 的值是对 SOF 到 DATA 段进行 CRC32 校验后的结果。CRC 实现参考 Demo 文件：`custom_crc32.c` 和 `custom_crc16.c`。

| 区域       | 偏移 | 大小 | 描述                                                         |
| ---------- | ---- | ---- | ------------------------------------------------------------ |
| SOF        | 0    | 1    | 帧头固定为 0xAA                                              |
| Ver/Length | 1    | 2    | [15:10] - 版本号 默认值 0<br>[9:0] - 整个帧的长度<br>注：LSB in first |
| CmdType    | 3    | 1    | [4:0] - 应答类型<br>0 - 数据发送之后不需要应答<br>1 - 数据发送之后需要应答，但是不应答也没关系<br>2-31 - 数据发送之后必须要应答<br>[5] - 帧类型<br>0 - 命令帧<br>1 - 应答帧<br>[7:6] - 保留 默认值 0 |
| ENC        | 4    | 1    | [4:0] - 加密时的补充字节长度（加密必须 16 字节对齐）<br/>[7:5] - 加密类型<br/>0 - 不加密<br/>1 - AES256 加密 |
| RES        | 5    | 3    | 保留字节段                                                   |
| SEQ        | 8    | 2    | 序列号                                                       |
| CRC-16     | 10   | 2    | 帧校验（SOF 到 SEQ）                                         |
| DATA       | 12   | n    | **DATA 数据段**，见下文详细描述                              |
| CRC-32     | n+12 | 4    | 帧校验（SOF 到 DATA）                                        |

注意：整个数据包采用**小端存储**方式。

## DATA 数据段

本程序的核心在于封装和解析 DATA 数据段。虽然 DATA 数据段的长度不固定，但开头的两个字节始终为 CmdSet 和 CmdID，每组 (CmdSet, CmdID) 可以确定一种功能。DATA 数据段（偏移 12，大小为 n）结构如下所示：

```
|  CmdSet  |  CmdID   |     Data Payload   |
|----------|----------|--------------------|
|  1-byte  |  1-byte  |     (n-2) byte     |
```

Data Payload 通常放入的是 **命令帧** 或者 **应答帧**。

详细的 DATA 数据段说明请参阅：[DATA 数据段详细文档](./protocol_data_segment_CN.md)

## 交互流程

<img title="Frame Interaction Process" src="./images/frame_interaction_process.png" alt="Frame interaction process" data-align="center" width="290">

注意：发送端为遥控器时，接收端为相机；发送端为相机时，接收端为遥控器。

如上图所示，在一次收发协议的过程中，发送方会确定 SEQ（序列号）。如果接收方需要回复，则必须回复相同的 SEQ。协议中的 CmdType [4:0] 字段指示是否需要回复该帧。

构造 DJI R SDK 发送帧的示例可以参考 [DATA 数据段详细文档](./protocol_data_segment_CN.md) 中的 **统一模式切换** 功能。

## 协议层

协议层是本程序为方便解析协议帧，单独抽离出来的一个模块。文件结构如下：

```
protocol/
├── dji_protocol_data_descriptors.c
├── dji_protocol_data_descriptors.h
├── dji_protocol_data_processor.c
├── dji_protocol_data_processor.h
├── dji_protocol_data_structures.c
├── dji_protocol_data_structures.h
├── dji_protocol_parser.c
└── dji_protocol_parser.h
```

- **dji_protocol_parser**：负责 DJI R SDK 协议帧的封装与解析。
- **dji_protocol_data_processor**：负责 DATA 段的封装与解析。
- **dji_protocol_data_descriptors**：为每个功能定义三元组 (CmdSet, CmdID) -> creator -> parser，以便进行功能扩展。
- **dji_protocol_data_structures**：为命令帧和应答帧定义结构体。

<img title="Protocol Layer Sequence Diagram" src="images/sequence_diagram_of_protocol_layer.png" alt="Protocol Layer Sequence Diagram" data-align="center" width="800">

上述图片展示了协议层如何解析 DJI R SDK 帧，组装过程也是类似的。

在 DJI R SDK 协议不变的情况下，您无需修改 `dji_protocol_parser`。同样，`dji_protocol_data_processor` 也无需修改，因为它调用的是 `dji_protocol_data_descriptors` 中定义的通用 `creator` 和 `parser` 方法：

```c
typedef uint8_t* (*data_creator_func_t)(const void *structure, size_t *data_length, uint8_t cmd_type);
typedef int (*data_parser_func_t)(const uint8_t *data, size_t data_length, void *structure_out, uint8_t cmd_type);
```

因此，新增功能的解析时，只需在 `dji_protocol_data_structures` 中定义帧结构体，在 `dji_protocol_data_descriptors` 中新增对应的 `creator` 和 `parser` 函数，并将其加入到 `data_descriptors` 三元组中即可。

以下是本程序已支持的命令功能：

```c
/* 结构体支持，但要为每个结构体定义 creator 和 parser */
const data_descriptor_t data_descriptors[] = {
    // 拍摄模式切换
    {0x1D, 0x04, (data_creator_func_t)camera_mode_switch_creator, (data_parser_func_t)camera_mode_switch_parser},
    // 版本号查询
    {0x00, 0x00, NULL, (data_parser_func_t)version_query_parser},
    // 拍录控制
    {0x1D, 0x03, (data_creator_func_t)record_control_creator, (data_parser_func_t)record_control_parser},
    // GPS 数据推送
    {0x00, 0x17, (data_creator_func_t)gps_data_creator, (data_parser_func_t)gps_data_parser},
    // 连接请求
    {0x00, 0x19, (data_creator_func_t)connection_data_creator, (data_parser_func_t)connection_data_parser},
    // 相机状态订阅
    {0x1D, 0x05, (data_creator_func_t)camera_status_subscription_creator, NULL},
    // 相机状态推送
    {0x1D, 0x02, NULL, (data_parser_func_t)camera_status_push_data_parser},
    // 按键上报
    {0x00, 0x11, (data_creator_func_t)key_report_creator, (data_parser_func_t)key_report_parser},
};
```

