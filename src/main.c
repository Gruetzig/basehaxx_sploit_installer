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
#include "ui.h"
#include <citro2d.h>

Handle save_session;
FS_Archive save_archive;
char status[256];
DrawContext ctx;
int progress;


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
    cfguInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    initContext(&ctx);
    initColors(&ctx);

    state_t current_state = STATE_NONE;
    state_t next_state = STATE_INITIALIZE;

    char gametitle[3];
    strcpy(gametitle, "OR"); //init gametitle
    char gameversion[4];
    u16 gameversion_id = 0;

    static char top_text[2048];
    static char action_text[2048];
    static char note_text[2048];
    

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

            switch(next_state)
            {
                case STATE_INITIALIZE:
                    sprintf(top_text, "Initializing...please wait...");
                    break;
                case STATE_INITIAL:
                    sprintf(top_text, "Welcome to the Basehaxx installer");
                    break;
                case STATE_SELECT_GAME:
                    sprintf(top_text, "Select your game...");
                    break;
                case STATE_READ_PAYLOAD:
                    sprintf(top_text,"Reading payload...");
                    break;
                case STATE_INSTALL_PAYLOAD:
                    sprintf(top_text, "Installing payload...");
                    break;
                case STATE_INSTALLED_PAYLOAD:
                    sprintf(top_text, "Done! basehaxx was successfully installed.");
                    break;
                case STATE_ERROR:
                    sprintf(top_text, "Looks like something went wrong. :(");
                    break;
                default:
                    break;
            }

            current_state = next_state;
        }

        switch(current_state) //action code
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
                sprintf(action_text, "Press A to begin!");
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
                        sprintf(note_text, "Installing for  v%s", gameversion);
                        break;
                    }
                }
                sprintf(action_text, "Selected game: %s", gametitle);

            }
            break;
            
            case STATE_READ_PAYLOAD:
            {
                progress = 0;
                FILE* file = fopen("sdmc:/basehaxx_payload.bin", "r");
                printf("Using payload from SD\n");
                if (file == NULL) {
                    sprintf(status, "\nFailed to open otherapp payload\n");
                    next_state = STATE_ERROR;
                    break;
                }
                progress = 20;
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
                progress = 40;
                payload_buffer = BLZ_Code(payload_buffer, payload_size, (unsigned int*)&payload_size, BLZ_NORMAL);
                progress = 60;
                void* buffer = NULL;
                size_t size = 0;
                Result ret = read_savedata("/main", &buffer, &size, gametitle);
                if(ret)
                {
                    //sprintf(status, "An error occured! Failed to embed payload\n    Error code: %08lX", ret);
                    next_state = STATE_ERROR;
                    break;
                }
                progress = 70;

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
                progress = 100;
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
            {
                next_state = STATE_NONE;
                break;
            }
            default: break;
        }
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(ctx.top, ctx.clrBgDark);
        C2D_TargetClear(ctx.bottom, ctx.clrBgDark);
        C2D_SceneBegin(ctx.top);
        drawText(SCREEN_WIDTH_TOP/2, 0, 0, 0.75f, ctx.clrWhite, top_text);
        drawText(SCREEN_WIDTH_TOP/2, 30*0.75f, 0, 0.6f, ctx.clrWhite, action_text);
        drawText(SCREEN_WIDTH_TOP/2, SCREEN_HEIGHT-30*0.75f, 0, 0.6f, ctx.clrWhite, note_text);
        if ((current_state == STATE_READ_PAYLOAD) || (current_state == STATE_INSTALL_PAYLOAD)) {
            drawProgress(&ctx, SCREEN_WIDTH_TOP/2-100, SCREEN_HEIGHT/2-25, 0, 200, 50, ctx.clrBlue, progress);
        }
        C2D_SceneBegin(ctx.bottom);
        drawText(SCREEN_WIDTH_BOTTOM/2, 0, 0, 0.5f, ctx.clrWhite, status);
        C3D_FrameEnd(0);
        

    }

    if(payload_buffer) free(payload_buffer);

    svcCloseHandle(save_session);
    C2D_Fini();
    C3D_Fini();
    fsExit();
    cfguExit();
    gfxExit();
    return 0;
}

