Далее по шагам написано как собрать все необходимое для примера с DOOM.
### 1. Что скачать и установить

1. **Visual Studio 2022 Community** (или Build Tools)  
    При установке выбери workload **Desktop development with C++**.
    
2. **CMake**  
    [cmake.org/download](https://cmake.org/download/)  
    При установке поставь галочку «Add CMake to the system PATH».
    
3. **Git**  
    [git-scm.com/download/win](https://git-scm.com/download/win)
    
4. **Vulkan SDK**  
    [vulkan.lunarg.com/sdk/home](https://vulkan.lunarg.com/sdk/home)  
    Ставь в стандартный путь (`C:\VulkanSDK\...`). После установки перезагрузи ПК или хотя бы открой новый терминал.
    
5. **Модель** (скачай отдельно):
    
    - Рекомендую начать с:  
        [huggingface.co/stable-diffusion-v1-5/stable-diffusion-v1-5/resolve/main/v1-5-pruned-emaonly.safetensors](https://huggingface.co/stable-diffusion-v1-5/stable-diffusion-v1-5/resolve/main/v1-5-pruned-emaonly.safetensors)
    - Для скорости потом можно взять SD-Turbo / LCM-версии.

### 2. Сборка stable-diffusion.cpp с Vulkan

Открой **Developer PowerShell for VS 2022** (важно!) и выполни:

PowerShell

```
cd path\to\sourcesstable-diffusion.cpp
rmdir /s /q build
mkdir build
cd build

cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DSD_BUILD_SHARED_LIBS=ON ^
  -DSD_VULKAN=ON ^
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL

cmake --build . --config Release -- /m
```
После успешной сборки появится:

- `build\bin\Release\sd-cli.exe` (или просто `build\bin\sd-cli.exe`)

Проверь, что Vulkan видит твою видеокарту:

PowerShell

```
.\sd-cli.exe --help
# или просто запусти с моделью
.\sd-cli.exe -m "C:\path\to\v1-5-pruned-emaonly.safetensors" -p "a cat" -o test.png
```

Если пишет что-то про Vulkan0 / AMD Radeon 780M — всё хорошо.

### 3. Быстрый тест img2img через командную строку

PowerShell

```
.\sd-cli.exe -m "C:\models\v1-5-pruned-emaonly.safetensors" `
  -i "C:\input.png" `
  -o "C:\output.png" `
  -p "oil painting style, thick brush strokes, artistic" `
  --strength 0.6 `
  --steps 8 `
  -W 512 -H 512
```

Параметры:

- `--strength` 0.3–0.8 (чем выше — тем сильнее меняет картинку)
- `--steps` 4–12 для turbo/LCM, 15–25 для обычной SD 1.5
### 4. Сборка DOOM

Открой **VS 2022** и открой solution\solution.sln в ней. Далее просто все пересобери как x64 и запусти doom_app.
