#pragma once

#include <windows.h>
#include <stdio.h>
#include <DOOM.h>

enum { DOOM_SOUND_BYTES = 2048 };
enum { DOOM_SOUND_SAMPLES = DOOM_SOUND_BYTES / sizeof(short) };
enum { DOOM_SAMPLE_RATE = 11025 };
enum { DOOM_SCREEN_W = 320 };
enum { DOOM_SCREEN_H = 200 };

typedef struct _PixelBuffer
{
    HBITMAP hBitmap;
    SIZE Size;
    BYTE* pPixels;
    UINT Pitch;
    CRITICAL_SECTION Lock;
} PixelBuffer;

typedef struct _SoundBuffer
{
    HWAVEOUT hWave;
    WAVEHDR  Header[2];
    short    pBuffer[2][DOOM_SOUND_SAMPLES];
}SoundBuffer;

typedef struct _MouseState
{
    BOOLEAN Captured;
    INT  X;
    INT  Y;
    BOOLEAN FirstMove;
}MouseState;

extern MouseState g_DOOM_MouseState;
extern PixelBuffer g_DOOM_ScreenPixels;
extern PixelBuffer g_DOOM_DoomPixels;
extern SoundBuffer g_DOOM_SoundBuffer;
extern BOOLEAN g_DOOM_Ready;
extern BOOLEAN g_DOOM_SoundEnabled;
extern BOOLEAN g_DOOM_StableDiffusionEnabled;
extern BOOLEAN g_DOOM_SoundReady;

int SoundBuffer_AudioPump(SoundBuffer* p_sound);
int SoundBuffer_Destroy(SoundBuffer* p_sound);

int PixelBuffer_PutPixel(PixelBuffer* p_backBuffer, int x, int y, UCHAR r, UCHAR g, UCHAR b);
int PixelBuffer_UpdateSize(PixelBuffer* p_backBuffer, INT w, INT h, HWND hwnd);
int PixelBuffer_Destroy(PixelBuffer* p_backBuffer);

int DOOM_ConvertKey(WPARAM key);
int DOOM_Setup(const char* p_file);

int MouseState_SendButton(MouseState* p_mouse, UINT msg);

void StableDiffusion_Destroy();
DWORD StableDiffusion_Loop();
int StableDiffusion_Setup(const char* p_model, const char* p_prompt, const char* p_negative, DWORD width, DWORD height);
