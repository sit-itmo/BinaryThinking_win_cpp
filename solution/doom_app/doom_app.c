#include <stdio.h>
#include <windows.h>
#include <DOOM.h>

#define DOOM_WINDOW_CLASS_NAME TEXT("{8480BDA8-21B7-45B6-8AB6-431F167241AB}")

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

MouseState g_DOOM_MouseState;
PixelBuffer g_DOOM_ScreenPixels;
PixelBuffer g_DOOM_DoomPixels;
PixelBuffer g_DOOM_SdPixels;
SoundBuffer g_DOOM_SoundBuffer;
BOOLEAN g_DOOM_Ready = 0;
BOOLEAN g_DOOM_SoundReady = 0;

int MouseState_SendButton(MouseState *p_mouse, UINT msg)
{
    if (p_mouse == NULL)
    {
        return -1;
    }
  
    switch (msg) 
    {
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: doom_button_down(DOOM_LEFT_BUTTON);   break;
    case WM_LBUTTONUP:                           doom_button_up(DOOM_LEFT_BUTTON);   break;
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK: doom_button_down(DOOM_RIGHT_BUTTON);  break;
    case WM_RBUTTONUP:                           doom_button_up(DOOM_RIGHT_BUTTON);  break;
    case WM_MBUTTONDOWN:                         doom_button_down(DOOM_MIDDLE_BUTTON); break;
    case WM_MBUTTONUP:                           doom_button_up(DOOM_MIDDLE_BUTTON); break;
    }
    return 0;
}

