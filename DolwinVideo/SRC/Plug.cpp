// plugin interface
#include "GX.h"

uint8_t* RAM;
HINSTANCE   hPlugin;
HWND hwndMain;

static bool gxOpened = false;

// critical error
void GFXError(const char *fmt, ...)
{
    va_list arg;
    char buf[0x1000];

    va_start(arg, fmt);
    vsprintf(buf, fmt, arg);
    va_end(arg);

    MessageBoxA(
        NULL, 
        buf, 
        "GFX plugin error", 
        MB_ICONEXCLAMATION | MB_OK | MB_TOPMOST
    );
}

// ---------------------------------------------------------------------------

long GXOpen(uint8_t * ramPtr, HWND _hwndMain)
{
    if (gxOpened)
        return 1;

    BOOL res;

    hPlugin = GetModuleHandle(NULL);
    hwndMain = _hwndMain;

    RAM = ramPtr;
    gfx = &ogl;         // hack : select opengl as default
    res = gfx->OpenSubsystem(hwndMain);

    // vertex programs extension
    //SetupVertexShaders();
    //ReloadVertexShaders();

    // reset pipeline
    FifoReconfigure(VTX_MAX_ATTR, 0, 0, 0, 0, 0);
    accptr = accum;
    acclen = 0;
    cmdidle=1;
    frame_done=1;

    // flush texture cache
    TexInit();

    // prepare on-screen font texture
    PerfInit();

    gxOpened = true;

    return res;
}

void GXClose()
{
    if (!gxOpened)
        return;

    gfx->CloseSubsystem();

    TexFree();

    PerfClose();

    gxOpened = false;
}
