# Q&A

This document is intended to address some common questions and answers. Before reading it, please ensure you have thoroughly and carefully reviewed all other relevant documentation.

[Project README Documentation](../README.md): Introduces the background and structure of the entire open-source project.

[Protocol Parsing Documentation](docs/protocol.md): Quickly understand the basic structure of the DJI R SDK protocol.

[DATA Segment Documentation](./protocol_data_segment.md): Detailed explanation of the various commands and functions supported by the camera.

[Data Layer Documentation](docs/data_layer.md): Design principles and implementation details of the data relay layer, provided for developer reference.

[Add Camera Sleep Feature Example Documentation](docs/add_camera_sleep_feature_example.md): Demonstrates how to quickly add a new control feature to the remote controller.

Note: This demo code is for reference only. If you encounter any bugs, please submit a detailed issue including videos or photos of the reproduction process, output logs, and reproduction steps. We will address the issue as soon as possible. Contributions via PRs are also welcome for fixes and improvements.

## 1. Characteristic Values for Communication Between Remote Controller and Camera

| **Characteristic** | **Description**                                              |
| ------------------ | ------------------------------------------------------------ |
| 0xFFF4             | Sent by the camera, received by the remote controller; notifications must be enabled |
| 0xFFF5             | Received by the camera, sent by the remote controller        |

Reference code:

```c
/* Define the Service/Characteristic UUIDs to filter, for search use */
#define REMOTE_TARGET_SERVICE_UUID   0xFFF0
#define REMOTE_NOTIFY_CHAR_UUID      0xFFF4
#define REMOTE_WRITE_CHAR_UUID       0xFFF5
```

After establishing the connection, only frames starting with `0xAA` need to be processed; all other broadcast packets can be ignored.

## 2. How to Put the Camera to Sleep and Wake It Up

The camera can enter sleep mode either by long-pressing its power button or through remote controller operations. Long-pressing the record button on the remote controller will put the camera into sleep mode. While the camera is asleep, pressing any button will wake it up. If the record button is pressed, the camera will wake up and immediately start recording, then automatically return to sleep mode after recording is complete.

Please note the following:

- Do not send any further data to the camera once it has entered sleep mode.
- Once a sleep acknowledgment is received from the camera, it can be considered successfully asleep; the Bluetooth connection will remain active.
- During the transition into sleep mode, the camera may still send a few status frames, which can be safely ignored.
- When the camera wakes up, the Bluetooth connection will temporarily disconnect; the remote controller should automatically reconnect.
- In order to wake up a target camera via broadcast, the remote controller must have successfully connected to that camera recently.

For detailed implementation, refer to the **Camera Power Mode Settings (001A)** feature in the [DATA Segment Documentation](./protocol_data_segment.md).

## 3. How to Disable Firmware Upgrade?

To disable the upgrade prompt, set the `fw_version` byte to 0 in the connection request protocol.

**Note:** This operation is only supported on cameras with the latest firmware.

For detailed implementation, refer to the **Connection Request (0019)** feature in the [DATA Segment Documentation](./protocol_data_segment.md).

## 4. How to Identify Camera Broadcast?

Supported cameras can be identified by checking specific bytes in the manufacturer field.

The logic is as follows: if bytes 0, 1, and 4 of the manufacturer field are 0xAA, 0x08, and 0xFA respectively, the camera is considered supported.

For implementation details, see the `bsp_link_is_dji_camera_adv` function in the `ble.c` file.

## 5. Simulated GPS command frame push but dashboard data appears abnormal?

It is not recommended to use simulated data to construct the **GPS Data Push (0017)** command frame as described in the [DATA Segment Documentation](./protocol_data_segment.md). Since the dashboard functionality relies on this data, using non-authentic data may result in abnormal display issues within the app.

For correctly constructed GPS command frames, refer to the `test_gps.c` file. In `app_main.c`, calling `start_ble_packet_test(1)` enables 1Hz cyclic data pushing for testing purposes. We will continue to improve these test datasets, and contributions of real, accurate test data are highly encouraged.

## 6. Snapshot Function After Wake-Up

First, send a broadcast wake-up packet to wake the camera.

Once the camera is awake, simply report a single-click shutter button event.

After capturing or recording, the camera will automatically return to sleep mode.

For detailed implementation, refer to the **Key Reporting (0011)** feature in the [DATA Segment Documentation](./protocol_data_segment.md).

## 7. **Magnification Calculation**

- In slow-motion mode, magnification equals frame rate divided by 30.
- In motion timelapse mode, magnification equals the time interval.
- In static timelapse mode, there is no magnification display—only the interval time is shown.

## 8. UI Design Corresponding to Camera Modes

Reference: Please refer to the **Camera Status Push (1D02)** function in the [DATA Segment Detailed Documentation](protocol_data_segment.md), which provides detailed explanations on how parameters are displayed and mapped to frame fields under different camera modes.
