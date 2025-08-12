# Protocol Parsing Documentation

## What is the DJI R SDK Protocol?

The DJI R SDK protocol is a simple, easy-to-use, and stable communication protocol. Third-party developers can use the DJI R SDK protocol to control handheld devices (such as the DJI Osmo 360, DJI Osmo Action 5 Pro and DJI Osmo Action 4) and retrieve certain information from them. With the support of the DJI R SDK protocol, the expandability of handheld devices has been enhanced, providing a wider range of application scenarios.

The frame structure of the DJI R SDK protocol is as follows:

<img title="DJI R SDK Protocol" src="./images/dji_r_sdk_protocol.png" alt="DJI R SDK Protocol" data-align="center" width="711">

Here, the CRC-16 value is the result of performing CRC16 checksum on the segment from SOF to SEQ, and the CRC-32 value is the result of performing CRC32 checksum on the segment from SOF to DATA. Refer to the Demo files `custom_crc32.c` and `custom_crc16.c` for the CRC implementation.

| Area       | Offset | Size | Description                                                  |
| ---------- | ------ | ---- | ------------------------------------------------------------ |
| SOF        | 0      | 1    | Frame header, fixed as 0xAA                                  |
| Ver/Length | 1      | 2    | [15:10] - Version number, default value 0   [9:0] - Total frame length   Note: LSB in first |
| CmdType    | 3      | 1    | [4:0] - Response Type<br/>0 - No response required after data is sent<br/>1 - Response is required after data is sent, but no response is also acceptable<br/>2-31 - Response is mandatory after data is sent<br>[5] - Frame Type<br/>0 - Command Frame<br/>1 - Response Frame<br/>[7:6] - Reserved, default value 0 |
| ENC        | 4      | 1    | [4:0] - Padding byte length for encryption (encryption must be 16-byte aligned)<br/>[7:5] - Encryption type<br/>0 - No encryption<br/>1 - AES256 encryption |
| RES        | 5      | 3    | Reserved byte segment                                        |
| SEQ        | 8      | 2    | Sequence number                                              |
| CRC-16     | 10     | 2    | Frame checksum (from SOF to SEQ)                             |
| DATA       | 12     | n    | **DATA segment**, detailed description below                 |
| CRC-32     | n+12   | 4    | Frame checksum (from SOF to DATA)                            |

Note: The entire data packet uses **little-endian** storage format.

## DATA Segment

The core of this program lies in encapsulating and parsing the DATA segment. Although the length of the DATA segment is variable, the first two bytes are always CmdSet and CmdID. Each (CmdSet, CmdID) pair determines a specific function. The structure of the DATA segment (offset 12, size n) is as follows:

```
|  CmdSet  |  CmdID   |     Data Payload   |
|----------|----------|--------------------|
|  1-byte  |  1-byte  |     (n-2) byte     |
```

The Data Payload typically contains **Command Frame** or **Response Frame**.

For a detailed description of the DATA segment, please refer to: [DATA Segment Documentation](./protocol_data_segment.md)

## Frame Interaction Process

<img title="Frame Interaction Process" src="./images/frame_interaction_process.png" alt="Frame interaction process" data-align="center" width="290">

Note: When the sender is the remote controller, the receiver is the camera device; when the sender is the camera device, the receiver is the remote controller.

As shown in the figure above, during a transmission protocol process, the sender determines the SEQ (sequence number). If a reply is required from the receiver, it must reply with the same SEQ. The CmdType [4:0] field in the protocol indicates whether a reply to the frame is needed.

An example of constructing a DJI R SDK send frame can be found in the **Mode Switch** feature in the [DATA Segment Documentation](./protocol_data_segment.md)

## Protocol Layer

The protocol layer is a separate module in this program designed to facilitate the parsing of protocol frames. The file structure is as follows:

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

- **dji_protocol_parser**: Responsible for the encapsulation and parsing of the DJI R SDK protocol frames.
- **dji_protocol_data_processor**: Responsible for the encapsulation and parsing of the DATA payload.
- **dji_protocol_data_descriptors**: Defines a triple (CmdSet, CmdID) - creator - parser for each function to facilitate functionality expansion.
- **dji_protocol_data_structures**: Defines structures for command frames and response frames.

<img title="Protocol Layer Sequence Diagram" src="images/sequence_diagram_of_protocol_layer.png" alt="Protocol Layer Sequence Diagram" data-align="center" width="800">

The image above demonstrates how the protocol layer parses the DJI R SDK frames, and the assembly process is similar.

When the DJI R SDK protocol remains unchanged, you do not need to modify `dji_protocol_parser`. Similarly, `dji_protocol_data_processor` does not require modification, as it calls the generic `creator` and `parser` methods defined in `dji_protocol_data_descriptors`:

```c
typedef uint8_t* (*data_creator_func_t)(const void *structure, size_t *data_length, uint8_t cmd_type);
typedef int (*data_parser_func_t)(const uint8_t *data, size_t data_length, void *structure_out, uint8_t cmd_type);
```

Therefore, when adding new functionality parsing, simply define the frame structure in `dji_protocol_data_structures`, add the corresponding `creator` and `parser` functions in `dji_protocol_data_descriptors`, and include them in the `data_descriptors` triple.

Below are the command functions currently supported by this program:

```c
/* Structure support, but need to define creator and parser for each structure */
const data_descriptor_t data_descriptors[] = {
    // Camera mode switch
    {0x1D, 0x04, (data_creator_func_t)camera_mode_switch_creator, (data_parser_func_t)camera_mode_switch_parser},
    // Version query
    {0x00, 0x00, NULL, (data_parser_func_t)version_query_parser},
    // Record control
    {0x1D, 0x03, (data_creator_func_t)record_control_creator, (data_parser_func_t)record_control_parser},
    // GPS data push
    {0x00, 0x17, (data_creator_func_t)gps_data_creator, (data_parser_func_t)gps_data_parser},
    // Connection request
    {0x00, 0x19, (data_creator_func_t)connection_data_creator, (data_parser_func_t)connection_data_parser},
    // Camera status subscription
    {0x1D, 0x05, (data_creator_func_t)camera_status_subscription_creator, NULL},
    // Camera status push
    {0x1D, 0x02, NULL, (data_parser_func_t)camera_status_push_data_parser},
    // Key report
    {0x00, 0x11, (data_creator_func_t)key_report_creator, (data_parser_func_t)key_report_parser},
};
```
