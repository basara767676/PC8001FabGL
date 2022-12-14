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

#include "pc80menu.h"

#include <Update.h>

#include "d88.h"
#include "file-stream.h"

#ifdef DEBUG_PC80
// #define DEBUG_PC80MENU
#endif

PC80MENU::PC80MENU() {}
PC80MENU::~PC80MENU() {}

#define DISK_MODE (0)
#define PROM_MODE (1)

#define MENU_EXIT (-1)
#define MENU_TO_VM (-3)
#define MENU_CONTINUE (-2)

#define MENU_MISC_SETTINGS (0)
#define MENU_TAPE (1)
#define MENU_DRIVE_MODE (2)
#define MENU_DRIVE_0 (3)
#define MENU_DRIVE_1 (4)
#define MENU_DRIVE_2 (5)
#define MENU_DRIVE_3 (6)
#define MENU_LOAD_N80_FILE (7)
#define MENU_PC80_RESET (8)
#define MENU_PC80_HOT_START (9)
#define MENU_PC80_COLD_BOOT (10)
#define MENU_ESP32_RESTART (11)

#define MENU_FILE_MANAGER (0)
#define MENU_CPU_SPEED (1)
#define MENU_VOLUME (2)
#define MENU_PROM_MODE (3)
#define MENU_EXP_UNIT (4)
#define MENU_PCG (5)
#define MENU_BASIC_ON_RAM (6)
#define MENU_PAD_ENTER (7)
#define MENU_UPDATE_FW (8)
#define MENU_ABOUT (9)

#define MENU_CREATE_TAPE (0)
#define MENU_RENAME_TAPE (1)
#define MENU_DELETE_TAPE (2)
#define MENU_CREATE_DISK (3)
#define MENU_RENAME_DISK (4)
#define MENU_PROTECT_DISK (5)
#define MENU_DELETE_DISK (6)

int PC80MENU::menu(PC80VM *vm) {
    int rc;

    fabgl::InputBox ib;

    mVM = vm;

    ib.begin(VGA_640x480_60Hz, 640, 480, 4);
    ib.setBackgroundColor(RGB888(0, 0, 0));

    auto current = mVM->getCurrentSettings();
    auto pc80Settings = mVM->getPC80Settings();

    strcpy(mMenuTitle, "PC8001FabGL preferences");

    do {
        sprintf(mMenuItem,
                "Miscellaneous settings;TAPE: %s;DISK: %s;Drive1: %s;Drive2: %s;Drive3: %s;Drive4: %s;Load n80 "
                "file;PC-8001 reset;PC-8001 hot start;PC-8001 cold boot;ESP32 reset",
                current->tape, getMode(DISK_MODE, current->drive, pc80Settings->getDrive()), current->disk[0], current->disk[1],
                current->disk[2], current->disk[3]);
        rc = ib.menu(mMenuTitle, "Select an item", mMenuItem);
        switch (rc) {
            case MENU_MISC_SETTINGS:
                rc = miscSettings(&ib);
                if (rc == MENU_EXIT) rc = MENU_CONTINUE;
                break;
            case MENU_TAPE:
                rc = tapeSelector(&ib, current, pc80Settings);
                break;
            case MENU_DRIVE_MODE:
                pc80Settings->setDrive(!pc80Settings->getDrive());
                pc80Settings->save();
                rc = MENU_CONTINUE;
                break;
            case MENU_DRIVE_0:
                rc = diskSelector(&ib, pc80Settings, 0, current->disk[0]);
                break;
            case MENU_DRIVE_1:
                rc = diskSelector(&ib, pc80Settings, 1, current->disk[1]);
                break;
            case MENU_DRIVE_2:
                rc = diskSelector(&ib, pc80Settings, 2, current->disk[2]);
                break;
            case MENU_DRIVE_3:
                rc = diskSelector(&ib, pc80Settings, 3, current->disk[3]);
                break;
            case MENU_LOAD_N80_FILE:
                rc = loadN80File(&ib);
                break;
            case MENU_PC80_RESET:
                rc = CMD_RESET;
                break;
            case MENU_PC80_HOT_START:
                rc = CMD_HOT_START;
                break;
            case MENU_PC80_COLD_BOOT:
                rc = CMD_COLD_BOOT;
                break;
            case MENU_ESP32_RESTART:
                rc = CMD_ESP32_RESTART;
                break;
        }
    } while (rc == MENU_CONTINUE);

    ib.end();

    return rc;
}

