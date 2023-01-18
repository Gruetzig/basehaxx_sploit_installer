#include "ui.h"

void initContext(DrawContext* ctx)
{
    ctx->top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    ctx->bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
}

void initColors(DrawContext* ctx)
{
    ctx->clrBlack = C2D_Color32(0x00, 0x00, 0x00, 0xFF);
    ctx->clrWhite = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
    ctx->clrRed = C2D_Color32(0xFF, 0x00, 0x00, 0xFF);
    ctx->clrBlue = C2D_Color32(0x00, 0x00, 0xFF, 0xFF);
    ctx->clrGreen = C2D_Color32(0x00, 0xFF, 0x00, 0xFF);
    ctx->clrYellow = C2D_Color32(0xFF, 0xFF, 0x00, 0xFF);
    ctx->clrBgBright = C2D_Color32(0x57, 0x31, 0x70, 0xFF);
    ctx->clrBgDark = C2D_Color32(0x49, 0x29, 0x5E, 0xFF);
}

void drawText(float x, float y, float z, float scale, u32 color, const char* value) //ty mechanicaldragon0687
{
    C2D_TextBuf buf=C2D_TextBufNew(4096);
    C2D_Text text;
    C2D_TextParse(&text, buf, value);
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor | C2D_AlignCenter, x, y, z, scale, scale, color);
    C2D_TextBufDelete(buf);
}

void drawProgress(DrawContext* ctx, float x, float y, float z, float w, float h, u32 color, float progress) { //progress 0-100
    char str[331];
    sprintf(str, "%d%%", (int)progress);
    C2D_DrawRectSolid(x, y, z, w, h, ctx->clrWhite);
    C2D_DrawRectSolid(x+1, y+1, z, w-2, h-2, ctx->clrBlack);
    C2D_DrawRectSolid(x+1, y+1, z, (w*(progress/100))-2, h-2, color);
    drawText(x+(w/2), y+h/2-(30*(h/60))/2, z, h/60, ctx->clrWhite, str);
}
