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
#include "utils.h"

Handle save_session;
FS_Archive save_archive;
char status[256];

typedef enum
{
    STATE_NONE,
    STATE_INITIALIZE,
    STATE_INITIAL,
    STATE_SELECT_GAME,
    STATE_READ_PAYLOAD,
    STATE_INSTALL_PAYLOAD,
    STATE_INSTALLED_PAYLOAD,
    STATE_ERROR,
} state_t;

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
    strcpy(gametitle, "OR"); //init gametitle
    char gameversion[4];
    u16 gameversion_id = 0;

    static char top_text[2048];

    char top_text_tmp[256];

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
                case STATE_SELECT_GAME:
                    strcat(top_text, "Select your game...\n");
                    break;
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

            current_state = next_state;
        }

        consoleSelect(&topConsole);
        printf("\x1b[0;%dHBasehaxx installer based hax edition ;)\n\n", (50 - 46) / 2);
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

                if(getProgramID(&program_id))
                {
                    next_state = STATE_ERROR;
                    break;
                }

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
                if ((hidKeysDown() & KEY_LEFT) && (!strcmp(gametitle, "OR"))) strcpy(gametitle, "AS");
                if ((hidKeysDown() & KEY_RIGHT) && (!strcmp(gametitle, "AS"))) strcpy(gametitle, "OR");
                if (hidKeysDown() & KEY_A) {
                    if (!strcmp(gametitle, "OR")) {
                        program_id = 0x000400000011C400;
                    }
                    else if (!strcmp(gametitle, "AS")) {
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
                        printf("\n\nDetected version: %s", gameversion);
                        break;
                    }
                }
                printf("Selected game: %s", gametitle);

            }
            break;
            
            case STATE_READ_PAYLOAD:
            {
                FILE* file = fopen("sdmc:/basehaxx_payload.bin", "r");
                printf("Using payload from SD\n");
                if (file == NULL) {
                    sprintf(status, "\nFailed to open otherapp payload\n");
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
                    //sprintf(status, "An error occured! Failed to embed payload\n    Error code: %08lX", ret);
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

