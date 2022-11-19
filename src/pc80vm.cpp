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

#include "pc80vm.h"

#include "pc80error.h"

#ifdef DEBUG_PC80
// #define DEBUG_PC80VM
#endif

#define EXP_UNIT_NONE (0)
#define EXP_UNIT_PC8011 (1)
#define EXP_UNIT_PC8012 (2)

PC80VM::PC80VM() {}
PC80VM::~PC80VM() {}

int PC80VM::init(void) {
    initFileSystem();

    mPC80Settings = new PC80SETTINGS;
    mPC80Settings->init(mRootDir);
    mSettings = mPC80Settings->get();

    mPD3301 = new PD3301;
    mPD3301->init(&mPort40In);
    PC80ERROR::setDisplayController(mPD3301->getDisplayController());

    mPD3301->run();

    initMemory();
    initFont();
    initDisk();

    mPD3301->setMemory(mRAM, mFontROM);

    mPD780C = new fabgl::Z80;

    mPD8257 = new PD8257;
    mPD8257->init(this);

    mI8255 = new I8255;
    mI8255->init(I8255_PC8001);

    mPD1990 = new PD1990;

    mPC80S31 = new PC80S31;
    mPC80S31->init(this, mDiskROM, mI8255);

    mPCG8100 = new PCG8100;
    mPCG8100->init(mFontROM, mSettings->volume);

    mDR320 = new DR320;

    mKeyboard = new PC80KeyBoard;
    mKeyboard->init(&mKeyMap[0], PC80VM::keyboardCallBack, this);

    mPC80MENU = new PC80MENU;

    coldBoot();

#ifdef DEBUG_PC80VM
    Serial.println("PC80VM init completed");
#endif
    return 0;
}

void PC80VM::coldBoot(void) {
    if (mSettings) free(mSettings);
    mSettings = mPC80Settings->get();

    mUnit = mSettings->expunit;

    for (int i = 0; i < 0xa000; i += 4) *(uint32_t *)(mRAM + i) = 0xff00ff00;
    for (int i = 0xa000; i < 0xc000; i += 4) *(uint32_t *)(mRAM + i) = 0x00ff00ff;
    for (int i = 0xc000; i < 0x10000; i += 4) *(uint32_t *)(mRAM + i) = 0xffffffff;

    mPortE2 = 0xf0;

    mNoWait = false;
    mWait = 6;

    setCpuSpeed(mSettings->speed);

    if (mUnit == EXP_UNIT_PC8012) {
        if (!mExtRAM) {
            mExtRAM = (uint8_t *)ps_malloc(1024 * 128);
        }
        for (int i = 0; i < 0x20000; i += 4) *(uint32_t *)(mExtRAM + i) = 0xff00ff00;

        mPD780C->setCallbacks(this, readBytePC8012, writeBytePC8012, readWordPC8012, writeWordPC8012, readIO, writeIO);
    } else {
        mPD780C->setCallbacks(this, readByte, writeByte, readWord, writeWord, readIO, writeIO);
    }

    fontGen();

    reset();
}

void PC80VM::reset(void) {
    mPC80S31->reset();
    mKeyboard->reset();

    for (int i = 0; i < 4; i++) {
        if (strlen(mSettings->disk[i]) > 0) {
            mPC80S31->openDrive(i, mSettings->disk[i]);
        }
    }

    mDR320->open(mSettings->tape);

    mPCG8100->reset();
    mPCG8100->setVolume(mSettings->volume);

    mRAM0000 = mBasicROM;
    if (mSettings->prom || mUnit == EXP_UNIT_PC8011 || mUnit == EXP_UNIT_PC8012) {
        mRAM6000 = mUserROM;
    } else {
        mRAM6000 = mRAM + 0x6000;
    }

    mMemMode = 0;

    mPD3301->reset();
    mPD3301->setPCG(mSettings->pcg);

    // Keyboard
    for (int i = 0; i < sizeof(mKeyMap); i++) {
        mKeyMap[i] = 0xff;
    }

    mPort40In = 0;
    if (mSettings->drive) {
        mPort40In &= 0xf7;  // PC-80S31 / PC-8011 / PC-8012
    } else {
        mPort40In |= 0x08;  // no disk drives
    }
    mPort40In |= 0x04;  // CMT

    mVRTC = false;
}

