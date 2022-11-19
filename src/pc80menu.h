#pragma once

#pragma GCC optimize("O2")

#include "pc80settings.h"
#include "pc80vm.h"

class PC80VM;

class PC80MENU {
   public:
    PC80MENU();
    ~PC80MENU();

    int menu(PC80VM *vm);

   private:
    char mMenuTitle[64];
    char mMenuMsg[64];
    char mPath[512];
    char mFileName[64];
    char mPath2[512];
    char mFileName2[64];
    char mMenuItem[1024];

    PC80VM *mVM;

    int diskSelector(fabgl::InputBox *ib, PC80SETTINGS *pc80Settings, int drive, char *driveStr);
    int tapeSelector(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings);

    const char *getMode(int mode, bool cur, bool next);
    const char *getExpUnitMode(int cur, int next);
    const int loadN80File(fabgl::InputBox *ib);

    int miscSettings(fabgl::InputBox *ib);
    int updateFirmware(fabgl::InputBox *ib);

    int changeVolume(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings);

    int fileManager(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings);

    int createDisk(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings);
    int renameDisk(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings);
    int protectDisk(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings);
    int deleteDisk(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings);

    int createTape(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings);
    int renameTape(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings);
    int deleteTape(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings);

    int setExpansionUnit(fabgl::InputBox *ib, PC80SETTINGS *pc80Settings);
    int cpuSpeed(fabgl::InputBox *ib, pc80_settings_t *current, PC80SETTINGS *pc80Settings);
    const char *cpuSpeedStr(int i);

    bool isMounted(const char *fileName, pc80_settings_t *current);
};