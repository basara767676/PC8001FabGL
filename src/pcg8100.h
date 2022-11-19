#pragma once

#pragma GCC optimize("O2")

#include <cstdint>

#include "fabgl.h"

class PCG8100 {
   public:
    PCG8100();
    ~PCG8100();

    void init(uint8_t *fontROM, int volume);
    void reset(void);

    void port00(uint8_t value);
    void port01(uint8_t value);
    void port02(uint8_t value);
    void port0c(uint8_t value);
    void port0d(uint8_t value);
    void port0e(uint8_t value);
    void port0f(uint8_t value);

    void suspend(bool value);
    void beep(bool value);
    void soundMute(void);

    void setVolume(int value);

    void volumeUp(void);
    void volumeDown(void);

   private:
    uint8_t *mFontROM80;
    uint8_t *mFontROM80PCG;
    uint8_t *mFontROM40;
    uint8_t *mFontROM40PCG;

    int mPCGAddr;
    uint8_t mPCGData;

    bool mCounter[3];

    bool mBit4;
    bool mBit5;

    bool mBeep;

    bool mSoundMute;
    int mVolumeValue;

    fabgl::SoundGenerator mSoundGenerator;
    fabgl::WaveformGenerator *mSquareWaveformGenerator[4];
    bool mStatus[3];
    uint8_t mI8253Mode[3];
    uint16_t mI8253Counter[3];

    static const uint8_t mVolume[16];
    void enable(int value, bool status);
    void setCounter(int counter, int value);
    void setFrequency(int counter);
};