void PC80VM::run(void) {
#ifdef DEBUG_PC80VM
    Serial.println("pc-8801 start.");
#endif

    disableCore0WDT();

    init();

    xTaskCreateUniversal(&pc80Task, "pc80Task", 4096, this, 1, &mTaskHandle, PRO_CPU_NUM);

    mPD8257->run();
    mKeyboard->run();

#ifdef DEBUG_PC80VM
    Serial.println("setup() end.");
#endif
}

void PC80VM::keyboardCallBack(void *arg, int value) {
    auto vm = (PC80VM *)arg;
    vm->mKbCmd = value;
    vm->mSuspending = true;
}

void IRAM_ATTR PC80VM::pc80Task(void *pvParameters) {
#ifdef DEBUG_PC80VM
    Serial.println("pc80Task");
#endif

    auto vm = (PC80VM *)pvParameters;

    vm->mSuspending = false;

    vm->mPD780C->reset();
    vm->mPD780C->setPC(0);

    int cycles = 0;
    uint32_t previousTime = micros();
    while (true) {
        if (vm->mSuspending) {
            vm->vmControl(vm);
            previousTime = micros();
            cycles = 0;
        }

        cycles += vm->mPD780C->step();
        vm->mPD3301->updateVRAMcahce();

        if (!vm->mNoWait & cycles > 100) {
            uint32_t currentTime = micros();
            int diff = currentTime - previousTime;
            if (diff < 0) diff = (0xFFFFFFFF - previousTime) + currentTime;
            previousTime = currentTime;

            int wait = (cycles - diff / 10) / vm->mWait;
            if (wait > 0) delayMicroseconds(wait);
            cycles = 0;
        }
    }
}

void PC80VM::suspend(bool suspend, bool pd3301) {
    mSuspending = suspend;
    mKeyboard->suspend(suspend);
    if (pd3301) mPD3301->suspend(suspend);
    mPCG8100->suspend(suspend);
}

int IRAM_ATTR PC80VM::readWord(void *context, int addr) { return readByte(context, addr) | (readByte(context, addr + 1) << 8); }

void IRAM_ATTR PC80VM::writeWord(void *context, int addr, int value) {
    writeByte(context, addr, value & 0xFF);
    writeByte(context, addr + 1, value >> 8);
}

int IRAM_ATTR PC80VM::readByte(void *context, int address) {
    auto vm = (PC80VM *)context;

    int value;
    if (address < 0x6000) {
        value = vm->mRAM0000[address];
    } else if (address < 0x8000) {
        value = vm->mRAM6000[address - 0x6000];
    } else {
        value = vm->mRAM8000[address - 0x8000];
    }

    return value;
}

void IRAM_ATTR PC80VM::writeByte(void *context, int address, int value) {
    auto vm = (PC80VM *)context;

    if (address < 0x6000) {
        vm->mRAM[address] = value;
    } else if (address < 0x8000) {
        vm->mRAM[address] = value;
    } else {
        vm->mRAM8000[address - 0x8000] = value;
    }
}

int IRAM_ATTR PC80VM::readWordPC8012(void *context, int addr) {
    return readBytePC8012(context, addr) | (readBytePC8012(context, addr + 1) << 8);
}

void IRAM_ATTR PC80VM::writeWordPC8012(void *context, int addr, int value) {
    writeBytePC8012(context, addr, value & 0xFF);
    writeBytePC8012(context, addr + 1, value >> 8);
}

