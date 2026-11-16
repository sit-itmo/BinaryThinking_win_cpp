#include "doom_internal.h"

MouseState g_DOOM_MouseState;
PixelBuffer g_DOOM_ScreenPixels;
PixelBuffer g_DOOM_DoomPixels;
SoundBuffer g_DOOM_SoundBuffer;
BOOLEAN g_DOOM_Ready = 0;
BOOLEAN g_DOOM_SoundEnabled = 0;
BOOLEAN g_DOOM_StableDiffusionEnabled = 0;
BOOLEAN g_DOOM_SoundReady = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        printf("\n%3d: DLL ATTACH!", __LINE__);
        break;

    case DLL_THREAD_ATTACH:
        printf("\n%3d: DLL ATTACH!", __LINE__);
        break;

    case DLL_THREAD_DETACH:
        printf("\n%3d: DLL DETACH!", __LINE__);
        break;

    case DLL_PROCESS_DETACH:
        printf("\n%3d: DLL DETACH!", __LINE__);
        break;
    }
    return TRUE;
}

__declspec(dllexport) int DoomMod_Resize(DWORD width, DWORD height, HWND hwnd)
{
    PixelBuffer_UpdateSize(&g_DOOM_ScreenPixels, width, height, hwnd);
    return 0;
}


__declspec(dllexport) HBITMAP DoomMod_BufferLock()
{
    EnterCriticalSection(&g_DOOM_ScreenPixels.Lock);
    return g_DOOM_ScreenPixels.hBitmap;
}

__declspec(dllexport) DWORD DoomMod_BufferGet_Width()
{
    return g_DOOM_ScreenPixels.Size.cx;
}

__declspec(dllexport) DWORD DoomMod_BufferGet_Height()
{
    return g_DOOM_ScreenPixels.Size.cy;
}

__declspec(dllexport) BYTE* DoomMod_BufferGet_Pixels()
{
    return g_DOOM_ScreenPixels.pPixels;
}

__declspec(dllexport) void DoomMod_BufferUnlock()
{
    LeaveCriticalSection(&g_DOOM_ScreenPixels.Lock);
}

__declspec(dllexport) void DoomMod_PutMouseEvent(UINT msg)
{
    MouseState_SendButton(&g_DOOM_MouseState, msg);
}

__declspec(dllexport) void DoomMod_PutKeyEvent(UINT msg, WPARAM wparam)
{
    switch (msg)
    {
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        doom_key_down(DOOM_ConvertKey(wparam));
        return;

    case WM_SYSKEYUP:
    case WM_KEYUP:
        doom_key_up(DOOM_ConvertKey(wparam));
        return;
    }
}

int DoomMod_Iteration()
{
    if (g_DOOM_Ready == 0)
    {
        return 0;
    }

    doom_update();

    if (g_DOOM_SoundEnabled)
    {
        SoundBuffer_AudioPump(&g_DOOM_SoundBuffer);
    }

    const unsigned char* p_frame = doom_get_framebuffer(4);

    if (g_DOOM_DoomPixels.pPixels)
    {
        EnterCriticalSection(&g_DOOM_DoomPixels.Lock);
        for (INT y = 0; y < DOOM_SCREEN_H; y++)
        {
            for (INT x = 0; x < DOOM_SCREEN_W; x++)
            {
                const BYTE* p_color = p_frame + (DOOM_SCREEN_W * y + x) * 4;
                PixelBuffer_PutPixel(&g_DOOM_DoomPixels, x, y,
                    p_color[2], p_color[1], p_color[0]);
            }
        }
        LeaveCriticalSection(&g_DOOM_DoomPixels.Lock);
    }

    if (g_DOOM_ScreenPixels.pPixels)
    {
        EnterCriticalSection(&g_DOOM_ScreenPixels.Lock);
        INT doom_area_width = g_DOOM_ScreenPixels.Size.cx;
        INT doom_area_height = g_DOOM_ScreenPixels.Size.cy;
        INT doom_area_x = 0;
        INT doom_area_y = 0;

        if (g_DOOM_StableDiffusionEnabled)
        {
            doom_area_width = g_DOOM_ScreenPixels.Size.cx / 2;
            doom_area_height = g_DOOM_ScreenPixels.Size.cy;
            doom_area_x = 0;
            doom_area_y = 0;
        }

        for (INT y = 0; y < doom_area_height; y++)
        {
            for (INT x = 0; x < doom_area_width; x++)
            {
                INT src_x = x * DOOM_SCREEN_W / doom_area_width;
                INT src_y = y * DOOM_SCREEN_H / doom_area_height;
                const BYTE* p_color = p_frame + (DOOM_SCREEN_W * src_y + src_x) * 4;
                PixelBuffer_PutPixel(&g_DOOM_ScreenPixels, x + doom_area_x, y + doom_area_y,
                    p_color[2], p_color[1], p_color[0]);
            }
        }
        LeaveCriticalSection(&g_DOOM_ScreenPixels.Lock);
    }
    return 0;
}

