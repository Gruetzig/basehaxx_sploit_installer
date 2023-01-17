#include "utils.h"

#define VERSION_14 0x1C70

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
        ret = AM_GetTitleInfo(1, 1, &update_program_id, &update_title);
        amExit();

        memcpy(gameversion, "1.0", 3);
        if(R_SUCCEEDED(ret))
        {
            if(update_title.version == VERSION_14)
            {
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