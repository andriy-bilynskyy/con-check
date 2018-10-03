#ifndef __TRACE_LOG_H
#define __TRACE_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#define __TRACE_LOG_MULTITHREAD
#define __TRACE_LOG_DATETIME

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>


#define trace_log_printf(level, format, ...) trace_log(level, "%s:%03d " format, __FILE__, __LINE__, ##__VA_ARGS__)
#define trace_log_printbuf(level, ptr, size, comment, ...) trace_log_show_buf(level, ptr, size, "%s:%03d " comment, __FILE__, __LINE__, ##__VA_ARGS__)
#define trace_log_printbufl(level, ptr, size, comment, ...) trace_log_show_buflong(level, ptr, size, "%s:%03d " comment, __FILE__, __LINE__, ##__VA_ARGS__)
#define trace_log_assert(test, level, message)                                                  \
    do{                                                                                         \
        if(!(test))                                                                             \
            trace_log(level, "%s:%03d !!!ASSERT!!! [%s] " message, __FILE__, __LINE__, #test);  \
    }while(0)


typedef enum
{
    LOG_INFO,
    LOG_DEBUG,
    LOG_WARNING,
    LOG_ERROR,
    LOG_CRITICAL,
    LOG_OFF
}LOG_level_t;

void trace_log(LOG_level_t level, const char *fmt, ...);
void trace_vlog(LOG_level_t level, const char *comment, const char *fmt, va_list ap);
void trace_log_setlevel(LOG_level_t level);
LOG_level_t trace_log_getlevel(void);
void trace_log_set_output(FILE *fp);
void trace_log_show_buf(LOG_level_t level, void * data, uint32_t size, const char *comment, ...);
void trace_log_show_buflong(LOG_level_t level, void * data, uint32_t size, const char *comment, ...);

#ifdef __cplusplus
}
#endif

#endif