int IRAM_ATTR PC80VM::readBytePC8012(void *context, int address) {
    auto vm = (PC80VM *)context;

    int value;
    if (address < 0x6000) {
        if ((vm->mPortE2 & 0x0f) == 0) {
            value = vm->mRAM0000[address];
        } else if (vm->mPortE2 & 0x01) {
            value = vm->mExtRAM[address];
        } else if (vm->mPortE2 & 0x02) {
            value = vm->mExtRAM[address + 0x8000];
        } else if (vm->mPortE2 & 0x04) {
            value = vm->mExtRAM[address + 0x10000];
        } else if (vm->mPortE2 & 0x08) {
            value = vm->mExtRAM[address + 0x18000];
        }
    } else if (address < 0x8000) {
        if ((vm->mPortE2 & 0x0f) == 0) {
            value = vm->mRAM6000[address - 0x6000];
        } else if (vm->mPortE2 & 0x01) {
            value = vm->mExtRAM[address];
        } else if (vm->mPortE2 & 0x02) {
            value = vm->mExtRAM[address + 0x8000];
        } else if (vm->mPortE2 & 0x04) {
            value = vm->mExtRAM[address + 0x10000];
        } else if (vm->mPortE2 & 0x08) {
            value = vm->mExtRAM[address + 0x18000];
        }
    } else {
        value = vm->mRAM8000[address - 0x8000];
    }

    return value;
}

void IRAM_ATTR PC80VM::writeBytePC8012(void *context, int address, int value) {
    auto vm = (PC80VM *)context;

    if (address < 0x8000) {
        if (vm->mPortE2 & 0x10) {
            vm->mExtRAM[address] = value;
        }
        if (vm->mPortE2 & 0x20) {
            vm->mExtRAM[address + 0x8000] = value;
        }
        if (vm->mPortE2 & 0x40) {
            vm->mExtRAM[address + 0x10000] = value;
        }
        if (vm->mPortE2 & 0x80) {
            vm->mExtRAM[address + 0x18000] = value;
        }
    } else {
        vm->mRAM8000[address - 0x8000] = value;
    }
}

int PC80VM::readIO(void *context, int address) {
    auto vm = (PC80VM *)context;
    auto pc = vm->mPD780C->getPC();

    switch (address & 0xff) {
        case 0x00:
            return vm->mKeyMap[0];
        case 0x01:
            return vm->mKeyMap[1];
        case 0x02:
            return vm->mKeyMap[2];
        case 0x03:
            return vm->mKeyMap[3];
        case 0x04:
            return vm->mKeyMap[4];
        case 0x05:
            return vm->mKeyMap[5];
        case 0x06:
            return vm->mKeyMap[6];
        case 0x07:
            return vm->mKeyMap[7];
        case 0x08:
            return vm->mKeyMap[8];
        case 0x09:
            return vm->mKeyMap[9];
        case 0x20:
        case 0x22:
        case 0x24:
        case 0x26:
        case 0x28:
        case 0x2a:
        case 0x2c:
        case 0x2e:
            return vm->mDR320->readData();
        case 0x21:
        case 0x23:
        case 0x25:
        case 0x27:
        case 0x29:
        case 0x2b:
        case 0x2d:
        case 0x2f:
            return vm->mDR320->readStatus();
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
        case 0x38:
        case 0x39:
        case 0x3a:
        case 0x3b:
        case 0x3c:
        case 0x3d:
        case 0x3e:
        case 0x3f:
            return vm->mPort30;
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4a:
        case 0x4b:
        case 0x4c:
        case 0x4d:
        case 0x4e:
        case 0x4f:
            // VRTC is updated in PD3301::drawScanline.
            vm->mPort40In = (vm->mPort40In & 0xef) | vm->mPD1990->read();  // PD1990 calender clock
            return vm->mPort40In;
        case 0x50:
            return vm->mPD3301->inPort50();
        case 0x51:
            return vm->mPD3301->inPort51();
        case 0x68:
            return vm->mPD8257->inPort68();
        case 0xe2:
            return ~vm->mPortE2;
            break;
        case 0xfc:  // Mini disk unit Port A
            return vm->mI8255->in(I8255_PORT_A);
        case 0xfd:  // Mini disk unit Port B
            return vm->mI8255->in(I8255_PORT_B);
        case 0xfe:  // Mini disk unit Port C
            return vm->mI8255->in(I8255_PORT_C);
        case 0xff:  // Mini disk unit control port
            return vm->mI8255->in(I8255_PORT_CONTROL);
        default:
#ifdef DEBUG_PC80VM
            Serial.printf("Read non-implemeted port %02x\n", address);
#endif
            return 0;
    }
    return 0;
}

