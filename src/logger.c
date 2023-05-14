#include "logger.h"

void _log(const char *prefix, TextColor color, const char *msg, ...) {
    char fmt_buffer[31000] = {0};
    char msg_buffer[32000] = {0};

    __builtin_va_list arg_ptr;
    va_start(arg_ptr, msg);
    vsprintf(fmt_buffer, msg, arg_ptr);
    va_end(arg_ptr);

    sprintf(msg_buffer, "%s: %s\n", prefix, fmt_buffer);

    platform_log(msg_buffer, color);
}