HANDLE g_DOOM_Thread = NULL;
HANDLE g_SD_Thread = NULL;

DWORD WINAPI StableDiffusionThreadProc(LPVOID x)
{
    StableDiffusion_Loop();
    return 0;
}

DWORD WINAPI DoomThreadProc(LPVOID lpThreadParameter)
{
    while (g_DOOM_Ready)
    {
        DoomMod_Iteration();
    }
    return 0;
}

__declspec(dllexport) int DoomMod_StableDiffusionStop()
{
    g_DOOM_StableDiffusionEnabled = 0;
    if (g_SD_Thread == NULL)
    {
        return 0;
    }
    
    DWORD res = WaitForSingleObject(g_SD_Thread, 2000);
    
    if (res != WAIT_OBJECT_0)
    {
        TerminateThread(g_SD_Thread, 0);
        WaitForSingleObject(g_SD_Thread, INFINITE);
    }
    g_SD_Thread = NULL;
    StableDiffusion_Destroy();
    return 0;
}

__declspec(dllexport) int DoomMod_StableDiffusionStart(LPCSTR modelFile, DWORD width, DWORD height, LPCSTR prompt, LPCSTR negative)
{
    if (g_DOOM_StableDiffusionEnabled && g_SD_Thread != NULL)
    {
        DoomMod_StableDiffusionStop();
    }

    int res = StableDiffusion_Setup(modelFile, prompt, negative, width, height);
    if (res != 0)
    {
        return res;
    }
    g_DOOM_StableDiffusionEnabled = 1;
    g_SD_Thread = CreateThread(NULL, 0, StableDiffusionThreadProc, NULL, 0, NULL);
    return 0;
}


__declspec(dllexport) int DoomMod_Start()
{
    if (g_DOOM_Ready && g_DOOM_Thread == NULL)
    {
        g_DOOM_Thread = CreateThread(NULL, 0, DoomThreadProc, NULL, 0, NULL);
    }
    return g_DOOM_Ready;
}

__declspec(dllexport) int DoomMod_Stop()
{
    DoomMod_StableDiffusionStop();

    g_DOOM_Ready = 0;
    WaitForSingleObject(g_DOOM_Thread, INFINITE);


    PixelBuffer_Destroy(&g_DOOM_ScreenPixels);
    PixelBuffer_Destroy(&g_DOOM_DoomPixels);
    SoundBuffer_Destroy(&g_DOOM_SoundBuffer);
    return 0;
}

__declspec(dllexport) int DoomMod_Setup(LPCSTR wadFile, BOOLEAN soundEnabled)
{
    if (g_DOOM_Ready)
    {
        DoomMod_Stop();
    }

    ZeroMemory(&g_DOOM_ScreenPixels, sizeof(g_DOOM_ScreenPixels));
    ZeroMemory(&g_DOOM_DoomPixels, sizeof(g_DOOM_DoomPixels));

    InitializeCriticalSection(&g_DOOM_ScreenPixels.Lock);
    InitializeCriticalSection(&g_DOOM_DoomPixels.Lock);

    g_DOOM_SoundEnabled = soundEnabled;
    if (soundEnabled)
    {
        ZeroMemory(&g_DOOM_SoundBuffer, sizeof(g_DOOM_SoundBuffer));
    }

    ZeroMemory(&g_DOOM_MouseState, sizeof(g_DOOM_MouseState));
    g_DOOM_MouseState.FirstMove = 1;

    DOOM_Setup(wadFile);
    PixelBuffer_UpdateSize(&g_DOOM_DoomPixels, DOOM_SCREEN_W, DOOM_SCREEN_H, NULL);

    return 0;
}

