#include "doom_internal.h"

int PixelBuffer_PutPixel(PixelBuffer* p_backBuffer, int x, int y, UCHAR r, UCHAR g, UCHAR b)
{
    if (p_backBuffer == NULL)
    {
        return -1;
    }
    if (x < 0 || y < 0 || x >= p_backBuffer->Size.cx || y >= p_backBuffer->Size.cy)
    {
        return 0;
    }

    BYTE* p = p_backBuffer->pPixels + y * p_backBuffer->Pitch + x * 4;

    p[0] = b;
    p[1] = g;
    p[2] = r;
    p[3] = 0xFF;
    return 0;
}

int PixelBuffer_UpdateSize(PixelBuffer* p_backBuffer, INT w, INT h, HWND hwnd)
{
    if (p_backBuffer == NULL)
    {
        return -1;
    }

    EnterCriticalSection(&p_backBuffer->Lock);

    if (p_backBuffer->hBitmap)
    {
        DeleteObject(p_backBuffer->hBitmap);
        p_backBuffer->hBitmap = NULL;
    }
    p_backBuffer->pPixels = NULL;

    BITMAPINFO bi;
    ZeroMemory(&bi, sizeof(bi));

    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* bits = NULL;
    HDC hdc = GetDC(hwnd);
    p_backBuffer->hBitmap = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    ReleaseDC(hwnd, hdc);

    p_backBuffer->pPixels = (BYTE*)bits;
    p_backBuffer->Size.cx = w;
    p_backBuffer->Size.cy = h;
    p_backBuffer->Pitch = w * 4;
    LeaveCriticalSection(&p_backBuffer->Lock);

    return 0;
}

int PixelBuffer_Destroy(PixelBuffer* p_backBuffer)
{
    if (p_backBuffer == NULL)
    {
        return -1;
    }
    EnterCriticalSection(&p_backBuffer->Lock);
    if (p_backBuffer->hBitmap)
    {
        DeleteObject(p_backBuffer->hBitmap);
    }
    LeaveCriticalSection(&p_backBuffer->Lock);
    ZeroMemory(&p_backBuffer, sizeof(p_backBuffer));
    return 0;
}