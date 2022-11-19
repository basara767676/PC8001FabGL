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

#include "pc80settings.h"

#include "d88.h"
#include "fabgl.h"

#ifdef DEBUG_PC80
// #define DEBUG_PC80_SETTINGS
#endif

#define TYPE_BOOL 0
#define TYPE_INT 1
#define TYPE_STRING 2
#define TYPE_RGB222 3

#define SETTING_FILE_NAME "settings.ini"

setting_type_t PC80SETTINGS::settings[12] = {{"PC80S31", TYPE_BOOL, &mSettings.drive, nullptr},
                                             {"PROM", TYPE_BOOL, &mSettings.prom, nullptr},
                                             {"PCG", TYPE_BOOL, &mSettings.pcg, nullptr},
                                             {"PADENTER", TYPE_BOOL, &mSettings.padEnter, nullptr},
                                             {"EXPUNIT", TYPE_INT, &mSettings.expunit, &expUnitValidate},
                                             {"VOLUME", TYPE_INT, &mSettings.volume, &volumeValidate},
                                             {"SPEED", TYPE_INT, &mSettings.speed, &speedValidate},
                                             {"TAPE", TYPE_STRING, &mSettings.tape, nullptr},
                                             {"DISK0", TYPE_STRING, &mSettings.disk[0], nullptr},
                                             {"DISK1", TYPE_STRING, &mSettings.disk[1], nullptr},
                                             {"DISK2", TYPE_STRING, &mSettings.disk[2], nullptr},
                                             {"DISK3", TYPE_STRING, &mSettings.disk[3], nullptr}};

char PC80SETTINGS::fileName[64];
pc80_settings_t PC80SETTINGS::mSettings;

int PC80SETTINGS::init(char *rootDir) {
    mSettings.drive = false;
    mSettings.padEnter = false;
    mSettings.volume = 8;
    mSettings.prom = false;
    mSettings.expunit = 0;
    mSettings.pcg = false;
    mSettings.speed = 4;

    char **items[] = {&mSettings.rom, &mSettings.tape, &mSettings.disk[0], &mSettings.disk[1], &mSettings.disk[2], &mSettings.disk[3]};

    for (int i = 0; i < 6; i++) {
        *items[i] = (char *)ps_malloc(256);
        strcpy(*items[i], "");
    }

    strcpy(fileName, rootDir);
    strcat(fileName, SETTING_FILE_NAME);

    auto fp = fopen(fileName, "r");
    if (!fp) {
        auto fp = fopen(fileName, "w");
        if (!fp) return -1;
        fclose(fp);
        save();
        return 0;
    }
    load(fp);
    fclose(fp);

    auto update = false;

    char *fileName[5] = {mSettings.tape, mSettings.disk[0], mSettings.disk[1], mSettings.disk[2], mSettings.disk[3]};
    for (int i = 0; i < 5; i++) {
        if (!PC80D88::exists(fileName[i])) {
            strcpy(fileName[i], "");
            update = true;
        }
    }

    if (update) save();

    return 0;
}

int PC80SETTINGS::load(FILE *fp) {
#ifdef DEBUG_PC80_SETTINGS
    Serial.println("PC80SETTINGS::load");
#endif
    char buf[256];
    while (fgets(buf, sizeof(buf), fp)) {
        char *pos;
        if ((pos = strchr(buf, '\n')) != NULL) *pos = '\0';
        auto num = sizeof(settings) / sizeof(settings[0]);
        for (int i = 0; i < num; i++) {
            auto name = settings[i].name;
            if (!strncmp(buf, name, strlen(name)) && (strlen(buf) > strlen(name) + 1)) {
                switch (settings[i].type) {
                    case TYPE_BOOL:
                        loadBool(buf, i);
                        break;
                    case TYPE_INT:
                        loadInt(buf, i);
                        break;
                    case TYPE_STRING:
                        loadString(buf, i);
                        break;
                }
            }
        }
    }
#ifdef DEBUG_PC80_SETTINGS
    Serial.println("PC80SETTINGS::load end");
#endif
    return 0;
}

