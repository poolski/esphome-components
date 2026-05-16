# HLK-LD2451 Serial Port Communication Protocol V1.02 (Extracted Text)

> Source PDF: `docs/ld2451/HLK-LD2451-serial-port-communication-protocol-V1.02.pdf`
>
> Note: This file is normalized from auto-extracted PDF text for readability.
> Frame values and protocol data are preserved; formatting is improved.

## Table of Contents

- [HLK-LD2451 Serial Port Communication Protocol V1.02 (Extracted Text)](#hlk-ld2451-serial-port-communication-protocol-v102-extracted-text)
  - [Table of Contents](#table-of-contents)
  - [1 Communication protocol](#1-communication-protocol)
    - [1.1 Protocol format](#11-protocol-format)
      - [1.1.1 Protocol data format](#111-protocol-data-format)
      - [1.1.2 Command protocol frame format](#112-command-protocol-frame-format)
    - [1.2 Send command and ACK](#12-send-command-and-ack)
      - [1.2.1 Enable configuration command (`0x00FF`)](#121-enable-configuration-command-0x00ff)
      - [1.2.2 End configuration command (`0x00FE`)](#122-end-configuration-command-0x00fe)
      - [1.2.3 Target detection parameter configuration (`0x0002`)](#123-target-detection-parameter-configuration-0x0002)
      - [1.2.4 Read target detection parameter command (`0x0012`)](#124-read-target-detection-parameter-command-0x0012)
      - [1.2.5 Radar sensitivity parameter configuration (`0x0003`)](#125-radar-sensitivity-parameter-configuration-0x0003)
      - [1.2.6 Radar sensitivity parameters inquire command (`0x0013`)](#126-radar-sensitivity-parameters-inquire-command-0x0013)
      - [1.2.7 Read firmware version command (`0x00A0`)](#127-read-firmware-version-command-0x00a0)
      - [1.2.8 Set serial port baud rate (`0x00A1`)](#128-set-serial-port-baud-rate-0x00a1)
      - [1.2.9 Restore factory settings (`0x00A2`)](#129-restore-factory-settings-0x00a2)
      - [1.2.10 Restart module (`0x00A3`)](#1210-restart-module-0x00a3)
    - [1.3 Radar data output protocol](#13-radar-data-output-protocol)
      - [1.3.1 Reporting data frame format](#131-reporting-data-frame-format)
    - [1.4 Radar command configuration method](#14-radar-command-configuration-method)
      - [1.4.1 Radar command configuration steps](#141-radar-command-configuration-steps)
  - [2 Revision History](#2-revision-history)
  - [3 Technical support and contact information](#3-technical-support-and-contact-information)

## 1 Communication protocol

This communication protocol is mainly used by users who are separated from visual tools for secondary development.
The LD2451 communicates with the outside world through a serial port (TTL level).
Radar data output and parameter configuration commands are carried out under this agreement.
The radar serial port has a default baud rate of `115200`, 1 stop bit, and no parity bit.

### 1.1 Protocol format

#### 1.1.1 Protocol data format

Serial port data communication of LD2451 uses little-end format.
All data in the following sections is hexadecimal.

#### 1.1.2 Command protocol frame format

The radar configuration command and ACK command formats defined by the protocol are shown below.

**Table 2. Send command protocol frame format**

| Frame Header  | Data length in frame | Intra-frame data | Frame end     |
| ------------- | -------------------- | ---------------- | ------------- |
| `FD FC FB FA` | 2 bytes              | See Table 3      | `04 03 02 01` |

**Table 3. Send frame data format**

| Command word | Command value |
| ------------ | ------------- |
| 2 bytes      | N bytes       |

**Table 4. ACK command protocol frame format**

| Frame Header  | Data length in frame | Intra-frame data | Frame end     |
| ------------- | -------------------- | ---------------- | ------------- |
| `FD FC FB FA` | 2 bytes              | See Table 5      | `04 03 02 01` |

**Table 5. ACK frame data format**

| Field                        | Size    |
| ---------------------------- | ------- |
| Send command word `& 0x0100` | 2 bytes |
| Return value                 | N bytes |

### 1.2 Send command and ACK

#### 1.2.1 Enable configuration command (`0x00FF`)

Any other command issued to the radar must be executed after this command is issued, otherwise it will be invalid.

- Command word: `0x00FF`
- Command value: `0x0001`
- Return value: 2-byte ACK status (`0` success, `1` failure) + 2-byte protocol version (`0x0001`) + 2 bytes

Send data:

```text
FD FC FB FA 04 00 FF 00 01 00 04 03 02 01
```

Radar ACK (success):

```text
FD FC FB FA 08 00 FF 01 00 00 01 00 00 00 04 03 02 01
```

#### 1.2.2 End configuration command (`0x00FE`)

End the configuration command, and the radar will resume working mode after execution.
If you need to send other commands again, send the enable configuration command first.

- Command word: `0x00FE`
- Command value: None
- Return value: 2-byte ACK status (`0` success, `1` failure)

Send data:

```text
FD FC FB FA 02 00 FE 00 04 03 02 01
```

Radar ACK (success):

```text
FD FC FB FA 04 00 FE 01 00 00 04 03 02 01
```

#### 1.2.3 Target detection parameter configuration (`0x0002`)

- Command word: `0x0002`
- Command value: 4 bytes
- Return value: 2-byte ACK status (`0` success, `1` failure)

Command value byte layout:

| Byte | Field                        | Description                                                            |
| ---- | ---------------------------- | ---------------------------------------------------------------------- |
| 1    | Maximum detection distance   | `(0A-FF)`, unit: m                                                     |
| 2    | Movement direction setting   | `00`: only detect away; `01`: only detect approach; `02`: all detected |
| 3    | Minimum motion speed setting | `(00-0x78)`, unit: km/h                                                |
| 4    | No target delay time setting | `00~FF`, unit: s                                                       |

Send data:

```text
FD FC FB FA 06 00 02 00 64 01 05 02 04 03 02 01
```

Interpretation from source text:

- Maximum detection distance: `0x64` -> 100 meters
- Movement direction setting: `0x01` (only detect approach)
- Minimum movement speed setting: `0x05` -> 5 km/h
- No target delay time setting: `0x02` -> 2 s

Radar ACK (success):

```text
FD FC FB FA 04 00 02 01 00 00 04 03 02 01
```

#### 1.2.4 Read target detection parameter command (`0x0012`)

This command can read the radar's current target detection parameters.

- Command word: `0x0012`
- Command value: None
- Return value: 2-byte ACK status (`0` success, `1` failure) + 4-byte configuration value (same format as setting command)

Send data:

```text
FD FC FB FA 02 00 12 00 04 03 02 01
```

Radar ACK:

```text
FD FC FB FA 08 00 12 01 00 00 64 01 05 02 04 03 02 01
```

#### 1.2.5 Radar sensitivity parameter configuration (`0x0003`)

- Command word: `0x0003`
- Command value: 4-byte sensitivity value
- Return value: 2-byte ACK status (`0` success, `1` failure)

Command value byte layout:

| Byte | Field                                 | Description                                                                                                   |
| ---- | ------------------------------------- | ------------------------------------------------------------------------------------------------------------- |
| 1    | Cumulative effective trigger times    | `1-0A`: alarm info is reported only after this number of consecutive detections is met (program default is 1) |
| 2    | Signal-to-noise ratio threshold level | `00`: program default parameter is 4; `3-8`: larger value means lower sensitivity and harder target detection |
| 3    | Extended parameter                    | `00`                                                                                                          |
| 4    | Extended parameter                    | `00`                                                                                                          |

Send data:

```text
FD FC FB FA 06 00 03 00 02 08 00 00 04 03 02 01
```

Interpretation from source text:

- Configure effective trigger times to 2
- Set signal-to-noise ratio threshold level to 8

Radar ACK (success):

```text
FD FC FB FA 04 00 03 01 00 00 04 03 02 01
```

#### 1.2.6 Radar sensitivity parameters inquire command (`0x0013`)

This command queries the motion sensitivity of each distance gate.

- Command word: `0x0013`
- Command value: None
- Return value: 2-byte ACK status (`0` success, `1` failure) + 4-byte sensitivity value (same format as setting command)

Send data:

```text
FD FC FB FA 02 00 13 00 04 03 02 01
```

Radar ACK (success):

```text
FD FC FB FA 08 00 13 01 00 00 01 00 00 00 04 03 02 01
```

Note from source text:

- The current configuration has a valid trigger count of 1
- The program default signal-to-noise ratio threshold of 4 is used

#### 1.2.7 Read firmware version command (`0x00A0`)

This command reads radar firmware version information.

- Command word: `0x00A0`
- Command value: None
- Return value: 2-byte ACK status (`0` success, `1` failure) + 2-byte firmware type (`0x2451`) + 2-byte host version number + 4-byte minor version number

Send data:

```text
FD FC FB FA 02 00 A0 00 04 03 02 01
```

Radar ACK (success):

```text
FD FC FB FA 0B 00 A0 01 00 00 51 24 01 01 10 15 05 24 04 03 02 01
```

The corresponding version number in source text is `V1.01.24051510`.

#### 1.2.8 Set serial port baud rate (`0x00A1`)

This command sets the module serial port baud rate.
The value is retained across power cycles and takes effect after restart.

- Command word: `0x00A1`
- Command value: 2-byte baud rate selection index
- Return value: 2-byte ACK status (`0` success, `1` failure)

**Table 6. Serial port baud rate selection**

| Index value | Baud rate |
| ----------- | --------- |
| `0x0001`    | 9600      |
| `0x0002`    | 19200     |
| `0x0003`    | 38400     |
| `0x0004`    | 57600     |
| `0x0005`    | 115200    |
| `0x0006`    | 230400    |
| `0x0007`    | 256000    |
| `0x0008`    | 460800    |

Factory default is `0x0005` (`115200`).

Send data:

```text
FD FC FB FA 04 00 A1 00 07 00 04 03 02 01
```

Radar ACK (success):

```text
FD FC FB FA 04 00 A1 01 00 00 04 03 02 01
```

#### 1.2.9 Restore factory settings (`0x00A2`)

This command restores all configuration values to default values.
The configuration values take effect after module restart.

- Command word: `0x00A2`
- Command value: None
- Return value: 2-byte ACK status (`0` success, `1` failure)

Send data:

```text
FD FC FB FA 02 00 A2 00 04 03 02 01
```

Radar ACK (success):

```text
FD FC FB FA 04 00 A2 01 00 00 04 03 02 01
```

#### 1.2.10 Restart module (`0x00A3`)

When the module receives this command, it will automatically restart after sending the response.

- Command word: `0x00A3`
- Command value: None
- Return value: 2-byte ACK status (`0` success, `1` failure)

Send data:

```text
FD FC FB FA 02 00 A3 00 04 03 02 01
```

Radar ACK (success):

```text
FD FC FB FA 04 00 A3 01 00 00 04 03 02 01
```

### 1.3 Radar data output protocol

LD2451 outputs radar detection results through serial port.
By default, it outputs target basic information, including target status, moving energy value, stationary energy value,
moving distance, stationary distance, and other information.
If configured in engineering mode, the radar outputs additional range gate energy values (moving and stationary).
Radar data is output in the specified frame format.

#### 1.3.1 Reporting data frame format

Table 8 and Table 9 show the format of radar report message frames defined by the protocol.
The extracted source text references Table 10 for reported data type definitions in normal and engineering modes.

**Table 8. Reporting data frame format**

| Frame Header  | Data length in frame | Intra-frame data | Frame tail    |
| ------------- | -------------------- | ---------------- | ------------- |
| `F4 F3 F2 F1` | 2 bytes              | See Table 9      | `F8 F7 F6 F5` |

**Table 9. Intra-frame data frame format**

| Field                | Size    | Notes                                                                  |
| -------------------- | ------- | ---------------------------------------------------------------------- |
| Target quantity      | 1 byte  |                                                                        |
| Alarm information    | 1 byte  | `01`: there is a target approaching; `00`: no approach target          |
| Target 1 information | 5 bytes | Angle (1), Distance (1), Speed direction (1), Speed value (1), SNR (1) |
| Target 2 information | 5 bytes | Same as target 1                                                       |
| Target N information | 5 bytes | Same as target 1                                                       |

Per-target field notes from source text:

- Angle: 1 byte, unit degree, actual angle = report value `- 0x80`
- Distance: 1 byte, unit m, range `0~100`
- Speed direction: 1 byte (`00`: close; `01`: stay away)
- Speed value: 1 byte, unit km/h, range `0~120`
- SNR: 1 byte, range `0~255`

Data example:

```text
F4 F3 F2 F1 110003 01 8A 28 00 3C 15 8A IE 01 3C 0F 76 5F 00 3C 0F F8 F7 F6 F5
```

Analysis from source text:

- Data length: `0x11` (17 bytes)
- Alarm information: `0x01` (there is a target approaching)
- Target number: `0x03`

Target 1 information:

- Angle: `0x8A` -> 10 degrees (`0x8A - 0x80`)
- Distance: `0x28` -> 40 meters
- Speed: `0x003C`, target moving toward radar direction, speed 60 km/h
- Signal-to-noise ratio: `0x15`

Target 2 information:

- Angle: `0x8A` -> 10 degrees (`0x8A - 0x80`)
- Distance: `0x1E` -> 30 meters
- Speed: `0x013C`, target moving away from radar direction, speed 60 km/h
- Signal-to-noise ratio: `0x0F`

Target 3 information:

- Angle: `0x76` -> -10 degrees (`0x76 - 0x80`)
- Distance: `0x5F` -> 95 meters
- Speed: `0x003C`, target moving toward radar direction, speed 60 km/h
- Signal-to-noise ratio: `0x0F`

### 1.4 Radar command configuration method

#### 1.4.1 Radar command configuration steps

The process of executing a configuration command by the LD2451 radar involves 2 steps:

- "send command" by the upstream device
- "reply command ACK" by the radar

If the radar does not respond with an ACK, or fails to respond with a valid ACK,
it indicates that the radar failed to execute the configuration command.

Before sending any command other than configuration enable, send the enable configuration command first.
Then send the required configuration command(s) within the specified time.
After command configuration is complete, send the end configuration command.

Example from source text:

To read radar configuration parameters:

1. Send the enable configuration command.
2. After successful ACK, send the read parameters command.
3. After successful ACK, send the end configuration command.
4. After successful ACK, the complete parameter reading operation is complete.

The source document references "Figure 2 Radar command configuration process".

## 2 Revision History

| Date      | Version | Modifications                  |
| --------- | ------- | ------------------------------ |
| 2024-5-15 | 1.01    | initial version                |
| 2024-5-20 | 1.02    | Modify some error descriptions |
| 2024-7-1  | 1.03    | Modify some error descriptions |

## 3 Technical support and contact information

Shenzhen Hi-Link Electronic Co., Ltd

- Address: 17F Building E, Xinghe WORLD, Minzhi Street, Long Hua district, Shenzhen 518131
- Phone: 0755-23152658/83575155
- Email: `sales@hlktech.com`
- Website: https://www.hlktech.net/
