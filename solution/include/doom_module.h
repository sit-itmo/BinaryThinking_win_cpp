#pragma once
#include <windows.h>

int DoomMod_Setup(LPCSTR wadFile, BOOLEAN soundEnabled);
int DoomMod_StableDiffusionStart(LPCSTR modelFile, DWORD width, DWORD height, LPCSTR prompt, LPCSTR negative);
int DoomMod_StableDiffusionStop();
int DoomMod_Resize(DWORD width, DWORD height, HWND hwnd);

int DoomMod_Start();
int DoomMod_Stop();

HBITMAP DoomMod_BufferLock();
DWORD DoomMod_BufferGet_Width();
DWORD DoomMod_BufferGet_Height();
BYTE* DoomMod_BufferGet_Pixels();
void DoomMod_BufferUnlock();

void DoomMod_PutMouseEvent(UINT msg);
void DoomMod_PutKeyEvent(UINT msg, WPARAM wparam);



