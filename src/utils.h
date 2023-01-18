#include "globals.h"
#include <stdio.h>
#include <string.h>

Result initFs();
Result getProgramID(u64* id);
Result getGameVersion(u64 program_id, char* gameversion, u16* gameversion_id);
u64 getCartID();
int creditMenu(touchPosition touch);