int PC80MENU::miscSettings(fabgl::InputBox *ib) {
    auto current = mVM->getCurrentSettings();
    auto pc80Settings = mVM->getPC80Settings();

    int rc;
    do {
        sprintf(mMenuItem,
                "File Manager;CPU Speed: %s;Volume: %d;ROM area: %s;Expansion unit: %s;PCG: %S;BASIC on RAM;Behavior of PAD enter key: "
                "%s;Update firmware;About this program",
                cpuSpeedStr(current->speed), current->volume, getMode(PROM_MODE, current->prom, pc80Settings->getProm()),
                getExpUnitMode(current->expunit, pc80Settings->getExpUnit()), current->pcg ? "on" : "off",
                current->padEnter ? "Behave as equal key (=)" : "Behave as RETURN key");
        rc = ib->menu(mMenuTitle, "Select an item           ", mMenuItem);
        switch (rc) {
            case MENU_FILE_MANAGER:
                rc = fileManager(ib, current, pc80Settings);
                break;
            case MENU_CPU_SPEED:
                rc = cpuSpeed(ib, current, pc80Settings);
                break;
            case MENU_VOLUME:
                rc = changeVolume(ib, current, pc80Settings);
                break;
            case MENU_PROM_MODE:
                pc80Settings->setProm(!pc80Settings->getProm());
                pc80Settings->save();
                rc = MENU_CONTINUE;
                break;
            case MENU_EXP_UNIT:
                rc = setExpansionUnit(ib, pc80Settings);
                break;
            case MENU_PCG:
                current->pcg = !current->pcg;
                pc80Settings->setPCG(current->pcg);
                pc80Settings->save();
                mVM->getPD3301()->setPCG(current->pcg);
                rc = MENU_CONTINUE;
                break;
            case MENU_BASIC_ON_RAM:
                rc = CMD_BASIC_ON_RAM;
                break;
            case MENU_PAD_ENTER:
                current->padEnter = !current->padEnter;
                pc80Settings->setPadEnter(current->padEnter);
                mVM->getPC80KeyBoard()->setPadEnter(current->padEnter);
                pc80Settings->save();
                rc = MENU_CONTINUE;
                break;
            case MENU_UPDATE_FW:
                rc = updateFirmware(ib);
                break;
            case MENU_ABOUT:
                ib->message("PC8001FabGL 1.0.0", "GNU GENERAL PUBLIC LICENSE Version 3 https://github.com/Basara767676/PC8001FabGL",
                            nullptr);
                rc = MENU_CONTINUE;
                break;
        }
    } while (rc == MENU_CONTINUE);

    return rc;
}

int PC80MENU::diskSelector(fabgl::InputBox *ib, PC80SETTINGS *pc80Settings, int driveNo, char *driveStr) {
    if (strlen(driveStr) > 0) {
        sprintf(mMenuMsg, "Eject the disk file for drive %d", driveNo + 1);
        sprintf(mMenuItem, "Eject: %s", driveStr);
        auto value = ib->menu(mMenuTitle, mMenuMsg, mMenuItem);
        if (value == 0) {
            strcpy(driveStr, "");
            pc80Settings->setDisk(driveNo, driveStr);
            pc80Settings->save();
            mVM->getPC80S31()->closeDrive(driveNo);
        }
    } else {
        strcpy(mPath, SD_MOUNT_POINT);
        strcat(mPath, PC80DIR);
        strcat(mPath, PC80DIR_DISK);
        strcpy(mFileName, "");
        sprintf(mMenuMsg, "Select the disk file for drive %d", driveNo);

        auto rc = ib->fileSelector(mMenuMsg, "Filename: ", mPath, sizeof(mPath) - 1, mFileName, sizeof(mFileName) - 1);
        if (rc == InputResult::Enter && strlen(mFileName) > 0) {
            const char *ext = strrchr(mFileName, '.');
            if (ext == nullptr || strcasecmp(ext, ".d88")) return MENU_CONTINUE;

            strcpy(driveStr, mPath);
            strcat(driveStr, "/");
            strcat(driveStr, mFileName);
            pc80Settings->setDisk(driveNo, driveStr);
            pc80Settings->save();
            mVM->getPC80S31()->openDrive(driveNo, driveStr);
        }
    }
    return MENU_CONTINUE;
}

