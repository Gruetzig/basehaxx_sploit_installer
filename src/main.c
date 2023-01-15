//
// Created by Nba_Yoh on 01/06/2016.
// Modified by aspargas2 on 12/30/2022
//

#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include <string.h>
#include "globals.h"
#include "save.h"
#include "blz.h"

#define VERSION_14 0x1C70

Handle save_session;
FS_Archive save_archive;
char status[256];

typedef enum
{
    STATE_NONE,
    STATE_INITIALIZE,
    STATE_INITIAL,
    STATE_SELECT_GAME,
    //STATE_SELECT_FIRMWARE,
    STATE_READ_PAYLOAD,
    STATE_INSTALL_PAYLOAD,
    STATE_INSTALLED_PAYLOAD,
    STATE_ERROR,
} state_t;

Result initFs()
{
    save_archive = 0;
    fsInit();

    Result ret = srvGetServiceHandleDirect(&save_session, "fs:USER");

    if(R_SUCCEEDED(ret))
        ret = FSUSER_Initialize(save_session);

    if(R_FAILED(ret))
        snprintf(status, sizeof(status) - 1, "An error occured! Failed to get a fs:USER session.\n    Error code: %08lX", ret);

    return ret;
}

/*Result initHttpc()
{
    Result ret = 0;

    ret = httpcInit(0);
    if(R_FAILED(ret))
        snprintf(status, sizeof(status) - 1, "An error occured! Failed to initialize httpc.\n    Error code: %08lX", ret);

    return ret;
}*/

Result getConfig(int* firmware_version)
{
    Result ret = 0;

    OS_VersionBin nver_versionbin, cver_versionbin;
    ret = osGetSystemVersionData(&nver_versionbin, &cver_versionbin);
    if(R_FAILED(ret))
    {
        snprintf(status, sizeof(status) - 1, "An error occured! Failed to get the system version.\n    Error code: %08lX", ret);
        return ret;
    }

    ret = cfguInit();
    if(R_FAILED(ret))
    {
        snprintf(status, sizeof(status) - 1, "An error occured! Failed to initialize cfgu.\n    Error code: %08lX", ret);
        return ret;
    }

    u8 region = 0;
    ret = CFGU_SecureInfoGetRegion(&region);
    if(R_FAILED(ret))
    {
        snprintf(status, sizeof(status) - 1, "An error occured! Failed to get the system region.\n    Error code: %08lX", ret);
        return ret;
    }

    cfguExit();

    bool is_new3ds = 0;
    APT_CheckNew3DS(&is_new3ds);

    firmware_version[0] = is_new3ds;
    firmware_version[5] = region;

    firmware_version[1] = cver_versionbin.mainver;
    firmware_version[2] = cver_versionbin.minor;
    firmware_version[3] = cver_versionbin.build;
    firmware_version[4] = nver_versionbin.mainver;

    return ret;
}

Result getProgramID(u64* id)
{
    Result ret = APT_GetProgramID(id);
    if(R_FAILED(ret))
        snprintf(status, sizeof(status) - 1, "An error occured! Failed to get the program ID for the current process.\n    Error code: %08lX", ret);

    return ret;
}

Result getGameVersion(u64 program_id, char* gameversion, u16* gameversion_id)
{
    Result ret = 0;

    AM_TitleEntry update_title;

    u64 update_program_id = 0;
    if(((program_id >> 32) & 0xFFFF) == 0) update_program_id = program_id | 0x0000000E00000000ULL;

    if(update_program_id)
    {
        ret = amInit();
        if(R_FAILED(ret))
        {
            snprintf(status, sizeof(status) - 1, "An error occured! Failed to initialize AM.\n    Error code: %08lX", ret);
            return ret;
        }
gameversion_id
                memcpy(gameversion, "1.4", 3);
                *gameversion_id = VERSION_14;
            }
            else
            {
                snprintf(status, sizeof(status) - 1, "Unsupported game version detected, please try again with a supported version.\n    Supported version : 1.0 / 1.4\n");
                return ret;
            }
        }
    }

    return 0;
}

