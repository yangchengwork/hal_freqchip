#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#include "co_log.h"

void log_printf_level(const char *level)
{
#if defined(__ARMCC_VERSION) || defined(__CC_ARM)
    fputs(level, &__stdout);
#endif
}

void log_printf(const char* tag,
                            const char* file_name,
                            uint32_t line,
                            const char *format, ...)
{
    va_list args;

    if(tag)
        printf("[%s] ",tag);
    
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

