#include "doom_internal.h"
#include "stable-diffusion.h"

//#define SD_MODEL_PATH   "C:\\models\\v1-5-pruned-emaonly.safetensors"
//#define SD_PROMPT       "oil painting style, thick brush strokes, artistic, vibrant colors"
//#define SD_WIDTH        512
//#define SD_HEIGHT       512
sd_ctx_t* g_ctx = NULL;

DWORD g_SD_WIDTH = 512;
DWORD g_SD_HEIGHT = 512;
CHAR g_SD_Prompt[1024];
CHAR g_SD_Negative[1024];
CHAR g_SD_ModelPath[MAX_PATH];

int StableDiffusion_Iteration(sd_image_t *p_init_img)
{
    if (p_init_img == NULL)
    {
        return 0;
    }


    CHAR r = 0;
    CHAR g = 0;
    CHAR b = 0;

    if (g_DOOM_DoomPixels.pPixels != NULL)
    {
        //r = rand() % 256;
        //g = rand() % 256;
        //b = rand() % 256;
        //
        //if (g_DOOM_ScreenPixels.pPixels)
        //{
        //    EnterCriticalSection(&g_DOOM_ScreenPixels.Lock);
        //    INT sd_area_width = g_DOOM_ScreenPixels.Size.cx / 2;
        //    INT sd_area_height = g_DOOM_ScreenPixels.Size.cy;
        //    INT sd_area_x = g_DOOM_ScreenPixels.Size.cx / 2;
        //    INT sd_area_y = 0;
        //
        //    for (INT y = 0; y < sd_area_height; y++)
        //    {
        //        for (INT x = 0; x < sd_area_width; x++)
        //        {
        //            INT src_x = x * g_SD_WIDTH / sd_area_width;
        //            INT src_y = y * g_SD_HEIGHT / sd_area_height;
        //            PixelBuffer_PutPixel(&g_DOOM_ScreenPixels, x + sd_area_x, y + sd_area_y,
        //                r, g, b);
        //        }
        //    }
        //    LeaveCriticalSection(&g_DOOM_ScreenPixels.Lock);
        //}

        EnterCriticalSection(&g_DOOM_DoomPixels.Lock);
        uint8_t* p_frame = (uint8_t*)g_DOOM_DoomPixels.pPixels;
        uint8_t* dst = p_init_img->data;
        for (int y = 0; y < (int)g_SD_HEIGHT; y++)
        {
            for (int x = 0; x < (int)g_SD_WIDTH; x++)
            {
                INT src_x = x * g_DOOM_DoomPixels.Size.cx / g_SD_WIDTH;
                INT src_y = y * g_DOOM_DoomPixels.Size.cy / g_SD_HEIGHT;
                const BYTE* p_src_color = g_DOOM_DoomPixels.pPixels
                    + (g_DOOM_DoomPixels.Size.cx * src_y + src_x) * 4;
                BYTE* p_dst_color = dst
                    + (g_SD_WIDTH * y + x) * 3;
                p_dst_color[0] = p_src_color[0]; // R
                p_dst_color[1] = p_src_color[1]; // G
                p_dst_color[2] = p_src_color[2]; // B
            }
        }
        LeaveCriticalSection(&g_DOOM_DoomPixels.Lock);

        sd_img_gen_params_t params;
        sd_img_gen_params_init(&params);
        params.prompt = g_SD_Prompt;
        params.negative_prompt = g_SD_Negative;
        memcpy(&params.init_image, p_init_img, sizeof(*p_init_img));
        params.strength = 0.25f;
        params.width = g_SD_WIDTH;
        params.height = g_SD_HEIGHT;
        params.sample_params.sample_steps = 10;
        params.sample_params.sample_method = EULER_A_SAMPLE_METHOD;
        params.seed = -1;

        sd_image_generation_result_t* result = NULL;
        if (generate_image(g_ctx, &params, &result) && result && result->items && result->items[0].image.data) {
            uint8_t* out = result->items[0].image.data;

            if (g_DOOM_ScreenPixels.pPixels)
            {
                EnterCriticalSection(&g_DOOM_ScreenPixels.Lock);
                INT sd_area_width = g_DOOM_ScreenPixels.Size.cx / 2;
                INT sd_area_height = g_DOOM_ScreenPixels.Size.cy;
                INT sd_area_x = g_DOOM_ScreenPixels.Size.cx / 2;
                INT sd_area_y = 0;

                for (INT y = 0; y < sd_area_height; y++)
                {
                    for (INT x = 0; x < sd_area_width; x++)
                    {
                        INT src_x = x * g_SD_WIDTH / sd_area_width;
                        INT src_y = y * g_SD_HEIGHT / sd_area_height;
                        const BYTE* p_color = out + (g_SD_WIDTH * src_y + src_x) * 3;
                        PixelBuffer_PutPixel(&g_DOOM_ScreenPixels, x + sd_area_x, y + sd_area_y,
                            p_color[2], p_color[1], p_color[0]);
                    }
                }
                LeaveCriticalSection(&g_DOOM_ScreenPixels.Lock);
            }

            free_sd_image_generation_result(result);
        }
    }
    else
    {
        if (g_DOOM_ScreenPixels.pPixels)
        {
            EnterCriticalSection(&g_DOOM_ScreenPixels.Lock);
            INT sd_area_width = g_DOOM_ScreenPixels.Size.cx / 2;
            INT sd_area_height = g_DOOM_ScreenPixels.Size.cy;
            INT sd_area_x = g_DOOM_ScreenPixels.Size.cx / 2;
            INT sd_area_y = 0;

            for (INT y = 0; y < sd_area_height; y++)
            {
                for (INT x = 0; x < sd_area_width; x++)
                {
                    INT src_x = x * g_SD_WIDTH / sd_area_width;
                    INT src_y = y * g_SD_HEIGHT / sd_area_height;
                    PixelBuffer_PutPixel(&g_DOOM_ScreenPixels, x + sd_area_x, y + sd_area_y,
                        r, g, b);
                }
            }
            LeaveCriticalSection(&g_DOOM_ScreenPixels.Lock);
        }
    }
    return 0;
}