void PC80SETTINGS::loadBool(char *buf, int i) {
#ifdef DEBUG_PC80_SETTINGS
    Serial.printf("buf: %s\n", buf);
#endif
    auto name = settings[i].name;
    auto value = (bool *)settings[i].value;
    setBool(&buf[strlen(name) + 1], value);
#ifdef DEBUG_PC80_SETTINGS
    Serial.printf("%s=%s\n", name, *value ? "true" : "false");
#endif
}

void PC80SETTINGS::setBool(char *buf, bool *b) {
    for (int i = 0; i < strlen(buf); i++) {
        buf[i] = tolower((unsigned char)buf[i]);
    }
    if (!strcmp(buf, "true")) {
        *b = true;
    } else if (!strcmp(buf, "false")) {
        *b = false;
    }
}

void PC80SETTINGS::loadInt(char buf[], int i) {
    auto name = settings[i].name;
    auto value = (int *)settings[i].value;
    *value = atoi(&buf[strlen(name) + 1]);
    settings[i].validate(value);
#ifdef DEBUG_PC80_SETTINGS
    Serial.printf("%s=%d\n", name, *value);
#endif
}

void PC80SETTINGS::loadString(char *buf, int i) {
    auto name = settings[i].name;
    auto value = *((char **)settings[i].value);
    strcpy(value, &buf[strlen(name) + 1]);
#ifdef DEBUG_PC80_SETTINGS
    Serial.printf("%s=%s\n", name, value);
#endif
}

int PC80SETTINGS::save(void) {
#ifdef DEBUG_PC80_SETTINGS
    Serial.printf("PC80SETTINGS::save %s\n", fileName);
#endif

    auto fp = fopen(fileName, "w");
    if (!fp) return -1;
    auto num = sizeof(settings) / sizeof(settings[0]);
    for (int i = 0; i < num; i++) {
        switch (settings[i].type) {
            case TYPE_BOOL:
                saveBool(fp, i);
                break;
            case TYPE_INT:
                saveInt(fp, i);
                break;
            case TYPE_STRING:
                saveString(fp, i);
                break;
        }
    }
    fclose(fp);

#ifdef DEBUG_PC80_SETTINGS
    Serial.println("PC80SETTINGS::save end");
#endif
    return 0;
}

void PC80SETTINGS::saveBool(FILE *fp, int i) {
    auto name = settings[i].name;
    auto value = (bool *)(settings[i].value);
#ifdef DEBUG_PC80_SETTINGS
    Serial.printf("saveBool %d %s %s\n", i, name, *value ? "true" : "false");
#endif
    fprintf(fp, "%s=%s\n", name, boolString(*value));
}

void PC80SETTINGS::saveInt(FILE *fp, int i) {
    auto name = settings[i].name;
    auto value = (int *)settings[i].value;
    settings[i].validate(value);
    fprintf(fp, "%s=%d\n", name, *value);
}

void PC80SETTINGS::saveString(FILE *fp, int i) {
    auto name = settings[i].name;
    auto value = (*((char **)settings[i].value));
    fprintf(fp, "%s=%s\n", name, value);
}

const char *PC80SETTINGS::getROMFileName(void) {
    auto p = getROM();
    if (p == nullptr) return "";

    auto q = strrchr(p, '/');
    if (q == nullptr) return p;

    return q + 1;
}

const char *PC80SETTINGS::getROM(void) { return strcmp(&mSettings.rom[0], "") ? nullptr : &mSettings.rom[0]; }

void PC80SETTINGS::setROM(const char *fileName) { strcpy(&mSettings.rom[0], fileName); }

void PC80SETTINGS::volumeValidate(void *arg) {
    auto value = (int *)arg;
    if (*value < 0 || *value > 15) {
        *value = 7;
    }
}

void PC80SETTINGS::tvramValidate(void *arg) {
    auto value = (int *)arg;
    if (*value < 0 || *value > 3) {
        *value = 1;
    }
}

void PC80SETTINGS::expUnitValidate(void *arg) {
    auto value = (int *)arg;
    if (*value < 0 || *value > 2) {
        *value = 0;
    }
}

void PC80SETTINGS::speedValidate(void *arg) {
    auto value = (int *)arg;
    if (*value < 0 || *value > 8) {
        *value = 4;
    }
}
