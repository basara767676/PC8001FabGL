#include "pcg8100.h"

#include <Arduino.h>

#ifdef DEBUG_PC80
// #define DEBUG_PCG8100
#endif

#define I8253_MODE_COUNTER_LATCH 0
#define I8253_MODE_LO_BYTE 1
#define I8253_MODE_HI_BYTE 2
#define I8253_MODE_LO_HI_BYTES 3
#define I8253_MODE_LO_HI_BYTES2 4

PCG8100::PCG8100() {
    mFontROM80 = 0;
    mFontROM80PCG = 0;
    mFontROM40 = 0;
    mFontROM40PCG = 0;

    mPCGAddr = 0;
    mPCGData = 0;

    for (int i = 0; i < 3; i++) {
        mCounter[i] = false;
        mStatus[i] = false;
        mI8253Mode[i] = I8253_MODE_COUNTER_LATCH;
        mI8253Counter[i] = 0;
    }

    mBit4 = false;
    mBit5 = false;

    mBeep = false;
}

PCG8100::~PCG8100() {}

const uint8_t PCG8100::mVolume[16] = {0, 8, 17, 25, 34, 42, 51, 59, 68, 76, 85, 93, 102, 110, 119, 127};

void PCG8100::init(uint8_t *fontROM, int volume) {
    mFontROM80 = fontROM;
    mFontROM80PCG = fontROM + 0x1400;

    mFontROM40 = fontROM + 0x1e00;
    mFontROM40PCG = fontROM + 0x1e00 + 0x2800;

    mSoundMute = false;

    setVolume(volume);
    mSoundGenerator.play(true);

    for (int i = 0; i < 3; i++) {
        mSquareWaveformGenerator[i] = new fabgl::SquareWaveformGenerator();
        mSoundGenerator.attach(mSquareWaveformGenerator[i]);
        enable(i, false);
        mCounter[i] = false;
        mStatus[i] = false;
        mI8253Mode[i] = I8253_MODE_COUNTER_LATCH;
        mI8253Counter[i] = 0;
    }

    // Beep
    mSquareWaveformGenerator[3] = new SquareWaveformGenerator();
    mSoundGenerator.attach(mSquareWaveformGenerator[3]);
    mSquareWaveformGenerator[3]->setFrequency(2400);
    mSquareWaveformGenerator[3]->enable(false);
}

void PCG8100::reset(void) {
    for (int i = 0; i < 4; i++) {
        mSquareWaveformGenerator[i]->enable(false);
    }
    mSoundMute = false;
}

void PCG8100::setVolume(int value) {
#ifdef DEBUG_PCG8100
    Serial.printf("PCG8100 Volume: %d\n", value);
#endif
    if (0 <= value && value <= 15) {
        mSoundGenerator.setVolume(mVolume[value]);
        mVolumeValue = value;
    }
}

void PCG8100::volumeUp() { setVolume(mVolumeValue + 1); }

void PCG8100::volumeDown() { setVolume(mVolumeValue - 1); }

void PCG8100::enable(int value, bool status) {
    mStatus[value] = status;
    mSquareWaveformGenerator[value]->enable(status);
}

