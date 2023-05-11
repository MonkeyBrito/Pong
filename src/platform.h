#pragma once

#include <stdint.h>

void platform_get_window_size(uint32_t *width, uint32_t *height);
const char *platform_read_file(const char *path, uint32_t *lenght);