DWORD StableDiffusion_Loop()
{
    sd_image_t init_img = { 0 };
    init_img.width = g_SD_WIDTH;
    init_img.height = g_SD_HEIGHT;
    init_img.channel = 3;
    init_img.data = (uint8_t*)malloc(g_SD_WIDTH * g_SD_HEIGHT * 3);

    while (g_DOOM_StableDiffusionEnabled)
    {
        if (g_DOOM_DoomPixels.pPixels == NULL)
        {
            continue;
        }
        StableDiffusion_Iteration(&init_img);
    }

    free(init_img.data);
    return 0;
}

int StableDiffusion_Setup(const char *p_model, const char* p_prompt, const char* p_negative, DWORD width, DWORD height)
{
    SetEnvironmentVariableA("GGML_VK_DISABLE_COOPMAT", "1");
    SetEnvironmentVariableA("GGML_VK_DISABLE_INTEGER_DOT", "1");

    sd_ctx_params_t ctx_params;
    sd_ctx_params_init(&ctx_params);

    snprintf(g_SD_ModelPath, sizeof(g_SD_ModelPath), "%s", p_model);
    snprintf(g_SD_Prompt, sizeof(g_SD_Prompt), "%s", p_prompt);
    snprintf(g_SD_Negative, sizeof(g_SD_Negative), "%s", p_negative);
    g_SD_ModelPath[sizeof(g_SD_ModelPath) - 1] = 0;
    g_SD_Prompt[sizeof(g_SD_Prompt) - 1] = 0;
    g_SD_Negative[sizeof(g_SD_Negative) - 1] = 0;


    ctx_params.model_path = g_SD_ModelPath;
    ctx_params.n_threads = 8;

    ctx_params.backend = "cpu";
    ctx_params.params_backend = "cpu";

    g_ctx = new_sd_ctx(&ctx_params);
    if (!g_ctx) 
    {
        return -1;
    }
    return 0;
}

void StableDiffusion_Destroy()
{
    if (g_ctx)
    {
        free_sd_ctx(g_ctx);
    }
}