void PC80VM::writeIO(void *context, int address, int value) {
    auto vm = (PC80VM *)context;

    switch (address & 0xff) {
        case 0x00:
            vm->mPCG8100->port00(value);
            break;
        case 0x01:
            vm->mPCG8100->port01(value);
            break;
        case 0x02:
            vm->mPCG8100->port02(value);
            break;
        case 0x0c:
            vm->mPCG8100->port0c(value);
            break;
        case 0x0d:
            vm->mPCG8100->port0d(value);
            break;
        case 0x0e:
            vm->mPCG8100->port0e(value);
            break;
        case 0x0f:
            vm->mPCG8100->port0f(value);
            break;
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x17:
        case 0x18:
        case 0x19:
        case 0x1a:
        case 0x1b:
        case 0x1c:
        case 0x1d:
        case 0x1e:
        case 0x1f:
            vm->mPD1990->write(0x10, value);
            break;
        case 0x20:
        case 0x22:
        case 0x24:
        case 0x26:
        case 0x28:
        case 0x2a:
        case 0x2c:
        case 0x2e:
            vm->mDR320->writeData(value);
            break;
        case 0x21:
        case 0x23:
        case 0x25:
        case 0x27:
        case 0x29:
        case 0x2b:
        case 0x2d:
        case 0x2f:
            vm->mDR320->modeCommand(value);
            break;
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
        case 0x38:
        case 0x39:
        case 0x3a:
        case 0x3b:
        case 0x3c:
        case 0x3d:
        case 0x3e:
        case 0x3f:
            vm->mPort30 = value & 0xff;
            vm->mColumn80 = value & 0x01;
            vm->mPD3301->setCloumn80(vm->mColumn80);
            vm->mDR320->systemControl(value);
            break;
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4a:
        case 0x4b:
        case 0x4c:
        case 0x4d:
        case 0x4e:
        case 0x4f:
            vm->mPCG8100->beep(value & 0x20);
            vm->mPD1990->write(0x40, value);
            vm->mPort40Out = value;
            break;
        case 0x50:
            vm->mPD3301->crtcData(value);
            break;
        case 0x51:
            vm->mPD3301->crtcCmd(value);
            break;
        case 0x60:
            vm->mPD8257->dmaAddress(0, value);
            break;
        case 0x61:
            vm->mPD8257->dmaTerminalCount(0, value);
            break;
        case 0x62:
            vm->mPD8257->dmaAddress(1, value);
            break;
        case 0x63:
            vm->mPD8257->dmaTerminalCount(1, value);
            break;
        case 0x64:
            vm->mPD8257->dmaAddress(2, value);
            break;
        case 0x65:
            vm->mPD8257->dmaTerminalCount(2, value);
            break;
        case 0x66:
            vm->mPD8257->dmaAddress(3, value);
            break;
        case 0x67:
            vm->mPD8257->dmaTerminalCount(3, value);
            break;
        case 0x68:
            vm->mPD8257->dmaCmd(value);
            break;
        case 0xe0:  // PC-8011 mode 0
            if (vm->mUnit == EXP_UNIT_PC8011) {
                vm->mRAM0000 = vm->mBasicROM;
                vm->mRAM6000 = vm->mUserROM;
                vm->mMemMode = 0;
            }
            break;
        case 0xe1:  // PC-8011 mode 1 (no effect)
            // vm->mMemMode = 1;
            break;
        case 0xe2:
            if (vm->mUnit == EXP_UNIT_PC8011) {  // PC-8011 mode 2
                vm->mRAM0000 = vm->mRAM;
                vm->mRAM6000 = vm->mRAM + 0x6000;
                vm->mMemMode = 2;
            } else if (vm->mUnit == EXP_UNIT_PC8012) {
                vm->mPortE2 = value;
            }
            break;
        case 0xe3:  // PC-8011 mode 3 (as same as mode 0)
            if (vm->mUnit == EXP_UNIT_PC8011) {
                vm->mRAM0000 = vm->mBasicROM;
                vm->mRAM6000 = vm->mUserROM;
                vm->mMemMode = 0;
            }
            break;
        case 0xfc:  // Mini disk unit Port A
            vm->mI8255->out(I8255_PORT_A, value & 0xff);
            break;
        case 0xfd:  // Mini disk unit Port B
            vm->mI8255->out(I8255_PORT_B, value & 0xff);
            break;
        case 0xfe:  // Mini disk unit Port C
            vm->mI8255->out(I8255_PORT_C, value & 0xff);
            break;
        case 0xff:  // Mini disk unit control port
            vm->mI8255->out(I8255_PORT_CONTROL, value & 0xff);
            break;
        default:
#ifdef DEBUG_PC80VM
            Serial.printf("Write non-implemeted port %02x %02x\n", address, value);
#endif
            break;
    }
}