int main()
{
    gfxInitDefault();
    gfxSet3D(false);

    PrintConsole topConsole, bottomConsole;
    consoleInit(GFX_TOP, &topConsole);
    consoleInit(GFX_BOTTOM, &bottomConsole);

    consoleSelect(&topConsole);
    consoleClear();

    state_t current_state = STATE_NONE;
    state_t next_state = STATE_INITIALIZE;

    char gametitle[3];
    char gameversion[4];
    u16 gameversion_id = 0;

    static char top_text[2048];
    //top_text[0] = '\0';

    char top_text_tmp[256];

    //int firmware_version[6] = {0};
    //int firmware_selected_value = 0;

    u8* payload_buffer = NULL;
    u32 payload_size = 0;

    u64 program_id = 0;

    while(aptMainLoop())
    {
        hidScanInput();
        if(hidKeysDown() & KEY_START) break;

        // transition function
        if(next_state != current_state)
        {
            memset(top_text_tmp, 0, sizeof(top_text_tmp));

            switch(next_state)
            {
                case STATE_INITIALIZE:
                    strncat(top_text, "Initializing...please wait...\n\n", sizeof(top_text) - 1);
                    break;
                case STATE_INITIAL:
                    strcat(top_text, "Welcome to the (quite dirty) basehaxx installer!\nThis installer does NOT support digital, it could overwrite unrelated cart saves :D\nPlease proceed with caution, as you might lose data if you don't.You may press START at any time to return to menu.\nThanks to smealum and SALT team for the installer code base!\n                           Press A to continue.\n\n");
                    break;
                case STATE_SELECT_GAME;
                    strcat(top_text, "Select your game...\n");
                /*case STATE_SELECT_FIRMWARE:
                    strcat(top_text, "Please select your console's firmware version.\nOnly select NEW 3DS if you own a New 3DS (XL).\nD-Pad to select, A to continue.\n");
                    break;
                case STATE_DOWNLOAD_PAYLOAD:
                    snprintf(top_text, sizeof(top_text) - 1,"%s\n\n\nDownloading payload...\n", top_text);
                    break;*/
                case STATE_READ_PAYLOAD:
                    strncat(top_text,"\n\n\nReading payload...\n", sizeof(top_text) - 1);
                    break;
                case STATE_INSTALL_PAYLOAD:
                    strcat(top_text, "Installing payload...\n");
                    break;
                case STATE_INSTALLED_PAYLOAD:
                    strcat(top_text, "Done ! basehaxx was successfully installed.");
                    break;
                case STATE_ERROR:
                    strcat(top_text, "Looks like something went wrong. :(\n");
                    break;
                default:
                    break;
            }

            if(top_text_tmp[0]) strncat(top_text, top_text_tmp, sizeof(top_text) - 1);
            current_state = next_state;
        }

        consoleSelect(&topConsole);
        printf("\x1b[0;%dHbasehaxx installer \"PASLR is useless\" edition\n\n", (50 - 46) / 2);
        printf(top_text);

        switch(current_state)
        {
            case STATE_INITIALIZE:
            {
                if(initFs())
                {
                    next_state = STATE_ERROR;
                    break;
                }
                if(R_FAILED(romfsInit())) {
                    next_state = STATE_ERROR;
                    break;
                }
                /*if(initHttpc())
                {
                    next_state = STATE_ERROR;
                    break;
                }

                if(getConfig(firmware_version))
                {
                    next_state = STATE_ERROR;
                    break;
                }*/

                if(getProgramID(&program_id))
                {
                    next_state = STATE_ERROR;
                    break;
                }

                /*if (program_id == 0x000400000011C400) {
                    strcpy(gametitle, "OR");
                } else if (program_id == 0x000400000011C500) {
                    strcpy(gametitle, "AS");
                } else {
                    printf("\n\nBad program ID: %016llX\n\nYou most likely need to run the installer under\nOR/AS as the hb takeover title", program_id);
                    next_state = STATE_ERROR;
                    break;
                }

                if(getGameVersion(program_id, gameversion, &gameversion_id))
                {
                    next_state = STATE_ERROR;
                    break;
                }*/

                next_state = STATE_INITIAL;
            }
            break;

            case STATE_INITIAL:
            {
                if(hidKeysDown() & KEY_A)
                    next_state = STATE_SELECT_GAME;
            }
            break;

            case STATE_SELECT_GAME: 
            {
                if(hidKeysDown() & KEY_LEFT) && (gametitle == "OR") strcpy(gametitle, "AS");
                if(hidKeysDown() & KEY_RIGHT) && (gametitle == "AS") strcpy(gametitle, "OR");
                if(hidKeysDown() & KEY_A) if(hidKeysDown() & KEY_A) {
                    if (gametitle == "OR") {
                        program_id = 0x000400000011C400;
                    }
                    else if (gametitle == "AS") {
                        program_id = 0x000400000011C500; 
                    }
                    else {
                        next_state = STATE_ERROR;
                        break;
                    }
                    if(getGameVersion(program_id, gameversion, &gameversion_id))
                    {
                        next_state = STATE_ERROR;
                        break;
                    }
                    else
                    {
                        next_state = STATE_READ_PAYLOAD;
                        break;
                    }
                }
                printf("Selected game: %s" gametitle);

            }
            /*case STATE_SELECT_FIRMWARE:
            {
                if(hidKeysDown() & KEY_LEFT) firmware_selected_value--;
                if(hidKeysDown() & KEY_RIGHT) firmware_selected_value++;

                if(firmware_selected_value < 0) firmware_selected_value = 0;
                if(firmware_selected_value > 5) firmware_selected_value = 5;

                if(hidKeysDown() & KEY_UP) firmware_version[firmware_selected_value]++;
                if(hidKeysDown() & KEY_DOWN) firmware_version[firmware_selected_value]--;

                int firmware_maxnum = 256;
                if(firmware_selected_value == 0) firmware_maxnum = 2;
                if(firmware_selected_value == 5) firmware_maxnum = 7;

                if(firmware_version[firmware_selected_value] < 0) firmware_version[firmware_selected_value] = 0;
                if(firmware_version[firmware_selected_value] >= firmware_maxnum) firmware_version[firmware_selected_value] = firmware_maxnum - 1;

                if(hidKeysDown() & KEY_A) next_state = STATE_DOWNLOAD_PAYLOAD;

                int offset = 26;
                if(firmware_selected_value)
                {
                    offset += 7;

                    for(int i = 1; i < firmware_selected_value; i++)
                    {
                        offset += 2;
                        if(firmware_version[i] >= 10) offset++;
                    }
                }

                printf((firmware_version[firmware_selected_value] < firmware_maxnum - 1) ? "%*s^%*s" : "%*s-%*s", offset, " ", 50 - offset - 1, " ");
                printf("      Selected firmware: %s %d-%d-%d-%d %s  \n", firmware_version[0] ? "New3DS" : "Old3DS", firmware_version[1], firmware_version[2], firmware_version[3], firmware_version[4], regions[firmware_version[5]]);
                printf((firmware_version[firmware_selected_value] > 0) ? "%*sv%*s" : "%*s-%*s", offset, " ", 50 - offset - 1, " ");
            }
            break;*/

            case STATE_READ_PAYLOAD:
            {
                FILE* file = fopen("sdmc:/basehaxx_payload.bin", "r");
                printf("Using payload from SD\n");
                if (file == NULL) {
                    printf("\nFailed to open otherapp payload\n");
                    next_state = STATE_ERROR;
                    break;
                }

                fseek(file, 0, SEEK_END);
                payload_size = ftell(file);
                fseek(file, 0, SEEK_SET);

                payload_buffer = malloc(payload_size);
                if(!payload_buffer) {
                    next_state = STATE_ERROR;
                    fclose(file);
                    break;
                }
 
                fread(payload_buffer, payload_size, 1, file);
                fclose(file);
                next_state = STATE_INSTALL_PAYLOAD;
            }
            break;

            case STATE_INSTALL_PAYLOAD:
            {
                payload_buffer = BLZ_Code(payload_buffer, payload_size, (unsigned int*)&payload_size, BLZ_NORMAL);

                void* buffer = NULL;
                size_t size = 0;
                Result ret = read_savedata("/main", &buffer, &size, gametitle);
                if(ret)
                {
                    sprintf(status, "An error occured! Failed to embed payload\n    Error code: %08lX", ret);
                    next_state = STATE_ERROR;
                    break;
                }


                u32 out_size = 0;
                char path[256];
                memset(path, 0, sizeof(path));

                snprintf(path, sizeof(path) - 1, "romfs:/%s/overflow.bin", gameversion);
                write_from_file(buffer + 0x23F58, path, &out_size);

                memset(path, 0, sizeof(path));
                snprintf(path, sizeof(path) - 1, "romfs:/%s/%s/rop.bin", gameversion, gametitle);
                write_from_file(buffer + 0x67c00, path, &out_size);

                memset(path, 0, sizeof(path));
                snprintf(path, sizeof(path) - 1, "romfs:/%s/oras_code.bin", gameversion);
                write_from_file(buffer + 0x67c00 + out_size, path, &out_size);

                romfsExit();

                *(u32*)(buffer + 0x67c00 + 0x5000 - 0x4) = payload_size;
                memcpy(buffer + 0x67c00 + 0x5000, payload_buffer, payload_size);

                u16 ccitt = ccitt16(buffer + 0x23a00, 0x7ad0);
                *(u16*)(buffer + 0x75fca) = ccitt;

                ccitt = ccitt16(buffer + 0x67c00, 0xe058);
                *(u16*)(buffer + 0x75fe2) = ccitt;

                ret = write_savedata("/main", buffer, size, gametitle);

                free(buffer);

                if(ret)
                {
                    sprintf(status, "An error occured! Failed to install payload\n    Error code: %08lX", ret);
                    next_state = STATE_ERROR;
                    break;
                }

                next_state = STATE_INSTALLED_PAYLOAD;
            }
            break;

            case STATE_INSTALLED_PAYLOAD:
                next_state = STATE_NONE;
                break;

            default: break;
        }

        consoleSelect(&bottomConsole);
        printf("\x1b[0;0H  Current status:\n    %s\n", status);

        gspWaitForVBlank();
    }

    if(payload_buffer) free(payload_buffer);

    httpcExit();

    svcCloseHandle(save_session);
    fsExit();

    gfxExit();
    return 0;
}

