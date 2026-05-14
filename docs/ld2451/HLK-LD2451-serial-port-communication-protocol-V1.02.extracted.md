# HLK-LD2451 Serial Port Communication Protocol V1.02 (Extracted Text)

> Source PDF: `docs/ld2451/HLK-LD2451-serial-port-communication-protocol-V1.02.pdf`
> Note: Auto-extracted from PDF; formatting may be imperfect.

## Page 1

Page 12 pages in total1
Shenzhen Hi-Link Electronic Co., Ltd.
HLK-LD2451
Serial communication protocol
Version: V1.03 Modification date: 2024-5-20 Copyright@Shenzhen Hi-Link Electronic Co., Ltd.

## Page 2

LD2451
Page 12 pages in total
Shenzhen Hi-Link Electronic Co., Ltd.
2
Content
1 Communication protocol .........................................................................................3
1.1 Protocol Format ............................................................................................3
1.2 Send command and ACK .............................................................................3
1.3 Radar data output protocol ........................................................................... 8
1.4 Radar command configuration method ......................................................10
2 Revision History ....................................................................................................12
3 Technical support and contact information ...........................................................12
Table Index
Table2 Send command protocol frame format ................................................................................... 3
Table3 Send frame data format ...........................................................................................................3
Table4 ACK command protocol frame format ...................................................................................3
Table5 ACK frame data format .......................................................................................................... 3
Table6 Serial port baud rate selection .................................................................................................7
Table8 Reporting data frame format ...................................................................................................8
Table9 Intra-frame data frame format .................................................................................................9
Figure2 Radar command configuration process ............................................................................... 11
Shenzhen Hi-Link Electronic Co., Ltd.

## Page 3

LD2451
Page 12 pages in total
Shenzhen Hi-Link Electronic Co., Ltd.
3
1 Communication protocol
This communication protocol is mainly used by users who are separated from
visual tools for secondary development. The LD2451 communicates with the outside
world through a serial port (TTL level). Radar data output and parameter configuration
commands are carried out under this agreement. The radar serial port has a default
baud rate of 115200, 1 stop bit and no parity bit.
1.1 Protocol Format
1.1.1 Protocol data format
Serial port data communication of LD2451 uses the small-end format. All data in
the following table is hexadecimal.
1.1.2 Command protocol frame format
The radar configuration command and ACK command formats defined by the
protocol are shown in Table 1 to Table 4.
Table2 Send command protocol frame format
Frame Header Data length in frame Intra-frame data Frame end
FD FC FB FA 2 bytes See Table 3 04 03 02 01
Table3 Send frame data format
Command word (2 bytes) Command value (N bytes)
Table4 ACK command protocol frame format
Frame Header Data length in frame Intra-frame data Frame end
FD FC FB FA 2 bytes See Table5 04 03 02 01
Table5 ACK frame data format
Send command word & 0x0100 (2 bytes) Return value (N bytes)
1.2 Send command and ACK
1.2.1 Enable configuration command
Any other command issued to the radar must be executed after this command is
issued, otherwise it will be invalid.
Command word: 0x00FF
Command value: 0x0001
Return value: 2 bytes ACK status (0 success, 1 failure) + 2 bytes protocol version
(0x0001) + 2 bytes

## Page 4

LD2451
Page 12 pages in total
Shenzhen Hi-Link Electronic Co., Ltd.
4
Send data:
FD FC FB FA 04 00 FF 00 01 00 04 03 02 01
Radar ACK (success):
FD FC FB FA 08 00 FF 01 00 00 01 00 00 00 04 03 02 01
1.2.2 End configuration command
End the configuration command, and the radar will resume working mode after
execution. If you need to send other commands again, you need to send the enable
configuration command first.
Command word: 0x00FE
Command value: None
Return value: 2-byte ACK status (0 for success, 1 for failure)
Send data:
FD FC FB FA 02 00 FE 00 04 03 02 01
Radar ACK (success):
FD FC FB FA 04 00 FE 01 00 00 04 03 02 01
1.2.3 Target detection parameter configuration commands
Command word: 0x0002
Command Value:4 bytes
Maximum detection
distance
Movement direction
setting
Minimum motion speed
setting
No target delay time
setting
1 byte 1 byte 1 byte 1 byte
(0A-FF):
Unit: m
00: Only detect away;
01: Only detect approach;
02: All detected
(00-0x78):
Unit: km/h 00~FF: unit s
Return value: 2-byte ACK status (0 for success, 1 for failure)
Send data:
FD FC FB FA 06 00 02 00 64 01 05 02 04 03 02 01
Maximum detection distance: 0x64 100 meters
Movement direction setting: 0x01 (only detect approach)
Minimum movement speed setting: 0x05 5km/h
No target delay time setting: 0x02 2s

