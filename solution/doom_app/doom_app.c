#include <stdio.h>
#include <windows.h>
#include <doom_module.h>

#define DOOM_WINDOW_CLASS_NAME TEXT("{8480BDA8-21B7-45B6-8AB6-431F167241AB}")


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
        DoomMod_PutMouseEvent(uMsg);
        return 0;

    case WM_MOUSEMOVE:
        return 0;

    case WM_CREATE:
        SetTimer(hwnd, 1, 20, NULL);
        return 0;

    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            DoomMod_Resize(LOWORD(lParam), HIWORD(lParam), hwnd);
        }
        return 0;

    case WM_TIMER:
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);


        HBITMAP h_buf = DoomMod_BufferLock();
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP old = (HBITMAP)SelectObject(memDC, h_buf);
        BitBlt(hdc, 0, 0, DoomMod_BufferGet_Width(), DoomMod_BufferGet_Height(), memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, old);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        DoomMod_BufferUnlock();

        return 0;
    }

    case WM_SYSKEYUP:
    case WM_KEYUP:
        DoomMod_PutKeyEvent(uMsg, wParam);
        return 0;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        if (wParam == 'X')
        {
            DoomMod_StableDiffusionStart(
                "C:\\models\\v1-5-pruned-emaonly.safetensors",
                512, 512,
                "Epic retro-futuristic demon-slaying action, 1990s heavy metal album cover aesthetic, a nightmarish sci-fi fortress in hell, glowing lava rivers, neon red and electric blue lighting, gigantic demonic architecture, detailed rusty metal walls, dramatic volumetric lighting, sparks and explosions, thick atmospheric smoke, glowing monster eyes, supernatural fire, vibrant saturated colors, cinematic shadows, dark fantasy meets cyberpunk, ultra detailed textures, gritty industrial atmosphere, breathtaking environment, dynamic action, sharp focus, masterpiece, highly detailed, 8k, unreal engine cinematic rendering",
                "blurry, low quality, washed out colors, flat lighting, distorted geometry, deformed monsters, extra limbs, duplicate objects, cartoonish, anime, plastic textures, oversaturated highlights, text, watermark, artifacts, excessive noise");
        }
        if (wParam == 'C')
        {
            DoomMod_StableDiffusionStop();
        }
        
        DoomMod_PutKeyEvent(uMsg, wParam);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


int main(int argc, char *argv[])
{
	printf("\nDOOM app version v0.1\n");
    
    ////////////////////////////////////////////
    // Test DLL
    HMODULE hmod = LoadLibrary(L"doom_module.dll");
    void* p_func1 = GetProcAddress(hmod, "DoomExp_Add");
    
	///////////////////////////////////
	// Phase I.  Create Window
    DoomMod_Setup(argc > 1 ? argv[1] : NULL, TRUE);

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

    DoomMod_Resize(800, 600, hwnd);
    
    DoomMod_Start();


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

    DoomMod_Stop();

	return 0;
}