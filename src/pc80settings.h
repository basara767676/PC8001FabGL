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

#include <cstdio>
#include <cstring>

#include "fabgl.h"

typedef struct {
    bool prom;
    bool drive;
    bool padEnter;
    bool pcg;
    int volume;
    int expunit;
    int speed;
    char *rom;
    char *tape;
    char *disk[4];
} pc80_settings_t;

typedef struct {
    const char *name;
    int type;
    void *value;
    void (*validate)(void *);
} setting_type_t;

class PC80SETTINGS {
   public:
    static int init(char *rootDir);
    static int save(void);

    static void setDrive(bool b) { mSettings.drive = b; }
    static bool getDrive(void) { return mSettings.drive; }

    static void setPadEnter(bool b) { mSettings.padEnter = b; }
    static bool getPadEnter(void) { return mSettings.padEnter; }

    static void setProm(bool b) { mSettings.prom = b; }
    static bool getProm(void) { return mSettings.prom; }

    static void setExpUnit(int value) { mSettings.expunit = value; }
    static int getExpUnit(void) { return mSettings.expunit; }

    static void setVolume(int vol) { mSettings.volume = vol; }
    static int getVolume(void) { return mSettings.volume; }

    static void setCpuSpeed(int vol) { mSettings.speed = vol; }
    static int getCpuSpeed(void) { return mSettings.speed; }

    static void setPCG(bool value) { mSettings.pcg = value; }
    static int getPCG(void) { return mSettings.pcg; }

    static void setTape(const char *fileName) { strcpy(mSettings.tape, fileName); }
    static void setDisk(const int index, const char *fileName) {
        switch (index) {
            case 0:
                strcpy(mSettings.disk[0], fileName);
                break;
            case 1:
                strcpy(mSettings.disk[1], fileName);
                break;
            case 2:
                strcpy(mSettings.disk[2], fileName);
                break;
            case 3:
                strcpy(mSettings.disk[3], fileName);
                break;
        }
    }

    static pc80_settings_t *get(void) {
        auto dest = (pc80_settings_t *)heap_caps_malloc(sizeof(pc80_settings_t), MALLOC_CAP_SPIRAM);
        if (dest != nullptr) {
            memcpy(dest, &mSettings, sizeof(pc80_settings_t));
        }
        return dest;
    }

    static const char *getROMFileName(void);
    static const char *getROM(void);
    static void setROM(const char *fileName);

   private:
    static pc80_settings_t mSettings;

    static setting_type_t settings[12];
    static char fileName[64];

    static void loadBool(char *buf, int i);
    static void loadInt(char buf[], int i);
    static void loadString(char *buf, int i);

    static int load(FILE *fp);
    static void saveBool(FILE *fp, int i);
    static void saveInt(FILE *fp, int i);
    static void saveString(FILE *fp, int i);

    static const char *boolString(bool b) { return b ? "true" : "false"; }
    static void setBool(char *buf, bool *b);

    static void volumeValidate(void *arg);
    static void tvramValidate(void *arg);
    static void expUnitValidate(void *arg);
    static void speedValidate(void *arg);
};