                Peripheral Device with UART Server Sample Code
                ==============================================

NOTE: If you use this sample application for your own purposes, follow
      the licensing agreement specified in Software_Use_Agreement.rtf
      in the home directory of the installed RSL10 Software
      Development Kit (SDK).

Overview
--------
This sample project generates a battery service, a custom service, and a UART
interface with a baud rate of 115200. It then starts an undirected connectable 
advertising with the device's public address, if an address is available at 
DEVICE_INFO_BLUETOOTH_ADDR in non-volatile memory three (NVR3). If this 
address is not defined (all 1s or 0s), use a pre-defined, private Bluetooth(R)
address (PRIVATE_BDADDR) located in ble_std.h.

For this sample project, any central device can scan, connect, and perform 
service discovery, receive battery value notifications, or read the battery 
value. The central device has the ability to read and write custom attributes. 
The RSL10 ADC is used to read the battery level value every 200 ms when there 
is a kernel timer event. The average for 16 reads is calculated, and if this 
average value changes, a flag is set to send a battery level notification. 
In addition, if the custom service notification is enabled, it sends a 
notification with an incremental value every six seconds. 

This sample project passes through several states before all services are 
enabled:
1. APPM_INIT (initialization)
   Application initializing and is configured into an idle state. 
2. APPM_CREATE_DB (create database)
   Application has configured the Bluetooth stack, including GAP, according to
   the required role and features. It is now adding services, handlers, and 
   characteristics for all services that it will provide.
3. APPM_READY (ready)
   Application has added the desired standard and custom services or profiles 
   into the Bluetooth GATT database and handlers.
4. APPM_ADVERTISING (advertising)
   The device starts advertising based on the sample project.
5. APPM_CONNECTED (connected)
   Connection is now established with another compatible device.

After establishing connection, the BLE_SetServiceState function is called, in 
which, for any profiles/services that need to be enabled, an enable request 
message is sent to the corresponding profile of the Bluetooth stack.
Once the response is received for the specific profile, a flag is set to 
indicate to the application that it is enabled and ready to use.  

Next, the UART exchange is initiated. The UART data is buffered for TX and RX
using two DMA buffers. The UART RX DMA buffer is periodically polled for data
that has been received, and if data is available, this is sent out in BLE
packets. Conversely if data has been received from the Bluetooth custom
service, it is added to the UART TX DMA buffer for output.

NOTE: The DMA is used without interrupts, in order to demonstrate support for
      an arbitrary-length data transfer without the need for handling each
      individual received word from the specified interface.

This application uses the following optional features of Bluetooth low energy
technology when the connected device also supports them:

- LE 2M PHY (Bluetooth 5): Use of this feature allows the device to transfer
  data using a symbol rate/bit rate of 2 Mb/s, reducing the amount of time
  that the radio is active (reducing power), and allows for the possibility of
  higher throughput transfers.

- LE Data Packet Length Extension (Bluetooth 4.2): Use of this feature allows
  this application to request an increase in the maximum transfer unit (MTU)
  allowed. This feature supports a maximum increase in the underlying data
  packet size from 27 bytes to 251 bytes, and this application supports a 
  maxiumum packet size of 250 bytes per packet.
  
Structure
---------
This sample project is structured as follows:

The source code exists in a "code" folder, all application-related include
header files are in the "include" folder, and the main() function "app.c" is 
located in the parent directory.

Code
----
    app_init.c    - All initialization functions are called here, but the 
                    implementation is in the respective C files
    app_process.c - Message handlers for the application
    ble_bass.c    - Support functions and message handlers pertaining to the
                    Battery Service Server
    ble_custom.c  - Support functions and message handlers pertaining to the 
                    Custom Service Server
    ble_std.c     - Support functions and message handlers pertaining to
                    Bluetooth low energy technology
    uart.c        - Support code for handling DMA-based UART data transfers

Include
-------
    app.h        - Overall application header file
    ble_bass.h   - Header file for the Battery Service Server
    ble_custom.h - Header file for the Custom Service Server
    ble_std.h    - Header file for the Bluetooth low energy standard
    uart.h       - Header file for the DMA-based UART data tranfer support 
                   code
    
Hardware Requirements
---------------------
This application can be executed on any RSL10 Evaluation and Development 
Board. The application needs to be connected to a terminal application or 
similar that can read and write serial UART data at 115200 baud. No external 
connections are required.

Importing a Project
-------------------
To import the sample code into your IDE workspace, refer to the 
Getting Started Guide for your IDE for more information.

Select the project configuration according to the chip ID version of RSL10. 
The appropriate libraries and include files are loaded according to the build
configuration that you choose. Use "Debug" for CID 101. Ensure that the device
CID matches the application build.
  
Verification
------------
To verify the BLE functionality, use RSL10 or another third-party central 
device application to establish a connection. In addition to establishing
a connection, this application can be used to read/write characteristics 
and receive notifications.

To show how an application can send notifications, for every 30 timer 
expirations a notification request flag is set, and the application sends an 
incremental value of the first attribute to a peer device.

Alternatively, while the Bluetooth application manager is in the state 
"APPM_ADVERTISING", the LED on the RSL10 Evaluation and Development Board 
DIO6) is blinking. It turns on steadily once the link is established, and it 
enters the APPM_CONNECTED state.

The UART functionality can be verified by connecting a terminal
application to RSL10 to send and receive serial data at the specified baud
rate. The correct COM ports to use can be identified using the computer's 
Device Manager. Look for "JLink CDC UART Port (COMxx)".

Once connected, character strokes written to the terminal of 
central_client_uart are echoed back to the terminal of peripheral_server_uart.
Similarly, character strokes written to the terminal of peripheral_server_uart
are echoed back to the terminal of central_client_uart. Note that some 
terminal programs don't handle carriage return characters cleanly, and will 
restart progress on a line following a carriage return without inserting a 
newline.

Notes
-----
#1
The theoretical maximum throughput for a  Bluetooth low energy
connection is 175 kB per second (if sending data in only one direction),
and lower if sending bi-directional traffic. 

This application attempts to maximize the effective throughput for both 
single direction and bi-directional transfers, and implements buffering of
the incoming UART data to ensure that we don’t lose data if we exceed
the effective throughput for a short period of time.  

For larger files at high data rates (beyond 115,200 baud), or a pair of files
being transferred bi-directionally at moderate data rates (beyond 25,000
baud), this can overflow the internal buffers and cause packet loss.
A user could extend the UART application to  implement UART traffic flow 
control to ensure that buffer overflows do not occur.

#2
Sometimes the firmware in RSL10 cannot be successfully re-flashed, due to the
application going into Sleep Mode or resetting continuously (either by design 
or due to programming error). To circumvent this scenario, a software recovery
mode using DIO12 can be implemented with the following steps:

1.  Connect DIO12 to ground.
2.  Press the RESET button (this restarts the application, which will
    pause at the start of its initialization routine).
3.  Re-flash RSL10. After successful re-flashing, disconnect DIO12 from
    ground, and press the RESET button so that the application will work
    properly.

==============================================================================
Copyright (c) 2017 Semiconductor Components Industries, LLC
(d/b/a ON Semiconductor).
