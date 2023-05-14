#pragma once

#include "platform.h"

#include <stdarg.h>
#include <stdio.h>
#include <intrin.h> // __debugbreak

void _log(const char *prefix, TextColor color, const char *msg, ...);

#define CAKEZ_TRACE(msg, ...) _log("TRACE", TEXT_COLOR_GREEN, msg, ##__VA_ARGS__)
#define CAKEZ_WARN(msg, ...) _log("WARN ", TEXT_COLOR_YELLOW, msg, ##__VA_ARGS__)
#define CAKEZ_ERROR(msg, ...) _log("ERROR", TEXT_COLOR_RED, msg, ##__VA_ARGS__)
#define CAKEZ_FATAL(msg, ...) _log("FATAL", TEXT_COLOR_LIGHT_RED, msg, ##__VA_ARGS__)

#ifdef _DEBUG
#define CAKEZ_ASSERT(x, msg, ...) {     \
    if (!(x)) {                         \
        CAKEZ_ERROR(msg, ##__VA_ARGS__);\
        __debugbreak();                \
    }                                   \
}
#else
#define CAKEZ_ASSERT(x, msg, ...)
#endif