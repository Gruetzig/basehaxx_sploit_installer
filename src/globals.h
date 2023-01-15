//
// Created by Nba_Yoh on 06/06/2016.
//

#ifndef BASEHAXX_SPLOIT_INSTALLER_GLOBALS_H
#define BASEHAXX_SPLOIT_INSTALLER_GLOBALS_H

#include <3ds.h>

extern Handle save_session;
extern FS_Archive save_archive;
extern char status[256];

// http://3dbrew.org/wiki/Nandrw/sys/SecureInfo_A
static const char regions[7][4] = {
        "JPN",
        "USA",
        "EUR",
        "EUR",
        "CHN",
        "KOR",
        "TWN"
};

#endif //BASEHAXX_SPLOIT_INSTALLER_GLOBALS_H