void PCG8100::port00(uint8_t value) { mPCGData = value; }
void PCG8100::port01(uint8_t value) { mPCGAddr = (mPCGAddr & 0xff00) | value; }
void PCG8100::port02(uint8_t value) {
    static uint8_t fontConv[16] = {0x00, 0x03, 0x0c, 0x0f, 0x30, 0x33, 0x3c, 0x3f, 0xc0, 0xc3, 0xcc, 0xcf, 0xf0, 0xf3, 0xfc, 0xff};
    mPCGAddr = mPCGAddr & 0xfcff | (value & 0x03) << 8;  // 0x07

    bool curBit4 = value & 0x10;
    bool curBit5 = value & 0x20;

    if (mBit4 && !curBit4) {
        if (mBit5 && !curBit5) {
            auto address = mPCGAddr + 0x80 * 8;
            auto offset = (address / 8) * 10 + address % 8;
            *(mFontROM80PCG + offset) = *(mFontROM80 + offset);
            offset = (address / 8) * 20 + address % 8;
            *(mFontROM40PCG + offset) = *(mFontROM40 + offset);
            *(mFontROM40PCG + offset + 10) = *(mFontROM40 + offset + 10);
        } else {
            auto address = mPCGAddr + 0x80 * 8;
            auto offset = (address / 8) * 10 + address % 8;
            *(mFontROM80PCG + offset) = mPCGData;
            offset = (address / 8) * 20 + address % 8;
            *(mFontROM40PCG + offset) = fontConv[(mPCGData & 0xf0) >> 4];
            *(mFontROM40PCG + offset + 10) = fontConv[(mPCGData & 0x0f)];
        }
    }
    mBit4 = curBit4;
    mBit5 = curBit5;

    // PCG Sound enable/disable
    mCounter[0] = value & 0x08;
    mCounter[1] = value & 0x40;
    mCounter[2] = value & 0x80;

    for (int i = 0; i < 3; i++) {
        if (mCounter[i] != mStatus[i]) {
            enable(i, mCounter[i]);
        }
    }
}

void PCG8100::port0c(uint8_t value) { setCounter(0, value); }

void PCG8100::port0d(uint8_t value) { setCounter(1, value); }

void PCG8100::port0e(uint8_t value) { setCounter(2, value); }

void PCG8100::port0f(uint8_t value) {
    int counter = (value & 0xc0) >> 6;
    int data = (value & 0x30) >> 4;

    if (counter < 3) {
        switch (data) {
            case 0:
                mI8253Mode[counter] = I8253_MODE_COUNTER_LATCH;
                break;
            case 1:
                mI8253Mode[counter] = I8253_MODE_LO_BYTE;
                break;
            case 2:
                mI8253Mode[counter] = I8253_MODE_HI_BYTE;
                break;
            case 3:
                mI8253Mode[counter] = I8253_MODE_LO_HI_BYTES;
                break;
        }
    }
}

void PCG8100::setCounter(int counter, int value) {
    value &= 0xff;
    switch (mI8253Mode[counter]) {
        case I8253_MODE_LO_BYTE:
            mI8253Counter[counter] = (mI8253Counter[counter] & 0xff00) | value;
            setFrequency(counter);
            break;
        case I8253_MODE_HI_BYTE:
            mI8253Counter[counter] = (mI8253Counter[counter] & 0x00ff) | (value << 8);
            setFrequency(counter);
            break;
        case I8253_MODE_LO_HI_BYTES:
            mI8253Counter[counter] = (mI8253Counter[counter] & 0xff00) | value;
            mI8253Mode[counter] = I8253_MODE_LO_HI_BYTES2;
            break;
        case I8253_MODE_LO_HI_BYTES2:
            mI8253Counter[counter] = (mI8253Counter[counter] & 0x00ff) | (value << 8);
            mI8253Mode[counter] = I8253_MODE_LO_HI_BYTES;
            setFrequency(counter);
            break;
    }
}

void PCG8100::setFrequency(int counter) {
    int freq = 0;
    if (mI8253Counter[counter] > 0) {
        freq = 4000000 / (int)mI8253Counter[counter];
    }
    if (freq > 15000) {
        freq = 15000;
    }
    mSquareWaveformGenerator[counter]->setFrequency(freq);
}

void PCG8100::suspend(bool value) {
    if (value) {
        mSoundGenerator.play(false);
    } else {
        mSoundGenerator.play(true);
    }
}

void PCG8100::beep(bool value) {
    if (mBeep != value) {
        mSquareWaveformGenerator[3]->enable(value);
        mBeep = value;
    };
}

void PCG8100::soundMute(void) {
    mSoundMute = !mSoundMute;
    if (mSoundMute) {
        mSoundGenerator.setVolume(0);
    } else {
        mSoundGenerator.setVolume(mVolume[mVolumeValue]);
    }
}