void PC80VM::vmControl(PC80VM *vm) {
    auto cmd = vm->mKbCmd;

    if (cmd == CMD_PC80MENU) {
        vm->suspend(true);
        cmd = vm->mPC80MENU->menu(vm);
        vm->suspend(false);
        if (cmd == -1) return;
    }

    switch (cmd) {
        case CMD_BASIC_ON_RAM:
            if (vm->mUnit == EXP_UNIT_PC8012) {
                memcpy(vm->mExtRAM, vm->mBasicROM, 0x6000);
                vm->writeIO(vm, 0xe2, 0x11);
            } else if (vm->mUnit == EXP_UNIT_PC8011) {
                memcpy(vm->mRAM, vm->mBasicROM, 0x6000);
                vm->writeIO(vm, 0xe2, 0);
            } else if (vm->mSettings->prom) {
                break;
            }
            vm->mPD780C->reset();
            vm->mPD780C->setPC(0x17e9);
            vm->mPD780C->writeRegWord(Z80_HL, 0x6000);
            break;
        case CMD_N80_FILE:
            vm->mPD780C->reset();
            vm->mPD780C->setPC(0xff3d);
            vm->mPD780C->writeRegWord(Z80_SP, *(uint16_t *)&vm->mRAM[0xff3e]);
            break;
        case CMD_HOT_START:
            vm->reset();
            vm->mPD780C->reset();
            vm->mPD780C->setPC(8);
            break;
        case CMD_RESET:
            vm->reset();
            vm->mPD780C->reset();
            vm->mPD780C->setPC(0);
            break;
        case CMD_COLD_BOOT:
            vm->coldBoot();
            vm->mPD780C->reset();
            vm->mPD780C->setPC(0);
            break;
        case CMD_ESP32_RESTART:
            esp32Restart(vm);
            break;
        case CMD_PCG_ON_OFF:
            vm->mSettings->pcg = !vm->mSettings->pcg;
            vm->mPD3301->setPCG(vm->mSettings->pcg);
            break;
        case CMD_TAPE_REWIND:
            vm->mDR320->rewind();
            break;
        case CMD_TAPE_EOT:
            vm->mDR320->eot();
            break;
        case CMD_BEEP_MUTE:
            vm->mPCG8100->soundMute();
            break;
        case CMD_VOLUME_UP:
            vm->mPCG8100->volumeUp();
            break;
        case CMD_VOLUME_DOWN:
            vm->mPCG8100->volumeDown();
            break;
        default:
            if (CMD_CPU_SPEED + CPU_SPEED_NO_WAIT <= cmd && cmd <= CMD_CPU_SPEED + CPU_SPEED_VERY_VERY_SLOW) {
                vm->setCpuSpeed(cmd - CMD_CPU_SPEED);
            }
            break;
    }
    vm->suspend(false, false);
}