## Page 5

LD2451
Page 12 pages in total
Shenzhen Hi-Link Electronic Co., Ltd.
5
Radar ACK (success):
FD FC FB FA 04 00 02 01 00 00 04 03 02 01
1.2.4 Read target detection parameter command
This command can read the radar's current target detection parameters.
Command word: 0x0012
Command value: None
Return value: 2 bytes ACK status (0 success, 1 failure)+ 4-byte configuration value
(same format as setting command)
Send data:
FD FC FB FA 02 00 12 00 04 03 02 01
Radar ACK:
FD FC FB FA 08 00 12 01 00 00 64 01 05 02 04 03 02 01
1.2.5 Radar sensitivity parameter configuration command
Command word: 0x0003
Command Value: 4 Byte sensitivity value
Cumulative effective trigger times Signal-to-noise ratio threshold level Extended Parameters Extended Parameters
1 byte 1 byte 1 byte 1 byte
1-0A:
The alarm information will be
reported only after the number of
consecutive detections is met.
(The program defaults to 1)
00: The program default parameter
is 4;
3-8: The larger the value, the lower
the sensitivity and the more
difficult it is to detect the target.
00 00
Return value: 2-byte ACK status (0 for success, 1 for failure)
Send data:
FD FC FB FA 06 00 03 00 02 08 00 00 04 03 02 01
Configure the effective trigger times to 2 times, set the signal-to-noise ratio threshold
level to 8

## Page 6

LD2451
Page 12 pages in total
Shenzhen Hi-Link Electronic Co., Ltd.
6
Radar ACK (success):
FD FC FB FA 04 00 03 01 00 00 04 03 02 01
1.2.6 Radar sensitivity parameters inquire command
This command queries the motion sensitivity of each distance gate.
Command word: 0x0013
Command Value: none
Return value: 2-byte ACK status (0 for success, 1 for failure)+ 4Byte sensitivity
value(The format is the same as the setting command)
Send data:
FD FC FB FA 02 00 13 00 04 03 02 01
Radar ACK (success):
FD FC FB FA 08 00 13 01 00 00 01 00 00 00 04 03 02 01
Note: The current configuration has a valid trigger count of 1; the program default
signal-to-noise ratio threshold of 4 is used.
1.2.7 Read firmware version command
This command reads the radar firmware version information.
Command word: 0x00A0
Command value: None
Return value: 2 bytes ACK status(0 Success,1 Failure)+ 2 bytes firmware
type(0x2451)+2 byteshostversion number +4Byte minor version number
Send data:
FD FC FB FA 02 00 A0 00 04 03 02 01
Radar ACK (success):
FD FC FB FA 0B 00 A0 01 00 00 51 24 01 01 10 15 05 24 04 03 02 01
The corresponding version number is V1.01.24051510
1.2.8 Set the serial port baud rate
This command used to set the baud rate of the module's serial port. The
configuration value will not be lost when the power is off. The configuration value

## Page 7

LD2451
Page 12 pages in total
Shenzhen Hi-Link Electronic Co., Ltd.
7
will take effect after the module is restarted.
Command word: 0x00A1
Command value: 2 bytes baud rate selection index
Return value: 2-byte ACK status (0 for success, 1 for failure)
Table6 Serial port baud rate selection
Baud rate selection index
value Baud rate
0x0001 9600
0x0002 19200
0x0003 38400
0x0004 57600
0x0005 115200
0x0006 230400
0x0007 256000
0x0008 460800
The factory default is 0x0005, that is, 115200.
Send data:
FD FC FB FA 04 00 A1 00 07 00 04 03 02 01
Radar ACK (success):
FD FC FB FA 04 00 A1 01 00 00 04 03 02 01
1.2.9 Restoring Factory Settings
Using this command, you can restore all configuration values to their default
values. The configuration values take effect after the module is restarted.
Command word: 0x00A2
Command Value:none
Return value: 2-byte ACK status (0 for success, 1 for failure)
Send data:
FD FC FB FA 02 00 A2 00 04 03 02 01
Radar ACK (success):
FD FC FB FA 04 00 A2 01 00 00 04 03 02 01

## Page 8

