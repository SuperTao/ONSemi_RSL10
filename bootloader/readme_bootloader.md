Bootloader - Sample Code
========================

NOTE: If you use this sample application for your own purposes, follow
      the licensing agreement specified in `Software_Use_Agreement.rtf`
      in the home directory of the installed RSL10 Software
      Development Kit (SDK).

Overview
--------
This sample project demonstrates a simple application that provides a UART-based 
bootloader. This code is provided in a form to work with the RSL10 Evaluation 
and Development Boards, but can also be used with the RSL10 Dongle by changing 
a single definition in the configuration header file. (See code for details.)

Hardware Requirements
---------------------
This application can be executed on any RSL10 Evaluation and Development Board
with no external connections required.

Importing a Project
-------------------
To import the sample code into your IDE workspace, refer to the 
Getting Started Guide for your IDE for more information.

Configuration of a Sample to Work with the Bootloader
-----------------------------------------------------
For a standard sample code project to work with the bootloader, change the 
linker control file associated with the sample. The base address and length of 
the flash memory must be modified to allow the bootloader to reside in the 
bottom 8K of the flash memory.

__sections.ld__

e.g. change the memory allocations in the `sections.ld` file for the target 
sample application from:

    MEMORY
    {
     ROM  (r) : ORIGIN = 0x00000000, LENGTH = 4K
     FLASH (xrw) : ORIGIN = 0x00100000, LENGTH = 380K
     PRAM (xrw) : ORIGIN = 0x00200000, LENGTH = 32K

     DRAM (xrw) : ORIGIN = 0x20000000, LENGTH = 24K
     DRAM_DSP (xrw) : ORIGIN = 0x20006000, LENGTH = 48K
     DRAM_BB (xrw) : ORIGIN = 0x20012000, LENGTH = 16K
    }
to

    MEMORY
    {
     ROM  (r) : ORIGIN = 0x00000000, LENGTH = 4K
     **BOOT (xrw) : ORIGIN = 0x00100000, LENGTH = 8K**
     **FLASH (xrw) : ORIGIN = 0x00102000, LENGTH = 372K**
     PRAM (xrw) : ORIGIN = 0x00200000, LENGTH = 32K

     DRAM (xrw) : ORIGIN = 0x20000000, LENGTH = 24K
     DRAM_DSP (xrw) : ORIGIN = 0x20006000, LENGTH = 48K
     DRAM_BB (xrw) : ORIGIN = 0x20012000, LENGTH = 16K
    }

__RSL10 Evaluation and Development Board and RSL10 Dongle__

The `config.h` file in the source code of the bootloader provides some 
definitions to configure it to run in the RSL10 Evaluation and Development 
Board or in the RSL10 Dongle. RSL10_DEV_OR_DONGLE can be RESL10_DEV for the 
RSL10 Evaluation and Development Board or RSL10_DONGLE to update the bootloader 
in the RSL10 Dongle.

    #define RSL10_DEV_OR_DONGLE	 RSL10_DEV

    CFG_nUPDATE_DIO   RSL10 DIO number of the Bootloader
                      activation pin
If this DIO is held high upon power on reset, the bootloader enters the update 
mode instead of starting the user application. DIO15 is configured by default 
for the RSL10 Evaluation Board.

    CFG_UART_RXD_DIO   RSL10 DIO number of the UART RxD signal
    CFG_UART_TXD_DIO   RSL10 DIO number of the UART TxD signal

__Updater.py__

If you are using the RSL10 Evaluation Board, make sure you have the bootloader 
flash loaded and activated. You can now rebuild and load your sample application 
using the `updater.py` Python script over UART. The script is provided in 
the `scripts` subfolder, together with additional dependencies. You also need 
the `pyserial` Python module installed.

If you run `python updater.py -h`, it gives the following output:

if `updater.py` is version 1.0.0.

    usage: updater.py [-h] [-v] [--force] [--jlink] PORT FILE
if `updater.py` is version 2.0.0.

    usage: updater.py [-h] [-v] [--force] PORT [FILE]
Updates the RSL10 Evaluation and Development Board and the RSL10 Dongle with 
a firmware image file.

- Installation

    Installed Python, version >=2.7 or >=3.4
    Installed module pyserial, version >=3.2
- positional arguments

    PORT           `COM port of the RSL10 UART`
    FILE           `image file (.bin) to download`

- optional arguments

    -h, --help     show this help message and exit
    -v, --version  show program's version number and exit
    --force        force overwrite of the bootloader
    --jlink        updating dev board using JLink. It is for 
                   version 1.0.0

For every transmitted flash sector of image data, an asterisk (*) is printed.

Note
----
The bootloader expects to use `.bin` format files which is an alternative to 
the `.hex` files normally produced by the Eclipse projects. The build command 
can be included in the Post-build steps of Eclipse to generate the `.bin` 
format file. Go to Settings on the Properties tab and add the build command 
in the Post-build steps:
Navigate to `Project > Settings > C/C++ Build > Settings >`
`Build Steps > Post-build steps`

```
${cross_prefix}objcopy -O binary "${BuildArtifactFileName}" 
"${BuildArtifactFileBaseName}.bin"
```

Verification
------------
To verify the operation of the bootloader using an RSL10 Evaluation Board and 
the blinky sample application, 
follow these steps: 
1. Build the bootloader application and flash it into your board. There are 
   two ways to flash a bootloader image:
   you can use a flashloader or `updater.py`. When using `updater.py`, 
   use the following command: 
	
    updater.py COMxx bootloader.bin --force
2. Modify the blinky application's linker script (`sections.ld`) to add 
   the BOOT section as specified above.
3. Add the Post-build command to generate the `.bin` image into the blinky 
   project settings.
4. Reboot your board while holding DIO15 high to enter the bootloader update 
   mode. Once you set DIO15 back to low, the Evaluation Board LED will be 
   steady high, indicating that the bootloader is active.
5. Using a command prompt, execute the `updater.py` using the following 
   command: 

    updater.py COMxx blinky.bin
6. You should expect the following output in the command prompt: 

    Image : "Name and Version"
    Bootloader : "Name and Version"
    an asterisk (*) is printed

Notes
-----
Sometimes the firmware in RSL10 cannot be successfully re-flashed, due to the
application going into Sleep Mode or resetting continuously (either by design 
or due to programming error). To circumvent this scenario, a software recovery
mode using DIO15 can be implemented with the following steps:

1. Connect DIO15 to ground.
2. Press the RESET button (this restarts the application and forces it into
   bootloading mode).
3. Update the program using the supplied Python script.

***********************************************************
Copyright (c) 2019 Semiconductor Components Industries, LLC
(d/b/a ON Semiconductor).