int PC80MENU::tapeSelector(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings) {
    if (!strcmp("", current->tape)) {
        strcpy(mPath, SD_MOUNT_POINT);
        strcat(mPath, PC80DIR);
        strcat(mPath, PC80DIR_TAPE);
        strcpy(mFileName, "");
        auto rc = ib->fileSelector("Select tape file to load", "Filename: ", mPath, sizeof(mPath) - 1, mFileName, sizeof(mFileName) - 1);
        if (rc == InputResult::Enter && strcmp("", mFileName)) {
            const char *ext = strrchr(mFileName, '.');
            if (ext == nullptr || strcasecmp(ext, ".cmt")) return MENU_CONTINUE;

            strcpy(current->tape, mPath);
            strcat(current->tape, "/");
            strcat(current->tape, mFileName);
            pc80Settings->setTape(current->tape);
            pc80Settings->save();
            mVM->getDR320()->open(current->tape);
            return MENU_EXIT;
        }
    } else {
        sprintf(mMenuItem, "Eject: %s", current->tape);
        auto value = ib->menu(mMenuTitle, "Eject the tape file", mMenuItem);
        if (value == 0) {
            strcpy(current->tape, "");
            pc80Settings->setTape(current->tape);
            pc80Settings->save();
            mVM->getDR320()->close();
        }
    }
    return MENU_CONTINUE;
}

const int PC80MENU::loadN80File(fabgl::InputBox *ib) {
    strcpy(mPath, SD_MOUNT_POINT);
    strcat(mPath, PC80DIR);
    strcat(mPath, PC80DIR_N80);
    strcpy(mFileName, "");

    auto rc = ib->fileSelector("Select the n80 file", "Filename: ", mPath, sizeof(mPath) - 1, mFileName, sizeof(mFileName) - 1);

    if (rc == InputResult::Enter && strlen(mFileName) > 0) {
        const char *ext = strrchr(mFileName, '.');
        if (ext == nullptr || strcasecmp(ext, ".n80")) return MENU_CONTINUE;

        strcat(mPath, "/");
        strcat(mPath, mFileName);
#ifdef DEBUG_PC80MENU
        Serial.println(mFileName);
#endif

        struct stat fileStat;
        if (stat(mPath, &fileStat) == -1) {
            return MENU_CONTINUE;
        }

        auto fp = fopen(mPath, "r");
        if (!fp) {
#ifdef DEBUG_PC80MENU
            Serial.printf("Open error: %s\n", mPath);
#endif
            return MENU_CONTINUE;
        }

#ifdef DEBUG_PC80MENU
        Serial.printf("%s %d\n", mPath, fileStat.st_size);
#endif

        fseek(fp, 0, SEEK_SET);
        size_t result = fread(mVM->getRAM8000(), 1, fileStat.st_size, fp);

        fclose(fp);

        if (result != fileStat.st_size) {
            return MENU_CONTINUE;
        }

#ifdef DEBUG_PC80MENU
        Serial.printf("%s loaded\n", mPath);
#endif

        return CMD_N80_FILE;
    }

    return MENU_CONTINUE;
}

int PC80MENU::updateFirmware(fabgl::InputBox *ib) {
    strcpy(mPath, SD_MOUNT_POINT);
    strcat(mPath, "/bin");
    strcpy(mFileName, "");

    auto updated = MENU_CONTINUE;
    auto rc = ib->fileSelector("Select the firmware file", "Filename: ", mPath, sizeof(mPath) - 1, mFileName, sizeof(mFileName) - 1);

    if (rc == InputResult::Enter && strlen(mFileName) > 0) {
        const char *ext = strrchr(mFileName, '.');
        if (ext == nullptr || strcasecmp(ext, ".bin")) return updated;

        strcat(mPath, "/");
        strcat(mPath, mFileName);
#ifdef DEBUG_PC80MENU
        Serial.println(mPath);
#endif

        auto r = ib->progressBox("Updating firmware", nullptr, true, 200, [&](fabgl::ProgressForm *form) {
            FileStream file;
            auto updateSize = file.open(mPath);
            if (updateSize <= 0) return updated;

            if (Update.begin(updateSize)) {
                Update.onProgress([&](unsigned int progress, unsigned int total) {
                    progress = progress / (total / 100);
                    form->update(progress, "%s", mFileName);
                });
                size_t written = Update.writeStream(file);
                if (Update.end()) {
                    if (Update.isFinished() && (written == updateSize)) {
                        ib->message("Done", "Firmware update is complete.", nullptr);
                        updated = CMD_ESP32_RESTART;
                    } else {
                        ib->message("Error", "Firmware update not finished.", nullptr);
                    }
                } else {
                    strcpy(mPath, "Error Occurred. Error #: ");
                    strcat(mPath, String(Update.getError()).c_str());
                    ib->message("Error", mPath, nullptr);
                }
            } else {
                ib->message("Error", "Not enough space to begin OTA", nullptr);
            }
            return updated;
        });
    }

    return updated;
}