void PC80VM::subTask(void) {
    if (mDiskROM) {
        mPC80S31->run();
    }
}

int PC80VM::initFileSystem(void) {
    strcpy(mRootDir, SD_MOUNT_POINT);
    strcat(mRootDir, PC80DIR);

    auto ret = FileBrowser::mountSDCard(false, SD_MOUNT_POINT);
    if (!ret) {
#ifdef DEBUG_PC80VM
        Serial.println("SD Mount failed");
#endif
        return -1;
    }

    const char *dirName[3] = {PC80DIR_DISK, PC80DIR_TAPE, PC80DIR_N80};

    FileBrowser dir(mRootDir);

    for (int i = 0; i < 3; i++) {
        char name[32];
        strcpy(&name[0], dirName[i]);
        if (!dir.exists(&name[1])) {
            dir.makeDirectory(&name[1]);
        }
    }

    strcat(mRootDir, "/");

    return ret;
}

int PC80VM::initMemory(void) {
    mRAM = lalloc(0x10000, false);

    mRAM0000 = mRAM;

    mRAM6000 = mRAM + 0x6000;
    mRAM8000 = mRAM + 0x8000;

    mBasicROM = lalloc(0x6000, false, "PC-8001.ROM");

    mRAM0000 = mBasicROM;

    mUserROM = lalloc(0x2000, false, "USER.ROM", false);

    if (mUserROM) {
        mHasUserROM = true;
    } else {
        mUserROM = lalloc(0x2000, false);
        mHasUserROM = false;
    }
    mRAM6000 = mUserROM;

    mMemMode = 0;

    mExtRAM = nullptr;

    return 0;
}

int PC80VM::initFont(void) {
    auto font = lalloc(2048, false, "PC-8001.FON");

    mFontROM = lalloc(10 * 256 * 3 * 3, true);
    memset(mFontROM, 0, 10 * 256 * 3 * 3);

    auto src = font;
    auto dest = mFontROM;
    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < 8; j++) {
            *dest = *src;
            src++;
            dest++;
        }
        *dest = 0;
        dest++;
        *dest = 0;
        dest++;
    }
    free(font);
    return 0;
}

void PC80VM::fontGen(void) {
    static uint8_t fontConv[16] = {0x00, 0x03, 0x0c, 0x0f, 0x30, 0x33, 0x3c, 0x3f, 0xc0, 0xc3, 0xcc, 0xcf, 0xf0, 0xf3, 0xfc, 0xff};

    auto p = mFontROM + 10 * 256;
    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < 4; j++) {
            if ((i >> j) & 0x01) {
                *(p + j * 2) |= 0xf0;
                *(p + j * 2 + 1) |= 0xf0;
            }
            if ((i >> (j + 4)) & 0x01) {
                *(p + j * 2) |= 0x0f;
                *(p + j * 2 + 1) |= 0x0f;
            }
        }
        p += 10;
    }

    memcpy(mFontROM + (10 * 256) * 2, mFontROM, 10 * 256);

    auto src = mFontROM;
    auto dest = mFontROM + (10 * 256) * 3;
    for (int i = 0; i < 256 * 3; i++) {
        for (int j = 0; j < 10; j++) {
            *(dest + j) = fontConv[(*(src + j) & 0xf0) >> 4];
            *(dest + j + 10) = fontConv[(*(src + j) & 0x0f)];
        }
        src += 10;
        dest += 20;
    }
}

