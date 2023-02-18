#include "net.h"

NetAdapter netAdapter{};

int yj_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    printf("[yojimbo] ");
    int result = vprintf(format, args);
    va_end(args);
    return result;
}