int PC80MENU::changeVolume(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings) {
    mMenuItem[0] = 0;
    char temp[16];
    for (int i = 0; i < 16; i++) {
        sprintf(temp, "Volume %d;", i);
        strcat(mMenuItem, temp);
    }
    mMenuItem[strlen(mMenuItem) - 1] = 0;
    int value = ib->select("Sound volume", "Select volume", mMenuItem);
    if (0 <= value && value <= 15) {
        mVM->setVolume(value);
        current->volume = value;
        pc80Settings->setVolume(value);
        pc80Settings->save();
    }
    return MENU_CONTINUE;
}

int PC80MENU::fileManager(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings) {
    int rc = MENU_CONTINUE;

    do {
        rc = ib->select("File Manager", "Select item",
                        "Create new tape;Rename tape;Delete tape;Create new disk;Rename disk;Protect disk;Delete disk");
        switch (rc) {
            case MENU_CREATE_TAPE:
                rc = createTape(ib, current, pc80Settings);
                break;
            case MENU_RENAME_TAPE:
                rc = renameTape(ib, current, pc80Settings);
                break;
            case MENU_DELETE_TAPE:
                rc = deleteTape(ib, current, pc80Settings);
                break;
            case MENU_CREATE_DISK:
                rc = createDisk(ib, current, pc80Settings);
                break;
            case MENU_RENAME_DISK:
                rc = renameDisk(ib, current, pc80Settings);
                break;
            case MENU_PROTECT_DISK:
                rc = protectDisk(ib, current, pc80Settings);
                break;
            case MENU_DELETE_DISK:
                rc = deleteDisk(ib, current, pc80Settings);
                break;
        }
    } while (rc == MENU_CONTINUE);

    return rc;
}

int PC80MENU::createDisk(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings) {
    strcpy(mPath, SD_MOUNT_POINT);
    strcat(mPath, PC80DIR);
    strcat(mPath, PC80DIR_DISK);
    strcpy(mFileName, "");

    auto rc = MENU_CONTINUE;

    if (ib->textInput("Create a new disk", "file name", mFileName, 31, nullptr, "OK") == InputResult::Enter) {
        strcat(mPath, "/");
        strcat(mPath, mFileName);
        strcat(mPath, ".D88");
#ifdef DEBUG_PC80MENU
        Serial.println(mPath);
#endif

        FILE *fp = fopen(mPath, "rb");
        if (fp) {
            fclose(fp);
            sprintf(mPath, "File already exists: %s.D88", mFileName);
            ib->message("Error", mPath, nullptr);
            return MENU_CONTINUE;
        }

        auto buf = (uint8_t *)ps_malloc(16 * (sizeof(d88_sector_header_t) + 0x100));

        auto r = ib->progressBox("Creating a new disk", nullptr, true, 200, [&](fabgl::ProgressForm *form) {
            fp = fopen(mPath, "w");
            auto p = (d88_header_t *)buf;
            memset(p, 0, sizeof(d88_header_t));
            p->diskSize = sizeof(d88_header_t) + 40 * 2 * 16 * (sizeof(d88_sector_header_t) + 0x100);
            auto offset = sizeof(d88_header_t);
            for (int i = 0; i < 40 * 2; i++) {
                p->track[i] = offset;
                offset += (sizeof(d88_sector_header_t) + 0x100) * 16;
            }
            fwrite(p, 1, sizeof(d88_header_t), fp);
            for (int i = 0; i < 80; i++) {
                form->update((i * 100) / 80, "%s", mFileName);
                auto sector = buf;
                memset(buf, 0xff, 16 * (sizeof(d88_sector_header_t) + 0x100));
                for (int j = 0; j < 16; j++) {
                    auto header = (d88_sector_header_t *)sector;
                    memset(header, 0, sizeof(d88_sector_header_t));
                    header->geometry.c = i / 2;
                    header->geometry.h = i % 2;
                    header->geometry.r = j + 1;
                    header->geometry.n = 1;
                    header->sizeOfData = 0x100;
                    sector += sizeof(d88_sector_header_t) + 0x100;
                }
                fwrite(buf, 1, 16 * (sizeof(d88_sector_header_t) + 0x100), fp);
            }
            fclose(fp);
        });

        free(buf);

        rc = MENU_CONTINUE;

        // mount new disk
        for (int i = 0; i < 4; i++) {
            if (strlen(current->disk[i]) == 0) {
                strcpy(current->disk[i], mPath);
                pc80Settings->setDisk(i, mPath);
                pc80Settings->save();
                mVM->getPC80S31()->openDrive(i, mPath);
                rc = MENU_EXIT;
                break;
            }
        }
    }
    return rc;
}