LD2451
Page 12 pages in total
Shenzhen Hi-Link Electronic Co., Ltd.
8
1.2.10 Restart module
When the module receives this command, it will automatically restart after the
response is sent.
Command word: 0x00A3
Command Value:none
Return value: 2-byte ACK status (0 for success, 1 for failure)
Send data:
FD FC FB FA 02 00 A3 00 04 03 02 01
Radar ACK (success):
FD FC FB FA 04 00 A3 01 00 00 04 03 02 01
1.3 Radar data output protocol
LD2451 output radar detection results through serial port, default output target
basic information, including target status, moving energy value, stationary energy
value, moving distance, stationary distance and other information. If the radar is
configured in engineering mode, the radar will output additional range gate energy
values (moving & stationary). Radar data is output in the specified frame format.
1.3.1 Reporting data frame format
Table 8 and Table 9 show the format of radar report message frames defined by
the protocol. Table 10 describes the definitions of reported data type values in normal
working mode and engineering mode.
Table8 Reporting data frame format
Frame Header Data length in frame Intra-frame data Frame tail
F4 F3 F2 F1 2 bytes See Table9 F8 F7 F6 F5

## Page 9

LD2451
Page 12 pages in total
Shenzhen Hi-Link Electronic Co., Ltd.
9
Table9 Intra-frame data frame format
Target
quantity
Alarm
information Target 1 Information Target 2
information
Target N
information
1 byte 1 byte
5 bytes 5 bytes 5 bytes
Angle
1 byte
Distance
1 byte
Speed
direction
1 byte
Speed value
1 byte
SNR
1 byte
Same as
left Same as left
01:
There is a
target
approaching
00:
No approach
target
Unit: Degree
Actual angle
value=Report
value - 0x80
Unit: m
0～100
00: Close
01: Stay
away
Unit:km/h
0～120
0～
255
Same as
left Same as left
Data example:
F4 F3 F2 F1 110003 01 8A 28 00 3C 15 8A IE 01 3C 0F 76 5F 00 3C 0F F8 F7
F6 F5
Analysis:
Data length: 0x11 17 bytes
Alarm information: 0x01 There is a target approaching
Target number: 0x03
Objective 1 Information:
Angle: 0x8A, 10 degrees (0x8A-0x80)
Distance: 0x28,40 meters
Speed: 0x003C, the target is moving towards the radar direction, the speed bit is
60km/h
Signal-to-noise ratio: 0x15
Objective 2 Information:
Angle: 0x8A, 10 degrees (0x8A-0x80)
Distance: 0x1E, 30 meters
Speed: 0x013C, the target is moving away from the radar, the speed bit is 60km/h
Signal-to-noise ratio: 0x0F
Goal 3 Information:
Angle: 0x76, -10 degrees (0x76-0x80)
Distance: 0x5F, 95 meters
Speed: 0x003C, the target is moving towards the radar direction, the speed bit is
60km/h
Signal-to-noise ratio: 0x0F

## Page 10

LD2451
Page 12 pages in total
Shenzhen Hi-Link Electronic Co., Ltd.
10
1.4 Radar command configuration method
1.4.1 Radar command configuration steps
The process of executing a configuration command by the LD2451 radar
includes two links: "send command" by the upper computer and "reply command
ACK" by the radar. If the radar does not respond with an ACK or fails to respond with
an ACK, it indicates that the radar fails to execute the configuration command.
As mentioned earlier, before sending any other commands to the radar, the
developer needs to send the "enable configuration" command, and then send the
configuration command within the specified time. After the command configuration is
complete, send the "End configuration" command to inform the radar that the
configuration is complete.
For example, to read the radar configuration parameters, the host computer first
sends the "enable configuration" command; After receiving the radar ACK
successfully, send the "Read parameters" command. After receiving the radar ACK
successfully, send the End configuration command. After the radar ACK succeeds, it
indicates that the complete parameter reading operation is complete.
The radar command configuration process is shown in the following figure.

## Page 11

LD2451
Page 12 pages in total
Shenzhen Hi-Link Electronic Co., Ltd.
11
Figure2 Radar command configuration process

## Page 12

LD2451
Page 12 pages in total
Shenzhen Hi-Link Electronic Co., Ltd.
12
2 Revision History
3 Technical support and contact information
Shenzhen Hi-Link Electronic Co., Ltd
Address：17F Building E, Xinghe WORLD, Minzhi Street, Long Hua district,
Shenzhen 518131
Phone：0755-23152658/83575155
Email: sales@hlktech.com
Website：https://www.hlktech.net/
Date Version Modifications
2024-5-15 1.01 initial version
2024-5-20 1.02 Modify some error descriptions
2024-7-1 1.03 Modify some error descriptions
