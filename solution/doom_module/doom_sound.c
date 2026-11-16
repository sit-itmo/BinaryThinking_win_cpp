#include "doom_internal.h"

int SoundBuffer_FillAndSubmit(SoundBuffer* p_sound, int i)
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

    if (p_sound == NULL || g_DOOM_SoundReady == 0)
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
