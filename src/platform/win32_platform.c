#include <windows.h>

#include "../defines.h"
#include "../platform.h"

#include "../renderer/vk_renderer.h"
#include <stdio.h>
#include <stdlib.h>

global_variable bool running = true;
global_variable HWND window;

LRESULT CALLBACK platform_window_callback(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE:
        running = false;
        break;
    }

    return DefWindowProcA(window, msg, wParam, lParam);
}

bool platform_create_window(void) {
    HINSTANCE instance = GetModuleHandleA(0);

    WNDCLASS wc = {0};
    wc.lpfnWndProc = platform_window_callback;
    wc.hInstance = instance;
    wc.lpszClassName = "vulkan_engine_class";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassA(&wc)) {
        MessageBoxA(0, "failed registering window class", "Error", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    window = CreateWindowExA(
        WS_EX_APPWINDOW,
        "vulkan_engine_class",
        "Pong",
        WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_OVERLAPPED | WS_THICKFRAME,
        100, 100, 1280, 720,
        0, 0, instance, 0
    );
    if (window == 0) {
        MessageBoxA(0, "failed creating window", "Error", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    ShowWindow(window, SW_SHOW);

    return true;
}

void platform_update_window(HWND window) {
    MSG msg;
    while (PeekMessageA(&msg, window, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

int main(void) {
    VkContext vk_context = {0};
    if (!platform_create_window()) {
        return -1;
    }

    if (!vk_init(&vk_context, window)) {
        return -1;
    }

    while (running) {
        platform_update_window(window);
        if (!vk_render(&vk_context)) {
            return -1;
        }
    }

    return 0;
}

void platform_get_window_size(uint32_t *width, uint32_t *height) {
    RECT rect;
    GetClientRect(window, &rect);

    *width = (uint32_t)(rect.right - rect.left);
    *height = (uint32_t)(rect.bottom - rect.top);
}

const char *platform_read_file(const char *path, uint32_t *lenght) {
    char *result = NULL;

    HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER size;
        if (GetFileSizeEx(file, &size)) {
            *lenght = (uint32_t)size.QuadPart; // 4GB LIMIT
            result = malloc(*lenght * sizeof(char));
            if (result == NULL) {
                // TODO: Assert
                printf("Failed to allocate memory\n");
            }

            DWORD bytes_read;
            if (ReadFile(file, result, *lenght, &bytes_read, NULL)) {
                // Success
            } else {
                // TODO: Assert and error checking
                printf("Failed reading file: %s\n", path);
            }
        } else {
            // TODO: Assert
            printf("Failed getting size of file: %s\n", path);
        }

        CloseHandle(file);

    } else {
        // TODO: Assert
        printf("Failed opening file: %s\n", path);
    }

    return result;
}