int MouseState_MouseHandle(MouseState* p_mouse, LPARAM lParam, HWND hwnd)
{
    if (p_mouse == NULL)
    {
        return -1;
    }

    //if (!p_mouse->Captured)
    //{
    //    return 0;
    //}

    int x = (short)LOWORD(lParam);
    int y = (short)HIWORD(lParam);

    if (p_mouse->FirstMove)
    {
        p_mouse->X = x;
        p_mouse->Y = y;
        p_mouse->FirstMove = 0;
        return 0;
    }

    int dx = x - p_mouse->X;
    int dy = y - p_mouse->Y;
    p_mouse->X = x;
    p_mouse->Y = y;

    if (dx || dy)
    {
        doom_mouse_move(dx, dy);
    }

    RECT rc; GetClientRect(hwnd, &rc);
    POINT center = { (rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2 };
    ClientToScreen(hwnd, &center);

    p_mouse->X = (rc.right - rc.left) / 2;
    p_mouse->Y = (rc.bottom - rc.top) / 2;
    return 0;
}

int SoundBuffer_FillAndSubmit(SoundBuffer *p_sound, int i)
{
    if (p_sound == NULL || g_DOOM_SoundReady == 0)
    {
        return -1;
    }
    
    i %= 2;

    memcpy(p_sound->pBuffer[i], doom_get_sound_buffer(), DOOM_SOUND_BYTES);

    WAVEHDR* h = &p_sound->Header[i];
    memset(h, 0, sizeof(WAVEHDR));
    h->lpData = (LPSTR)p_sound->pBuffer[i];
    h->dwBufferLength = DOOM_SOUND_BYTES;

    waveOutPrepareHeader(p_sound->hWave, h, sizeof(WAVEHDR));
    waveOutWrite(p_sound->hWave, h, sizeof(WAVEHDR));
    return 0;
}

int SoundBuffer_AudioStart(SoundBuffer* p_sound)
{
    if (p_sound == NULL || g_DOOM_Ready == 0)
    {
        return -1;
    }
    if (g_DOOM_SoundReady)
    {
        return -2;
    }

    WAVEFORMATEX wf;
    ZeroMemory(&wf, sizeof(wf));
    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = 1;                      // моно
    wf.nSamplesPerSec = DOOM_SAMPLE_RATE;
    wf.wBitsPerSample = 16;
    wf.nBlockAlign = (WORD)(wf.nChannels * wf.wBitsPerSample / 8); // 2
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;           // 22050

    if (waveOutOpen(&p_sound->hWave, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
    {
        return 0;
    }
    g_DOOM_SoundReady = 1;
    SoundBuffer_FillAndSubmit(p_sound, 0);
    SoundBuffer_FillAndSubmit(p_sound, 1);
    return 0;
}

int SoundBuffer_AudioPump(SoundBuffer* p_sound)
{
    SoundBuffer_AudioStart(p_sound);

    if (p_sound == NULL || g_DOOM_SoundReady == 0 )
    {
        return -1;
    }

    for (int i = 0; i < 2; ++i) 
    {
        if (p_sound->Header[i].dwFlags & WHDR_DONE)
        { 
            waveOutUnprepareHeader(p_sound->hWave, &p_sound->Header[i], sizeof(WAVEHDR));
            SoundBuffer_FillAndSubmit(p_sound, i);
        }
    }
    return 0;
}

int SoundBuffer_Destroy(SoundBuffer* p_sound)
{
    if (p_sound == NULL)
    {
        return -1;
    }

    if (p_sound->hWave)
    {
        waveOutReset(p_sound->hWave);
        waveOutClose(p_sound->hWave);
        p_sound->hWave = NULL;
    }
    return 0;
}

inline int PixelBuffer_PutPixel(PixelBuffer* p_backBuffer, int x, int y, UCHAR r, UCHAR g, UCHAR b)
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

int PixelBuffer_UpdateSize(PixelBuffer *p_backBuffer, INT w, INT h, HWND hwnd)
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

static void ParseDoomMode(const char* mode, DWORD* access, DWORD* disposition, DWORD* share) 
{
    *access = 0;
    *share = FILE_SHARE_READ | FILE_SHARE_WRITE; // Doom typically doesn't require strict locking
    *disposition = OPEN_EXISTING;

    if (strchr(mode, 'r') || strchr(mode, 'R')) {
        *access |= GENERIC_READ;
    }
    if (strchr(mode, 'w') || strchr(mode, 'W')) {
        *access |= GENERIC_WRITE;
        *disposition = CREATE_ALWAYS; // Matches fopen "w"
    }
    if (strchr(mode, 'a') || strchr(mode, 'A')) {
        *access |= GENERIC_WRITE;
        *disposition = OPEN_ALWAYS; // Matches fopen "a", but need to move pointer to end later
    }
    if (strchr(mode, '+')) {
        *access |= GENERIC_READ | GENERIC_WRITE;
    }
}


void* DOOM_Open(const char* filename, const char* mode) {
    DWORD access, disposition, share;
    ParseDoomMode(mode, &access, &disposition, &share);

    HANDLE hFile = CreateFileA(
        filename,
        access,
        share,
        NULL,
        disposition,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    if (strchr(mode, 'a') || strchr(mode, 'A'))
    {
        SetFilePointer(hFile, 0, NULL, FILE_END);
    }

    return (void*)hFile;
}

void DOOM_Close(void* handle) {
    if (handle) {
        CloseHandle((HANDLE)handle);
    }
}

int DOOM_Read(void* handle, void* buf, int count) {
    if (!handle || count <= 0) return 0;
    DWORD bytesRead = 0;
    BOOL result = ReadFile((HANDLE)handle, buf, (DWORD)count, &bytesRead, NULL);

    // Win32 synchronous read: returns TRUE and bytesRead=0 when EOF is reached
    if (result) {
        return (int)bytesRead;
    }
    return -1; // Read error
}

int DOOM_Write(void* handle, const void* buf, int count) {
    if (!handle || count <= 0) return 0;
    DWORD bytesWritten = 0;
    BOOL result = WriteFile((HANDLE)handle, buf, (DWORD)count, &bytesWritten, NULL);

    if (result) {
        return (int)bytesWritten;
    }
    return -1; // Write error
}

int DOOM_Seek(void* handle, int offset, doom_seek_t origin) {
    if (!handle) return -1;

    DWORD moveMethod;
    switch (origin) {
    case DOOM_SEEK_SET: moveMethod = FILE_BEGIN; break;
    case DOOM_SEEK_CUR: moveMethod = FILE_CURRENT; break;
    case DOOM_SEEK_END: moveMethod = FILE_END; break;
    default: return -1;
    }

    DWORD result = SetFilePointer((HANDLE)handle, offset, NULL, moveMethod);
    if (result == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
        return -1;
    }
    return 0; // Doom typically only checks for non-zero failure
}

int DOOM_Tell(void* handle) {
    if (!handle) return -1;
    DWORD pos = SetFilePointer((HANDLE)handle, 0, NULL, FILE_CURRENT);
    if (pos == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
        return -1;
    }
    return (int)pos;
}

int DOOM_Eof(void* handle) {
    if (!handle) return 1;

    // Win32 doesn't have a direct "peek" EOF flag; use current position vs file size to determine
    DWORD curPos = SetFilePointer((HANDLE)handle, 0, NULL, FILE_CURRENT);
    if (curPos == INVALID_SET_FILE_POINTER) return 1;

    DWORD fileSize = GetFileSize((HANDLE)handle, NULL);
    if (fileSize == INVALID_FILE_SIZE) return 1;

    return (curPos >= fileSize) ? 1 : 0;
}

void DOOM_GetTime(int* sec, int* usec) {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    // Convert FILETIME (100-nanosecond intervals since 1601-01-01) to Unix timestamp
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    // Difference between 1601-01-01 and 1970-01-01 in 100-nanosecond intervals
    const ULONGLONG EPOCH_DIFF = 116444736000000000ULL;

    ULONGLONG unixTime100ns = uli.QuadPart - EPOCH_DIFF;

    if (sec) {
        *sec = (int)(unixTime100ns / 10000000ULL);
    }
    if (usec) {
        *usec = (int)((unixTime100ns % 10000000ULL) / 10ULL);
    }
}

void DOOM_Print(const char* p_string)
{
    size_t string_len = strlen(p_string);
    for (size_t i = 0; i < string_len; i++)
    {
        if (p_string[i] == '\n')
        {
            printf("\n[DOOM]: ");
        }
        else
        {
            printf("%c", p_string[i]);
        }
    }

}

void* DOOM_Alloc(int size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

void DOOM_Free(void* Mem)
{
    HeapFree(GetProcessHeap(), 0, Mem);
    return;
}
void DOOM_Exit(int code)
{
    printf("[DOOM]: Exiting with code %d\n", code);
    ExitProcess(1);
}

static int DOOM_ConvertKey(WPARAM key) {
    switch (key) {
    case VK_RETURN:  return DOOM_KEY_ENTER;
    case 'A':
    case VK_LEFT:    return DOOM_KEY_LEFT_ARROW;
    case 'D':
    case VK_RIGHT:   return DOOM_KEY_RIGHT_ARROW;
    case 'W':
    case VK_UP:      return DOOM_KEY_UP_ARROW;
    case 'S':
    case VK_DOWN:    return DOOM_KEY_DOWN_ARROW;
    case VK_SPACE:   return DOOM_KEY_SPACE;
    case VK_CONTROL: return DOOM_KEY_CTRL;
    case VK_ESCAPE:  return DOOM_KEY_ESCAPE;
    case 'Y':        return DOOM_KEY_Y;
    default:         return 0;
    }
}

void DOOM_Iteration()
{
    if (g_DOOM_Ready == 0)
    {
        return;
    }
    
    doom_update();
    
    SoundBuffer_AudioPump(&g_DOOM_SoundBuffer);

    const unsigned char* p_frame = doom_get_framebuffer(4);
    
   
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


    EnterCriticalSection(&g_DOOM_ScreenPixels.Lock);
    INT doom_area_width = g_DOOM_ScreenPixels.Size.cx / 2;
    INT doom_area_height = g_DOOM_ScreenPixels.Size.cy;
    INT doom_area_x = 0;
    INT doom_area_y = 0;

    for (INT y = 0; y < doom_area_height; y++)
    {
        for (INT x = 0; x < doom_area_width; x++)
        {
            INT src_x = x * DOOM_SCREEN_W / doom_area_width + doom_area_x;
            INT src_y = y * DOOM_SCREEN_H / doom_area_height + doom_area_y;
            const BYTE* p_color = p_frame + (DOOM_SCREEN_W * src_y + src_x) * 4;
            PixelBuffer_PutPixel(&g_DOOM_ScreenPixels, x, y,
                p_color[2], p_color[1], p_color[0]);
        }
    }
    LeaveCriticalSection(&g_DOOM_ScreenPixels.Lock);
}

LRESULT CALLBACK DoomWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,  LPARAM lParam)
{

    switch (uMsg)
    {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        MouseState_SendButton(&g_DOOM_MouseState, uMsg);
        return 0;
    
    case WM_MOUSEMOVE:
        //MouseState_MouseHandle(&g_DOOM_MouseState, lParam, hwnd);

        return 0;
    
    case WM_CREATE:
        SetTimer(hwnd, 1, 20, NULL);
        // Initialize the window. 
        return 0;

    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            PixelBuffer_UpdateSize(&g_DOOM_ScreenPixels, LOWORD(lParam), HIWORD(lParam), hwnd);
        }
        return 0;

    case WM_TIMER:
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    case WM_PAINT: 
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
           

        EnterCriticalSection(&g_DOOM_ScreenPixels.Lock);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP old = (HBITMAP)SelectObject(memDC, g_DOOM_ScreenPixels.hBitmap);
        BitBlt(hdc, 0, 0, g_DOOM_ScreenPixels.Size.cx, g_DOOM_ScreenPixels.Size.cy, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, old);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        LeaveCriticalSection(&g_DOOM_ScreenPixels.Lock);

        return 0;
    }
    
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        doom_key_down(DOOM_ConvertKey(wParam));
        return 0;
    
    case WM_SYSKEYUP:
    case WM_KEYUP:
        doom_key_up(DOOM_ConvertKey(wParam));
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


const char* g_DOOM_argv[] =
{
    "doom.exe",
    "-file", 
    "doom.wad"
};

char g_DOOM_Home[128] = { 0 };

char* DOOM_GetEnv(const char* Name)
{
    if (!strcmp(Name, "HOME"))
    {
        return g_DOOM_Home;
    }
    return NULL;
}

int DOOM_Setup(const char *p_file)
{
    doom_set_resolution(DOOM_SCREEN_W, DOOM_SCREEN_H);
    doom_set_file_io(DOOM_Open, DOOM_Close, DOOM_Read, DOOM_Write,
        DOOM_Seek, DOOM_Tell, DOOM_Eof);
    doom_set_malloc(DOOM_Alloc, DOOM_Free);
    doom_set_exit(DOOM_Exit);
    doom_set_getenv(DOOM_GetEnv);
    doom_set_gettime(DOOM_GetTime);
    doom_set_print(DOOM_Print);

    if (p_file)
    {
        g_DOOM_argv[2] = p_file;
    }
    doom_init(3, (char **)g_DOOM_argv, 0);

    g_DOOM_Ready = 1;
    return 0;
}

DWORD WINAPI DoomThreadProc(LPVOID lpThreadParameter)
{
    while (g_DOOM_Ready)
    {
        DOOM_Iteration();
    }
    return 0;
}

HANDLE g_DOOM_Thread = NULL;
HANDLE g_SD_Thread = NULL;

#include "stable-diffusion.h"

#define SD_MODEL_PATH   "C:\\models\\v1-5-pruned-emaonly.safetensors"
#define SD_PROMPT       "oil painting style, thick brush strokes, artistic, vibrant colors"
#define SD_WIDTH        512
#define SD_HEIGHT       512
sd_ctx_t* g_ctx = NULL;

DWORD WINAPI StableDiffusionThreadProc(LPVOID x)
{
    sd_image_t init_img = { 0 };
    init_img.width = SD_WIDTH;
    init_img.height = SD_HEIGHT;
    init_img.channel = 3;
    init_img.data = (uint8_t*)malloc(SD_WIDTH * SD_HEIGHT * 3);

    while (g_DOOM_Ready)
    {
        if (g_DOOM_DoomPixels.pPixels == NULL)
        {
            continue;
        }
        EnterCriticalSection(&g_DOOM_DoomPixels.Lock);
        uint8_t* p_frame = (uint8_t*)g_DOOM_DoomPixels.pPixels;
        uint8_t* dst = init_img.data;
        for (int y = 0; y < SD_HEIGHT; y++)
        {
            for (int x = 0; x < SD_WIDTH; x++)
            {
                INT src_x = x * g_DOOM_DoomPixels.Size.cx / SD_WIDTH;
                INT src_y = y * g_DOOM_DoomPixels.Size.cy / SD_HEIGHT;
                const BYTE* p_src_color = g_DOOM_DoomPixels.pPixels 
                    + (g_DOOM_DoomPixels.Size.cx * src_y + src_x) * 4;
                BYTE* p_dst_color = dst
                    + (SD_WIDTH * y + x) * 3;
                p_dst_color[0] = p_src_color[2]; // R
                p_dst_color[1] = p_src_color[1]; // G
                p_dst_color[2] = p_src_color[0]; // B
            }
        }
        LeaveCriticalSection(&g_DOOM_DoomPixels.Lock);

        sd_img_gen_params_t params;
        sd_img_gen_params_init(&params);
        params.prompt = SD_PROMPT;
        params.negative_prompt = "";
        params.init_image = init_img;
        params.strength = 0.65f;
        params.width = SD_WIDTH;
        params.height = SD_HEIGHT;
        params.sample_params.sample_steps = 8;
        params.sample_params.sample_method = EULER_A_SAMPLE_METHOD;
        params.seed = -1;

        sd_image_generation_result_t* result = NULL;
        if (generate_image(g_ctx, &params, &result) && result && result->items && result->items[0].image.data) {
            uint8_t* out = result->items[0].image.data;
            
            EnterCriticalSection(&g_DOOM_ScreenPixels.Lock);
            INT sd_area_width = g_DOOM_ScreenPixels.Size.cx / 2;
            INT sd_area_height = g_DOOM_ScreenPixels.Size.cy;
            INT sd_area_x = g_DOOM_ScreenPixels.Size.cx / 2;
            INT sd_area_y = 0;

            for (INT y = 0; y < sd_area_height; y++)
            {
                for (INT x = 0; x < sd_area_width; x++)
                {
                    INT src_x = x * SD_WIDTH / sd_area_width;
                    INT src_y = y * SD_HEIGHT / sd_area_height;
                    const BYTE* p_color = out + (SD_WIDTH * src_y + src_x) * 3;
                    PixelBuffer_PutPixel(&g_DOOM_ScreenPixels, x + sd_area_x, y + sd_area_y,
                        p_color[2], p_color[1], p_color[0]);
                }
            }
            LeaveCriticalSection(&g_DOOM_ScreenPixels.Lock);


            free_sd_image_generation_result(result);
        }
    }

    free(init_img.data);
    return 0;
}

int StableDiffusion_Setup()
{
    SetEnvironmentVariableA("GGML_VK_DISABLE_COOPMAT", "1");
    SetEnvironmentVariableA("GGML_VK_DISABLE_INTEGER_DOT", "1");

    sd_ctx_params_t ctx_params;
    sd_ctx_params_init(&ctx_params);
    ctx_params.model_path = SD_MODEL_PATH;
    ctx_params.n_threads = 8;

    ctx_params.backend = "cpu";
    ctx_params.params_backend = "cpu";

    g_ctx = new_sd_ctx(&ctx_params);
    if (!g_ctx) {
        MessageBoxW(NULL, L"Не удалось загрузить модель", L"Ошибка", MB_OK);
        return 1;
    }
}

int main(int argc, char *argv[])
{
	printf("\nDOOM app version v0.1\n");
    ZeroMemory(&g_DOOM_ScreenPixels, sizeof(g_DOOM_ScreenPixels));
    ZeroMemory(&g_DOOM_DoomPixels, sizeof(g_DOOM_DoomPixels));
    ZeroMemory(&g_DOOM_SdPixels, sizeof(g_DOOM_SdPixels));
    
    InitializeCriticalSection(&g_DOOM_ScreenPixels.Lock);
    InitializeCriticalSection(&g_DOOM_DoomPixels.Lock);
    InitializeCriticalSection(&g_DOOM_SdPixels.Lock);

    ZeroMemory(&g_DOOM_SoundBuffer, sizeof(g_DOOM_SoundBuffer));
    ZeroMemory(&g_DOOM_MouseState, sizeof(g_DOOM_MouseState));
    g_DOOM_MouseState.FirstMove = 1;


	///////////////////////////////////
	// Phase I.  Create Window
	
	// RegisterClass
	WNDCLASSEX wcx;
    wcx.cbSize = sizeof(wcx);
    wcx.style = CS_HREDRAW | CS_VREDRAW; // redraw if size changes
    wcx.lpfnWndProc = DoomWndProc;       // points to window procedure 
    wcx.cbClsExtra = 0;                // no extra class memory 
    wcx.cbWndExtra = 0;                // no extra window memory 
    wcx.hInstance = GetModuleHandle(NULL);         // handle to instance 
    wcx.hIcon = LoadIcon(NULL, IDI_WARNING);   // predefined app. icon 
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);                    // predefined arrow 
    wcx.hbrBackground = GetStockObject(BLACK_BRUSH);
    wcx.lpszMenuName = NULL;    // name of menu resource 
    wcx.lpszClassName = DOOM_WINDOW_CLASS_NAME;  // name of window class 
    wcx.hIconSm = NULL;

    // Register the window class. 
    ATOM val = RegisterClassEx(&wcx);
	
    // CreateWindow
    HWND hwnd = CreateWindowEx(
        0,
        DOOM_WINDOW_CLASS_NAME,
        L"DOOM v0.1",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, GetModuleHandle(NULL), NULL);

	// ShowWindow
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    if (g_DOOM_DoomPixels.pPixels == NULL)
    {
        PixelBuffer_UpdateSize(&g_DOOM_DoomPixels, DOOM_SCREEN_W, DOOM_SCREEN_H, hwnd);
    }
    DOOM_Setup(argc > 1 ? argv[1] : NULL);
    g_DOOM_Thread = CreateThread(NULL, 0, DoomThreadProc, NULL, 0, NULL);

    StableDiffusion_Setup();
    g_SD_Thread = CreateThread(NULL, 0, StableDiffusionThreadProc, NULL, 0, NULL);


	// Phase II. While read messages and process them
	// while:
	//    ReadMessage
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
	    //  ProcessMesage
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    g_DOOM_Ready = 0;
    WaitForSingleObject(g_DOOM_Thread, INFINITE);
    TerminateThread(g_SD_Thread, 0);
    WaitForSingleObject(g_SD_Thread, INFINITE);
    if (g_ctx) free_sd_ctx(g_ctx);

    PixelBuffer_Destroy(&g_DOOM_ScreenPixels);
    PixelBuffer_Destroy(&g_DOOM_DoomPixels);
    PixelBuffer_Destroy(&g_DOOM_SdPixels);
    SoundBuffer_Destroy(&g_DOOM_SoundBuffer);

	return 0;
}