#include "doom_internal.h"

int MouseState_SendButton(MouseState* p_mouse, UINT msg)
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

int DOOM_ConvertKey(WPARAM key) {
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

int DOOM_Setup(const char* p_file)
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
    doom_init(3, (char**)g_DOOM_argv, 0);

    g_DOOM_Ready = 1;
    return 0;
}
