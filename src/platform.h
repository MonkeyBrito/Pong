#pragma once

#include <stdint.h>

typedef enum TextColor {
    TEXT_COLOR_WHITE,
    TEXT_COLOR_GREEN,
    TEXT_COLOR_YELLOW,
    TEXT_COLOR_RED,
    TEXT_COLOR_LIGHT_RED
} TextColor;

void platform_get_window_size(uint32_t *width, uint32_t *height);

const char *platform_read_file(const char *path, uint32_t *lenght);

void platform_log(const char *msg, TextColor color);