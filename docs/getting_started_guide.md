# Getting Started Guide

> This guide is designed for users who are using this demo for the first time and do not have Bluetooth development experience.
> Following this document, you can complete hardware connection and software configuration from scratch, and successfully establish a connection between the Bluetooth controller and the camera to transmit data.

## 1. Business Background

Taking the Osmo Action GPS Bluetooth controller released by DJI as a reference, this accessory can establish Bluetooth connections with devices such as DJI Osmo Action 4, Osmo Action 5 Pro, and Osmo 360, enabling interactive control of the camera. Users can not only send various control commands to the camera through the controller but also receive real-time camera status information, providing a smooth and convenient operation experience. Since Osmo series cameras do not have built-in GPS modules, the controller can synchronously push real-time GPS data to the DJI Mimo APP, enabling dashboard functionality and providing users with an immersive experience like "Fast & Furious".

To further expand this capability, we plan to apply it to more portable devices, such as smartwatches. Therefore, we launched [Osmo-GPS-Controller-Demo](https://github.com/dji-sdk/Osmo-GPS-Controller-Demo), providing complete documentation and sample code to help developers quickly integrate related functions and jointly enrich and improve the DJI Osmo ecosystem.

## 2. How to Interact with Camera

Communication between the controller and camera is implemented based on Bluetooth Low Energy (BLE). It should be clarified that in this communication architecture, the Bluetooth controller serves as the master device, and the Osmo camera serves as the slave device. Both parties interact according to this protocol mode, as shown in the figure below.

<img title="Osmo RC Master Slave Relationship" src="./images/osmo_rc_master_slave_relationship.png" alt="Osmo RC Master Slave Relationship" data-align="center" width="411">

The basic connection workflow of the Bluetooth controller is as follows:

* **Device Scanning**: The controller scans nearby Bluetooth broadcasts to find DJI Osmo devices with the highest signal strength that support the **open source protocol**.

* **Establish Connection**: After discovering the target device, initiate a connection request; after the camera agrees to connect, complete pairing and connection establishment.

* **Status Subscription**: After successful connection, the controller subscribes to the camera's status information to obtain real-time working status.

* **Interactive Operations**: Based on the established connection, execute interactive commands such as shooting, recording, mode switching, sleep, wake-up, GPS pushing, etc.

We use **Establish Connection** as an entry point to help you quickly gain an overall understanding of this demo:

* First, the **open source protocol** used is the **DJI R SDK** protocol. For detailed instructions, please refer to: [Protocol Parsing Documentation](./protocol.md).

* For detailed instructions on establishing connections, please refer to: **Connection Request (0019)** command set in [DATA Segment Documentation](./protocol_data_segment.md).

* If you need to reference the Demo's code implementation, please first read the [Demo README Documentation](../README.md) completely, familiar with hardware wiring diagrams and code structure, and then check the `connect_logic.c` file.

After reading the above documents, you will have a basic understanding of the overall business process and protocol, and then you can enter the code development stage.

## 3. How to Develop Code

Since the interaction process is based on Bluetooth BLE, development is essentially BLE application development. You can use different programming languages, such as Python, JavaScript, or C, and BLE has corresponding cross-platform support libraries on various platforms.

[Osmo-GPS-Controller-Demo](https://github.com/dji-sdk/Osmo-GPS-Controller-Demo) is based on the ESP32-C6 development board and written in C language. We recommend that you run and familiarize yourself with this Demo before developing Bluetooth controllers on other platforms, which will help with subsequent camera debugging and testing work.

During development, we recommend organizing your code by business logic layers, such as dividing into **ble**, **protocol**, **data**, **logic** modules in the Demo, to facilitate future expansion of new command sets or controller functions.

The following will demonstrate development approaches and highlight some key points for reference.

### 3.1 BLE Connection Logic Development

Mainly involves: broadcast searching, BLE connection, callback notification handling, etc.

Key Point 1: How to identify DJI camera broadcasts? When bytes 0, 1, and 4 of the manufacturer field are 0xAA, 0x08, and 0xFA respectively, it indicates that the camera is a supported device. You can refer to the **bsp_link_is_dji_camera_adv** function in `ble.c`.

Key Point 2: When developing Bluetooth applications, you need to first clarify the communication characteristic values of the target device. The characteristic values agreed upon by Osmo cameras are as follows:

| **Characteristic** | **Description**                                    |
| ------------------ | -------------------------------------------------- |
| 0xFFF4            | Camera sends, controller receives, needs notification enabled |
| 0xFFF5            | Camera receives, controller sends                  |

Reference code:

```c
/* Define the Service/Characteristic UUIDs to filter for searching */
#define REMOTE_TARGET_SERVICE_UUID   0xFFF0
#define REMOTE_NOTIFY_CHAR_UUID      0xFFF4
#define REMOTE_WRITE_CHAR_UUID       0xFFF5
```

After connection is established, only frames starting with `0xAA` (DJI R SDK Protocol) need to be processed; other broadcast data can be directly ignored.

After the BLE layer establishes a connection, the camera's communication channel is actually occupied, and other devices will no longer be able to connect via BLE. Subsequently, authentication connection needs to be completed through the DJI R SDK protocol before interaction with the camera can occur.

### 3.2 Protocol Parsing Development

Mainly involves: parsing and encapsulation of DJI R SDK protocol frames

In the previous section, you have learned about the basic structure of the DJI R SDK protocol. Next, you need to write parsing and encapsulation functions based on its frame format. You can refer to the existing code in the Demo:

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

### 3.3 Data Layer Development

Mainly involves: calling the BLE layer for frame data transmission and reception, calling the Protocol layer for protocol frame encapsulation and parsing

The Data layer connects to upper-level business logic and is responsible for writing data to lower layers and passing received data to upper layers, playing a crucial role in the overall architecture. The data layer in the Demo implements a data caching mechanism based on SEQ sequence numbers or Cmd command sets within the limited development board memory, and provides write and read interfaces to upper layers, enabling upper-level development without awareness of BLE details, similar to how the DAO layer operates on databases in backend development.

You can refer to the [Data Layer Documentation](./data_layer.md) and the `data.c` code file.

### 3.4 Logic Layer Development

Mainly involves: writing various controller business logic code, such as button event handling, indicator light control, and camera function control (shooting, recording, mode switching, sleep, wake-up, GPS pushing, etc.)

The Demo has integrated some basic logic functions, including long-pressing buttons to connect to the camera, single-clicking buttons for shooting or recording, subscribing to camera status, etc. Related implementations can be found in the code under the `logic` folder.

### 3.5 Development Process Summary

Through layered design, decoupling of various modules is achieved, providing convenience for subsequent feature expansion. When code development is completed according to the above layers, if you need to quickly add new features, you can refer to: [Add Camera Sleep Feature Example Documentation](./add_camera_sleep_feature_example.md)

## 4. How to Solve Problems During Development

First, please read [FAQ_for_this_demo](./Q&A.md) in detail to confirm whether the problems you encounter already have corresponding solutions.

**Camera push value related issues**: Please run this Demo or perform packet capture to obtain the corresponding protocol frames and analyze them yourself. The figure below shows an example of the Demo's log output, where **TX** represents data frames sent to the camera, and **RX** represents data frames sent from the camera to the controller. The frame parsing process and camera status are clear at a glance.

<img title="Debug Info" src="./images/debug_info.png" alt="Debug Info" data-align="center" width="1300">

If you notice discrepancies between the protocol documentation and the actual data pushed by the camera, this is normal. It is recommended to report the issue to DJI personnel to determine whether the discrepancy is due to an error in the documentation or a defect in the camera firmware.

## 5. Summary of All Documentation References

### 5.1 Demo Related Documentation

[Demo README Documentation](../README.md): Introduces the background and structure of the entire open source demo

[Protocol Parsing Documentation](./protocol.md): Quick understanding of the basic composition of the DJI R SDK protocol

[DATA Segment Documentation](./protocol_data_segment.md): Detailed explanation of various commands and functions supported by the camera

[Data Layer Documentation](./data_layer.md): Design principles and implementation details of the data layer for development reference

[Add Camera Sleep Feature Example Documentation](./add_camera_sleep_feature_example.md): Demonstrates how to quickly add a new control function to the controller

[FAQ_for_this_demo](./Q&A.md): Answers some common questions that may be encountered during development

### 5.2 External Reference Documentation

**ESP-IDF**: [ESP-IDF Official GitHub Repository](https://github.com/espressif/esp-idf/)

**LC76G GNSS Module**: [LC76G GNSS Module - Waveshare Wiki](https://www.waveshare.com/wiki/LC76G_GNSS_Module)

**ESP32-C6-WROOM-1**: [ESP32-C6-DevKitC-1 v1.2 - ESP32-C6 User Guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c6/esp32-c6-devkitc-1/user_guide.html)