int PC80MENU::renameDisk(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings) {
    strcpy(mPath, SD_MOUNT_POINT);
    strcat(mPath, PC80DIR);
    strcat(mPath, PC80DIR_DISK);
    strcpy(mFileName, "");

    auto rc = ib->fileSelector("Rename disk file", "Filename: ", mPath, sizeof(mPath) - 1, mFileName, sizeof(mFileName) - 1);
    if (rc == InputResult::Enter && strlen(mFileName) > 0) {
        const char *ext = strrchr(mFileName, '.');
        if (ext == nullptr || strcasecmp(ext, ".d88")) {
            strcat(mPath, "/");
            strcat(mPath, mFileName);
            ib->message("Error: not disk file", mPath, nullptr);
            return MENU_CONTINUE;
        }

        strcpy(mFileName2, "");
        if (ib->textInput("Enter new disk name", "file name", mFileName2, 31, nullptr, "OK") == InputResult::Enter) {
            ext = strrchr(mFileName2, '.');
            if (ext == nullptr || strcasecmp(ext, ".d88")) {
                strcat(mFileName2, ".d88");
            }
            strcat(mPath, "/");
            strcpy(mPath2, mPath);
            strcat(mPath, mFileName);
            strcat(mPath2, mFileName2);
            if (isMounted(mPath, current)) {
                ib->message("Error: already mounted", mPath, nullptr);
                return MENU_CONTINUE;
            }
            FILE *fp = fopen(mPath2, "r");
            if (fp) {
                fclose(fp);
                ib->message("Error: already exists", mPath2, nullptr);
                return MENU_CONTINUE;
            }
            rename(mPath, mPath2);
        }
    }
    return MENU_CONTINUE;
}

int PC80MENU::protectDisk(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings) {
    strcpy(mPath, SD_MOUNT_POINT);
    strcat(mPath, PC80DIR);
    strcat(mPath, PC80DIR_DISK);
    strcpy(mFileName, "");

    auto rc = ib->fileSelector("Delete disk file", "Filename: ", mPath, sizeof(mPath) - 1, mFileName, sizeof(mFileName) - 1);
    if (rc == InputResult::Enter && strlen(mFileName) > 0) {
        const char *ext = strrchr(mFileName, '.');
        if (ext == nullptr || strcasecmp(ext, ".d88")) {
            strcat(mPath, "/");
            strcat(mPath, mFileName);
            ib->message("Error: not disk file", mPath, nullptr);
            return MENU_CONTINUE;
        }

        strcat(mPath, "/");
        strcat(mPath, mFileName);

        if (isMounted(mPath, current)) {
            ib->message("Error: already mounted", mPath, nullptr);
            return MENU_CONTINUE;
        }

        FILE *fp = fopen(mPath, "rb+");
        if (fp) {
            uint8_t buf;
            fseek(fp, 0x1a, SEEK_SET);
            fread(&buf, 1, 1, fp);
            buf &= 0x10;
            buf ^= 0x10;
            fseek(fp, 0x1a, SEEK_SET);
            fwrite(&buf, 1, 1, fp);
            fclose(fp);

#ifdef DEBUG_PC80MENU
            Serial.printf("protect %02x", buf);
#endif
            ib->message(buf == 0 ? "Writable" : "Protected", mPath, nullptr);
        }
    }
    return MENU_CONTINUE;
}

