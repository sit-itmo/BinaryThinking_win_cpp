#include <stdio.h>
#include <windows.h>

LONG64 g_Counter = 0;

void DoImportantWork(int arg1, int arg2)
{
    int i = 0; 

    for (i = 0; i < 10000000; i++)
    {
        printf("\n[%4x]Work step %d!", arg1, i);
    }
    return;
}

DWORD WINAPI ThreadExample1(LPVOID lpThreadParameter)
{
    int thread_decorator = (int)lpThreadParameter;
    DoImportantWork(thread_decorator, 100);
    return 0;
}

DWORD WINAPI ThreadCounterNoSync(LPVOID lpThreadParameter)
{
    int count = (int)lpThreadParameter;
    for (int i = 0; i < count; i++)
    {
        g_Counter++;
    }
    
    return 0;
}

DWORD WINAPI ThreadCounterInterlock(LPVOID lpThreadParameter)
{
    int count = (int)lpThreadParameter;

    for (int i = 0; i < count; i++)
    {
        InterlockedIncrement64(&g_Counter);
    }

    return 0;
}

CRITICAL_SECTION g_CS;
DWORD WINAPI ThreadCounterCS(LPVOID lpThreadParameter)
{
    int count = (int)lpThreadParameter;

    for (int i = 0; i < count; i++)
    {
        EnterCriticalSection(&g_CS);
        g_Counter++;
        LeaveCriticalSection(&g_CS);
    }

    return 0;
}

HANDLE g_Mutex;
DWORD WINAPI ThreadCounterMutex(LPVOID lpThreadParameter)
{
    int count = (int)lpThreadParameter;

    for (int i = 0; i < count; i++)
    {
        WaitForSingleObject(g_Mutex, INFINITE);
        g_Counter++;
        ReleaseMutex(g_Mutex);
    }

    return 0;
}


#define TEST_THREADS_COUNT 10

int main()
{
    printf("\n[Hello from Threads experiments area]\n");
    HANDLE all_threads[TEST_THREADS_COUNT];

    InitializeCriticalSection(&g_CS);
    g_Mutex = CreateMutexW(NULL, FALSE, NULL);

    ZeroMemory(all_threads, sizeof(all_threads));
    g_Counter = 0;

    for (int i = 0; i < TEST_THREADS_COUNT; i++)
    {
        all_threads[i] = CreateThread(NULL, 0, ThreadCounterMutex, 10000000, 0, NULL);
    }

    for (int i = 0; i < TEST_THREADS_COUNT; i++)
    {
        WaitForSingleObject(all_threads[i], INFINITE);
    }

    printf("\n[RESULT= %lld]\n", g_Counter);


    return 0;
}