int PC80VM::initDisk(void) {
#ifdef DEBUG_PC80VM
    Serial.println("initDisk");
#endif

    auto diskROM = lalloc(2048, false, "PC-80S31.ROM", false);

    if (diskROM) {
        mDiskROM = lalloc(0x8000);
        memcpy(mDiskROM, diskROM, 2048);
        free(diskROM);
    } else {
        mDiskROM = nullptr;
    }

    return 0;
}

// load file with memory allocation

uint8_t *PC80VM::lalloc(size_t size, bool internal, const char *fileName, bool require) {
    auto mem = (uint8_t *)(internal ? heap_caps_malloc(size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL) : ps_malloc(size));
    if (!mem) {
        PC80ERROR::dialog("Memory allocation error");
        return nullptr;
    }
#ifdef DEBUG_PC80VM
    Serial.printf("memory allocated: 0x%x bytes ", size);
#endif
    memset(mem, 0, size);

    if (fileName == nullptr) {
#ifdef DEBUG_PC80VM
        Serial.println("");
#endif
        return mem;
    }

    char fName[32];
    strcpy(fName, mRootDir);
    strcat(fName, fileName);

    struct stat fileStat;
    if (stat(fName, &fileStat) == -1) {
        if (require) {
            PC80ERROR::dialog("%s not found", fileName);
        }
        return nullptr;
    }

    if (size != fileStat.st_size) {
#ifdef DEBUG_PC80VM
        Serial.printf("File size error: %s\n", fName);
#endif
        if (require) {
            PC80ERROR::dialog("File size error: %s %d %d", fileName, size, fileStat.st_size);
        }
        free(mem);
        return nullptr;
    }

    auto fp = fopen(fName, "rb");
    if (!fp) {
        if (require) {
            PC80ERROR::dialog("Open error: %s", fileName);
        }
        free(mem);
        return nullptr;
    }

    fseek(fp, 0, SEEK_SET);
    size_t result = fread(mem, 1, fileStat.st_size, fp);

    fclose(fp);

    if (result != fileStat.st_size) {
        PC80ERROR::dialog("Read error: %s", fileName);
        free(mem);
        return nullptr;
    }

#ifdef DEBUG_PC80VM
    Serial.printf("%s is loaded %d bytes\n", fileName, size);
#endif

    return mem;
}

void PC80VM::setVolume(int value) { mPCG8100->setVolume(value); }

void PC80VM::setCpuSpeed(int speed) {
    mNoWait = false;
    mSettings->speed = speed;

    switch ((speed)) {
        case CPU_SPEED_NO_WAIT:
            mNoWait = true;
            break;
        case CPU_SPEED_VERY_VERY_FAST:
            mWait = 22;
            break;
        case CPU_SPEED_VERY_FAST:
            mWait = 13;
            break;
        case CPU_SPEED_FAST:
            mWait = 9;
            break;
        case CPU_SPEED_A_LITTLE_FAST:
            mWait = 7;
            break;
        case CPU_SPEED_NORMAL:
            mWait = 6;
            break;
        case CPU_SPEED_A_LITTLE_SLOW:
            mWait = 5;
            break;
        case CPU_SPEED_SLOW:
            mWait = 4;
            break;
        case CPU_SPEED_VERY_SLOW:
            mWait = 3;
            break;
        case CPU_SPEED_VERY_VERY_SLOW:
            mWait = 2;
            break;
        default:
            mWait = 6;
            break;
    }
}

void PC80VM::esp32Restart(PC80VM *vm) {
    vm->mKeyboard->reset();
    vm->mPC80S31->eject();
    vm->mDR320->close();
    ESP.restart();
}