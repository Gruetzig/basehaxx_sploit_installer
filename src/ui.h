#include <citro2d.h>
#include "globals.h"

#define SCREEN_WIDTH_TOP 400
#define SCREEN_WIDTH_BOTTOM 320
#define SCREEN_HEIGHT 240

typedef struct {
    C3D_RenderTarget* top;
    C3D_RenderTarget* bottom;
    u32 clrBlack;
    u32 clrWhite;
    u32 clrRed;
    u32 clrBlue;
    u32 clrGreen;
    u32 clrYellow;
    u32 clrBgBright;
    u32 clrBgDark;
} DrawContext;

void initContext(DrawContext* ctx);
void initColors(DrawContext* ctx);
void drawText(float x, float y, float z, float scale, u32 color, const char* value);
void drawProgress(DrawContext* ctx, float x, float y, float z, float w, float h, u32 color, float progress);
void drawCreditsButton(DrawContext* ctx);
void drawCredits(DrawContext* ctx); 