int PC80MENU::deleteDisk(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings) {
    strcpy(mPath, SD_MOUNT_POINT);
    strcat(mPath, PC80DIR);
    strcat(mPath, PC80DIR_DISK);
    strcpy(mFileName, "");

    auto rc = ib->fileSelector("Delete disk file", "Filename: ", mPath, sizeof(mPath) - 1, mFileName, sizeof(mFileName) - 1);
    if (rc == InputResult::Enter && strlen(mFileName) > 0) {
        const char *ext = strrchr(mFileName, '.');
        if (ext == nullptr || strcasecmp(ext, ".d88")) {
            strcat(mPath, "/");
            strcat(mPath, mFileName);
            ib->message("Error: not disk file", mPath, nullptr);
            return MENU_CONTINUE;
        }

        strcat(mPath, "/");
        strcat(mPath, mFileName);

        if (isMounted(mPath, current)) {
            ib->message("Error: already mounted", mPath, nullptr);
            return MENU_CONTINUE;
        }
        if (ib->message("Do you want to delete this ?", mPath) == InputResult::Enter) {
            remove(mPath);
        }
    }
    return MENU_CONTINUE;
}

int PC80MENU::createTape(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings) {
    int rc = MENU_CONTINUE;

    strcpy(mFileName, "");

    if (ib->textInput("Create a new tape", "file name", mFileName, 31, nullptr, "OK") == InputResult::Enter) {
#ifdef DEBUG_PC80MENU
        Serial.printf("OK: %s\n", mFileName);
#endif

        strcpy(mPath, SD_MOUNT_POINT);
        strcat(mPath, PC80DIR);
        strcat(mPath, PC80DIR_TAPE);
        strcat(mPath, "/");
        strcat(mPath, mFileName);
        strcat(mPath, ".CMT");

        FILE *fp = fopen(mPath, "rb");
        if (fp) {
            fclose(fp);
            sprintf(mPath, "File already exists: %s.CMT", mFileName);
            ib->message("Error", mPath, nullptr);
        } else {
            fp = fopen(mPath, "w");
            fclose(fp);
            strcpy(current->tape, mPath);
            pc80Settings->setTape(current->tape);
            pc80Settings->save();
            mVM->getDR320()->open(current->tape);
            rc = MENU_EXIT;
        }
    } else {
#ifdef DEBUG_PC80MENU
        Serial.printf("Cancel: %s\n", mFileName);
#endif
    }

    return rc;
}

int PC80MENU::renameTape(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings) {
    strcpy(mPath, SD_MOUNT_POINT);
    strcat(mPath, PC80DIR);
    strcat(mPath, PC80DIR_TAPE);
    strcpy(mFileName, "");

    auto rc = ib->fileSelector("Rename tape file", "Filename: ", mPath, sizeof(mPath) - 1, mFileName, sizeof(mFileName) - 1);
    if (rc == InputResult::Enter && strlen(mFileName) > 0) {
        const char *ext = strrchr(mFileName, '.');
        if (ext == nullptr || strcasecmp(ext, ".cmt")) {
            strcat(mPath, "/");
            strcat(mPath, mFileName);
            ib->message("Error: not tape file", mPath, nullptr);
            return MENU_CONTINUE;
        }

        strcpy(mFileName2, "");
        if (ib->textInput("Enter new tape name", "file name", mFileName2, 31, nullptr, "OK") == InputResult::Enter) {
            ext = strrchr(mFileName2, '.');
            if (ext == nullptr || strcasecmp(ext, ".cmt")) {
                strcat(mFileName2, ".cmt");
            }
            strcat(mPath, "/");
            strcpy(mPath2, mPath);
            strcat(mPath, mFileName);
            strcat(mPath2, mFileName2);
            if (isMounted(mPath, current)) {
                ib->message("Error: already mounted", mPath, nullptr);
                return MENU_CONTINUE;
            }
            FILE *fp = fopen(mPath2, "r");
            if (fp) {
                fclose(fp);
                ib->message("Error: already exists", mPath2, nullptr);
                return MENU_CONTINUE;
            }
            rename(mPath, mPath2);
        }
    }
    return MENU_CONTINUE;
}

