# PC8001FabGL

## What is PC8001FabGL

The PC8001FabGL is an emulator of PC-8001 running on ESP32 with [FabGL](http://www.fabglib.org/).
It emulates not only the main unit but also peripheral devices such as disk units.

### Emulated hardware

| Hardware | Description                                                  |
| -------- | ------------------------------------------------------------ |
| PC-8001  | Main unit                                                    |
| PC-8011  | Expansion unit with 32KB RAM                                 |
| PC-8012  | Expansion Unit with 128KB RAM                                |
| PC-80S31 | Dual mini disk units (for supporting d88 file)               |
| PC-80S32 | Dual mini disk units for expansion (for supporting d88 file) |
| DR320    | Data recoder (for supporting cmt file)                       |
| PCG-8100 | Programmable character generator unit for PC-8001            |

## Requirements

### Hardware

-   LILYGO [TTGO VGA32 V1.4](http://www.lilygo.cn/prod_view.aspx?TypeId=50063&Id=1083)
-   PicoSoft [ORANGE-ESPer](http://www.picosoft.co.jp/ESP32/index.html) with [ESP32-WROVER-E](https://akizukidenshi.com/catalog/g/gM-15674/)
-   PS/2 Japanese 106/109 keyboard
-   Display with VGA port (VGA 640×480 60Hz)
-   VGA cable
-   Micro SD card

### Software

-   [Arduino ESP32 Version 2.0.5](https://github.com/espressif/arduino-esp32/releases/tag/2.0.5)
-   [FabGL v1.0.9](https://github.com/fdivitto/FabGL/releases/tag/v1.0.9)
-   [Arduino IDE](https://www.arduino.cc/) (for building PC8001FabGL)

### ROM images

| File name    | Requirement | File size     |
| ------------ | ----------- | ------------- |
| PC-8001.ROM  | Required    | 24,576 bytes  |
| PC-8001.FON  | Required    | 2,048 bytes   |
| PC-80S31.ROM | Optional    | 2,048 bytes   |
| USER.ROM     | Optional    | 8,192 bytes   |

## How to make micro SD card image

In a micro SD card, create a `pc8001` folder in the root. Under the folder,  
put ROM images and create some folders as shown:

```
/
+-- pc8001/
    +-- PC-8001.ROM
    +-- PC-8001.FON
    +-- PC-80S31.ROM (optional)
    +-- USER.ROM (optional)
    +-- disk/
        +--- *.d88
    +-- tape/
        +--- *.cmt
    +-- n80/
        +--- *.n80
    +-- bin/
        +--- *.bin
```

The files with `.ROM` extension are ROM images. The `disk` is a folder putting d88 files.
The `tape` is a folder putting cmt files. The `n80` is a folder putting n80 files.
The `bin` is the folder where bin files that are compiled sketches put.

## How to build PC8001FabGL

### Set up Arduino IDE

First of all, set up the Arduino IDE on your machine. You can get it from [the Arduino web site](https://www.arduino.cc/en/software).

### Download PC8001FabGL

Download PC8001FabGL from `https://github.com/Basara767676/PC8001FabGL`. Put it in your Sketchbook location.

### Install Arduino ESP32 Version 2.0.5

Run the Arduino IDE. Select the `Preferences` from the `File` menu. Add the following URL in the Additional Boards Manager URLs.

```
https://dl.espressif.com/dl/package_esp32_index.json
```

Next, select the `Board` from the `Tools` menu and open the Boards Manager. Search for `ESP32` and install the esp32 by Espressif
Systems board with version 2.0.5.

### Install FabGL v1.0.9

Select the `Manage Libraries...` from the `Tools` menu. Search for `FabGL` and install FabGL by Fabrizio Di Vittorio version v1.0.9.

### Specify the board information

Specify the board information from the `Tools` menu.

 -   Board:            ESP32 Dev Module
 -   Partition Scheme: Hug APP (3MB No OTA/1MB SPIFFS)
 -   PSRAM:            Enabled

### Upload the PC8001FabGL

Compile the sketch of PC8001FabGL and upload it into your ESP32.

## Operation

### PC-8001 keyboard mapping to PS/2 keyboard

| PC-8001 Keyboard | PS/2 Japanese 106/109 keyboard                                                 |
| ---------------- | ------------------------------------------------------------------------------ |
| STOP             | Esc                                                                            |
| ESC              | TAB or 半角/全角\|漢字                                                         |
| CTRL             | Ctrl or Caps Lock                                                              |
| カナ             | カタカナひらがな\|漢字 (Turn on LED of Scroll Lock on Keyboard when enabling.) |
| GRPH             | Alt                                                                            |
| INS DEL          | BackSpace or Delete                                                            |
| HOME CLR         | Home                                                                           |
| PAD =            | Shift + PAD Enter                                                              |
| SHIFT + UP       | Down arrow                                                                     |
| SHIFT + RIGHT    | Left arrow                                                                     |

## Keys and Key combination

The keys and key combination to operating an emulator are as shown:

| Keys and Key combination | Description                                                 |
| ------------------------ | ----------------------------------------------------------- |
| F9                       | Whether to mute BEEP and PCG sound.                         |
| F10                      | Whether to enable PCG.                                      |
| F12                      | Enter preferences mode.                                     |
| Ctrl + Alt + Delete      | Reset PC-8001 with keeping memory contents.                 |
| Ctrl + Alt + Insert      | Power on reset                                              |
| Ctrl + Alt + r           | 40K BASIC.                                                  |
| Win + Left arrow         | Rewind tape. (Move tape to BOT)                             |
| Win + Right arrow        | Move tape to EOT.                                           |
| Win + Up arrow           | Up sound volume.                                            |
| Win + Dwon arrow         | Down sound volume.                                          |
| Win + (from 0 to 9)      | 0: No wait, 1: Very very fast, 5: Normal, 9: Very very slow | 

## Preferences

Press `F12` key to enter the preferences mode. To exit this mode, press `Esc` key.

| Item                   | Description                                                         |
| ---------------------- | ------------------------------------------------------------------- |
| Miscellaneous settings | Move to miscellaneous settings.                                     |
| TAPE                   | Specify a cmt file to be mounted on the tape unit.                  |
| DISK                   | Conect or disconect disk units.                                     |
| Drive1                 | Specify a d88 file to be mounted on the drive unit 1.               |
| Drive2                 | Specify a d88 file to be mounted on the drive unit 2.               |
| Drive3                 | Specify a d88 file to be mounted on the drive unit 3.               |
| Drive4                 | Specify a d88 file to be mounted on the drive unit 4.               |
| Load n80 file          | Specify a n80 file. Switch to N-BASIC mode when using this feature. |
| PC-8001 reset          | Reset PC-8001 with keeping memory contents.                         |
| PC-8001 hot start      | Hot start PC-8001 with keeping memory contents.                     |
| PC-8001 cold boot      | Power on reset PC-8001 without keeping memory contents.             |
| ESP32 reset            | Reset ESP32                                                         |

### Miscellaneous settings

| Item                      | Description                                                       |
| ------------------------- | ----------------------------------------------------------------- |
| File Manager              | Enter File Manager menu.                                          |
| CPU Speed                 | Set CPU Speed.                                                    | 
| Volume                    | Set volume level                                                  |
| ROM area                  | Whether to use the 6000h-7fffh area as USER ROM area or RAM area. |
| Expansion unit            | Whether to connect expansion unit. (Unused, PC-8011 or PC-8012)   |
| PCG                       | Whereer to enable PCG-8100.                                       |
| BASIC on RAM              | Enable 40KB BASIC.                                                |
| Behavior of PAD enter key | Specify behavior of PAD enter key as `=` key or `RETURN` key.     |
| Update firmware           | Update firmware for this emulator.                                |

### File Manager

| Item                       | Description                                                       |
| -------------------------- | ----------------------------------------------------------------- |
| Create a new tape          | Create a new empty cmt file.                                      |
| Rename a tape              | Rename a cmt file.                                                |
| Delete a tape              | Delete a cmt file.                                                |
| Create a new disk          | Create a new d88 file that is a blank disk formatted for 2D disk. |
| Rename a disk              | Rename a d88 file.                                                |
| Protect a disk             | Whether to set a d88 file to protected or writable.               |
| Delete a disk              | Delete a d88 file.                                                |

## PC-8011 / PC-8012

-   PC-8011 only supports 32KB RAM, no other peripherals.
-   PC-8012 only supports 128KB RAM, no other peripherals.
-   When connecting PC-8011 or PC-8012, the USER ROM area cannot be used as RAM area.

## 40KB BASIC

You may expands the free memory of N-BASIC to 40KB when using USER ROM area as RAM area or connecting PC-8011 or
PC-8012. To enable 40KB BASIC, you can press `Ctrl` `Alt` + `r` key combination or select the `BASIC on RAM` item
in Miscellaneous settings. It is recommended to perform this operation immediately after startup N-BASIC or Disk
BASIC. The N-BASIC ROM area is protected when using USER ROM area as RAM area and is writable when connecting
PC-8011 or PC-8012.

## Architecture

### Tasks

| Task           | Core | Priority    |
| -------------- | ---- | ----------- |
| PC-8001 task   | 0    | 1           |
| PC-80S31 task  | 1    | 1           |
| Keyboard task  | 1    | 1           |
| FabGL tasks    | 1    | more than 1 |

This program runs without the Wifi and Bluetooth feature.

## Dependencies

| Software                                                                             | OSS license                                          |
| ------------------------------------------------------------------------------------ | -------------------------------------------------|
| [FabGL v1.0.9](https://github.com/fdivitto/fabgl)                                    | [GNU GENERAL PUBLIC LICENSE Version 3](https://www.gnu.org/licenses/gpl-3.0.en.html) |
| [Arduino ESP32 Version 2.0.5](https://github.com/espressif/arduino-esp32/tree/2.0.5) | [GNU LESSER GENERAL PUBLIC LICENSE Version 2.1](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html) |

## OSS license

GNU GENERAL PUBLIC LICENSE Version 3 (GNU GPLv3)

## Copyright

Copyright (C) 2022 Basara767676
