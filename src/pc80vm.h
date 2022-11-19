/*
    This file is part of PC8001FabGL.

    https://github.com/Basara767676/PC8001FabGL

    Copyright (C) 2022 Basara767676

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#pragma once

#pragma GCC optimize("O2")

#include <sys/stat.h>

#include "dr320.h"
#include "emudevs/Z80.h"
#include "fabgl.h"
#include "fabutils.h"
#include "i8255.h"
#include "pc80keyboard.h"
#include "pc80menu.h"
#include "pc80s31.h"
#include "pc80settings.h"
#include "pcg8100.h"
#include "pd1990.h"
#include "pd3301.h"
#include "pd8257.h"

#define SD_MOUNT_POINT "/SD"
#define PC80DIR "/pc8001"
#define PC80DIR_DISK "/disk"
#define PC80DIR_TAPE "/tape"
#define PC80DIR_N80 "/n80"

#define CMD_PC80MENU (0x1000)
#define CMD_HOT_START (0x1001)
#define CMD_RESET (0x1002)
#define CMD_COLD_BOOT (0x1003)
#define CMD_ESP32_RESTART (0x1004)
#define CMD_PCG_ON_OFF (0x1005)
#define CMD_TAPE_REWIND (0x1006)
#define CMD_TAPE_EOT (0x1007)
#define CMD_BEEP_MUTE (0x1008)
#define CMD_VOLUME_UP (0x1009)
#define CMD_VOLUME_DOWN (0x100a)
#define CMD_N80_FILE (0x100b)
#define CMD_BASIC_ON_RAM (0x100c)

// CMD 0x2000 - 0x2006
#define CMD_CPU_SPEED (0x2000)
#define CPU_SPEED_NO_WAIT (0)
#define CPU_SPEED_VERY_VERY_FAST (1)
#define CPU_SPEED_VERY_FAST (2)
#define CPU_SPEED_FAST (3)
#define CPU_SPEED_A_LITTLE_FAST (4)
#define CPU_SPEED_NORMAL (5)
#define CPU_SPEED_A_LITTLE_SLOW (6)
#define CPU_SPEED_SLOW (7)
#define CPU_SPEED_VERY_SLOW (8)
#define CPU_SPEED_VERY_VERY_SLOW (9)

typedef struct {
    int cmd;
    int data;
} cpu_cmd_t;

typedef struct {
    int cmd;
    char param[32];
} debug_cmd_t;

class PC80MENU;
class DR320;

class PC80VM {
   public:
    PC80VM();
    ~PC80VM();

    void run(void);
    void subTask(void);

    static int readByte(void *context, int address);
    static void writeByte(void *context, int address, int value);

    static int readWord(void *context, int addr);
    static void writeWord(void *context, int addr, int value);

    static int readBytePC8012(void *context, int address);
    static void writeBytePC8012(void *context, int address, int value);

    static int readWordPC8012(void *context, int addr);
    static void writeWordPC8012(void *context, int addr, int value);

    static int readIO(void *context, int address);
    static void writeIO(void *context, int address, int value);

    volatile bool mSuspending;

    bool mColumn80 = false;
    bool mLine25 = true;

    // Keyboard
    uint8_t mKeyMap[12];

    PD3301 *getPD3301(void) { return mPD3301; }
    DR320 *getDR320(void) { return mDR320; }
    PC80S31 *getPC80S31(void) { return mPC80S31; }
    PC80KeyBoard *getPC80KeyBoard(void) { return mKeyboard; }
    pc80_settings_t *getCurrentSettings(void) { return mSettings; }
    PC80SETTINGS *getPC80Settings(void) { return mPC80Settings; }
    uint8_t *getRAM8000(void) { return mRAM8000; }
    void setVolume(int value);
    void vmControl(PC80VM *vm);
    void setCpuSpeed(int speed);

   private:
    PC80KeyBoard *mKeyboard;

    PD3301 *mPD3301;
    PD8257 *mPD8257;
    I8255 *mI8255;
    PD1990 *mPD1990;
    PC80S31 *mPC80S31;
    PCG8100 *mPCG8100;
    DR320 *mDR320;

    PC80MENU *mPC80MENU;

    TaskHandle_t mTaskHandle;

    uint8_t *mRAM0000;
    uint8_t *mRAM6000;
    uint8_t *mRAM8000;

    uint8_t *mRAM;

    int mUnit;

    int mMemMode;

    uint8_t *mBasicROM;
    uint8_t *mFontROM;
    uint8_t *mDiskROM;
    uint8_t *mUserROM;

    uint8_t *mExtRAM;

    bool mHasUserROM;

    PC80SETTINGS *mPC80Settings;
    pc80_settings_t *mSettings;

    char mRootDir[14];

    fabgl::Z80 *mPD780C;
    static void pc80Task(void *pvParameters);

    // Port 30h
    uint8_t mPort30;

    // Port 40;
    uint8_t mPort40In;
    uint8_t mPort40Out;
    bool mVRTC;

    uint8_t *m0000Bank;

    // Port 53h
    uint8_t mPort53;

    // Port e2h
    uint8_t mPortE2;

    char mPath[512];
    char mFileName[64];

    int mKbCmd;

    volatile int mWait;
    volatile bool mNoWait;

    void memDump(uint8_t *mRAM, int address, int offset);

    int init(void);
    int initFileSystem(void);

    int initMemory(void);
    int initFont(void);
    int initDisk(void);

    void fontGen(void);

    uint8_t *lalloc(size_t size, bool internal = false, const char *fileName = nullptr, bool require = true);
    int getAddress(char *p);
    void coldBoot(void);
    void reset(void);
    void dumpReg(fabgl::Z80_STATE *state);

    void suspend(bool value, bool pd3301 = true);

    static void keyboardCallBack(void *arg, int value);
    void printHeapMemory(void);

    static void esp32Restart(PC80VM *vm);
};