int PC80MENU::deleteTape(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings) {
    strcpy(mPath, SD_MOUNT_POINT);
    strcat(mPath, PC80DIR);
    strcat(mPath, PC80DIR_TAPE);
    strcpy(mFileName, "");

    auto rc = ib->fileSelector("Delete tape file", "Filename: ", mPath, sizeof(mPath) - 1, mFileName, sizeof(mFileName) - 1);
    if (rc == InputResult::Enter && strlen(mFileName) > 0) {
        const char *ext = strrchr(mFileName, '.');
        if (ext == nullptr || strcasecmp(ext, ".cmt")) {
            strcat(mPath, "/");
            strcat(mPath, mFileName);
            ib->message("Error: not tape file", mPath, nullptr);
            return MENU_CONTINUE;
        }

        strcat(mPath, "/");
        strcat(mPath, mFileName);

        if (isMounted(mPath, current)) {
            ib->message("Error: already mounted", mPath, nullptr);
            return MENU_CONTINUE;
        }
        if (ib->message("Do you want to delete this ?", mPath) == InputResult::Enter) {
            remove(mPath);
        }
    }
    return MENU_CONTINUE;
}

const char *PC80MENU::getMode(int mode, bool cur, bool next) {
    static const char *msg[][4] = {{"Disable", "Enable", "Disable (Enable when next cold boot)", "Enable (Disable when next cold boot)"},
                                   {"Use as RAM", "Use as USER ROM", "Use as RAM(Use as USER ROM when next cold boot)",
                                    "Use as USER ROM (Use as RAM when next cold boot)"}};

    int index = 0;

    if (cur) index = 0x01;
    if (cur != next) index |= 0x02;

    return msg[mode][index];
}

const char *PC80MENU::getExpUnitMode(int cur, int next) {
    static const char *msg[][3] = {{"Unused", "Unused (PC-8011 when next cold boot)", "Unused (PC-8012 when next cold boot)"},
                                   {"PC-8011 (Unused when next cold boot", "PC-8011", "PC-8011 (PC-8012 when next cold boot)"},
                                   {"PC-8012 (Unused when next cold boot)", "PC-8012 (PC-8011 when next cold boot)", "PC-8012"}};

    return msg[cur][next];
}

int PC80MENU::setExpansionUnit(fabgl::InputBox *ib, PC80SETTINGS *pc80Settings) {
    int value = ib->select("Expansion unit", "Select expansion unit", "Unused;PC-8011;PC-8012");

    if (0 <= value && value <= 2) {
        pc80Settings->setExpUnit(value);
        pc80Settings->save();
    }

    return MENU_CONTINUE;
}

const char *PC80MENU::cpuSpeedStr(int i) {
    static const char *str[10] = {"No wait", "Very very fast", "Very fast", "Fast",      "A little fast",
                                  "Normal",  "A little slow",  "Slow",      "Very slow", "Very very slow"};
    if (0 <= i && i < 10) {
        return str[i];
    } else {
        return "Unknown";
    }
}

int PC80MENU::cpuSpeed(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings) {
    mMenuItem[0] = 0;
    for (int i = 0; i < 10; i++) {
        strcat(mMenuItem, cpuSpeedStr(i));
        strcat(mMenuItem, ";");
    }
    mMenuItem[strlen(mMenuItem) - 1] = 0;
    int value = ib->select("CPU speed", "Select speed", mMenuItem);
    if (CPU_SPEED_NO_WAIT <= value && value <= CPU_SPEED_VERY_VERY_SLOW) {
        mVM->setCpuSpeed(value);
        current->speed = value;
        pc80Settings->setCpuSpeed(value);
        pc80Settings->save();
    }
    return MENU_TO_VM;
}

bool PC80MENU::isMounted(const char *fileName, pc80_settings_t *current) {
    for (int i = 0; i < 4; i++) {
        if (strcmp(fileName, current->disk[i]) == 0) return true;
    }
    return false;
}