#include "trace_log.h"
#include <stdarg.h>
#include <string.h>
#include <ctype.h>


static LOG_level_t  current_log_level = LOG_WARNING;
static FILE *       current_log_file  = NULL;
static const char * trace_level_str[] = {"INFO  ", "DEBUG ", "WARN  ", "ERROR ", "CRITIC"};


#ifdef __TRACE_LOG_MULTITHREAD
    #include <pthread.h>

    static pthread_mutex_t io_protector = PTHREAD_MUTEX_INITIALIZER;

    static inline void lock_logger(void)
    {
        pthread_mutex_lock(&io_protector);
    }

    static inline void unlock_logger(void)
    {
        pthread_mutex_unlock(&io_protector);
    }

#else
    #define lock_logger()
    #define unlock_logger()
#endif

#ifdef __TRACE_LOG_DATETIME

    #include <sys/time.h>
    #include <time.h>
    
    static inline void time_prefix(void)
    {
        struct timeval  tv;
        (void)gettimeofday(&tv, NULL);
        struct tm * ti;
        ti = localtime(&tv.tv_sec);
        fprintf(current_log_file, "[%04u/%02u/%02u %02u:%02u:%02u:%03lu] ", 
                                  ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday,
                                  ti->tm_hour, ti->tm_min, ti->tm_sec, tv.tv_usec/1000);
    }

#else
    #define time_prefix()
#endif

#define init_logger()                   \
    do{                                 \
        if(!current_log_file)           \
            current_log_file = stdout;  \
    }while(0)


void trace_log(LOG_level_t level, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    if(level > LOG_CRITICAL)
    {
        level = LOG_CRITICAL;
    }

    lock_logger();
    init_logger();
    if(level >= current_log_level)
    {
        fprintf(current_log_file, "[%s] ", trace_level_str[level]);
        time_prefix();
        vfprintf(current_log_file, fmt, args);
        fprintf(current_log_file, "\n");
        fflush(current_log_file);
    }
    unlock_logger();

    va_end(args);
}

void trace_log_setlevel(LOG_level_t level)
{
    lock_logger();
    init_logger();
    current_log_level = level;
    unlock_logger();
}

LOG_level_t trace_log_getlevel(void)
{
    lock_logger();
    init_logger();
    LOG_level_t result = current_log_level;
    unlock_logger();
    return result;
}

void trace_log_set_output(FILE *fp)
{
    lock_logger();
    init_logger();
    if(fp)
    {
        current_log_file = fp;
    }
    unlock_logger();
}

void trace_log_show_buf(LOG_level_t level, void * data, uint32_t size, const char *comment, ...)
{
    va_list args;
    va_start(args, comment);

    if(level > LOG_CRITICAL)
    {
        level = LOG_CRITICAL;
    }

    lock_logger();
    init_logger();
    if(level >= current_log_level)
    {
        fprintf(current_log_file, "[%s] ", trace_level_str[level]);
        time_prefix();
        vfprintf(current_log_file, comment, args);
        if(strlen(comment))
        {
            fprintf(current_log_file, " ");
        }
        for(uint32_t i = 0; i < size; i++)
        {
            fprintf(current_log_file, "%02X ", ((uint8_t*)data)[i]);
        }
        fprintf(current_log_file, "\n");
        fflush(current_log_file);
    }
    unlock_logger();

    va_end(args);
}

void trace_log_show_buflong(LOG_level_t level, void * data, uint32_t size, const char *comment, ...)
{
    va_list args;
    va_start(args, comment);

    if(level > LOG_CRITICAL)
    {
        level = LOG_CRITICAL;
    }

    uint32_t lines = size / 0x10 + ((size % 0x10) ? 1 : 0);

    lock_logger();
    init_logger();
    if(level >= current_log_level)
    {
        fprintf(current_log_file, "[%s] ", trace_level_str[level]);
        time_prefix();
        vfprintf(current_log_file, comment, args);
        fprintf(current_log_file, "\n");

        for(uint32_t j = 0; j < lines; j++)
        {
            fprintf(current_log_file, "\t%08X  ", j * 0x10);
            for(uint32_t i = 0; i < 0x08; i++)
            {
                if(j * 0x10 + i < size)
                {
                    fprintf(current_log_file, "%02X ", ((uint8_t*)data)[j * 0x10 + i]);
                }
                else
                {
                    fprintf(current_log_file, "   ");
                }
            }
            fprintf(current_log_file, " ");
            for(uint32_t i = 0x08; i < 0x10; i++)
            {
                if(j * 0x10 + i < size)
                {
                    fprintf(current_log_file, "%02X ", ((uint8_t*)data)[j * 0x10 + i]);
                }
                else
                {
                    fprintf(current_log_file, "   ");
                }
            }
            fprintf(current_log_file, " ");
            for(uint32_t i = 0; i < 0x10; i++)
            {
                if(j * 0x10 + i < size && isprint(((uint8_t*)data)[j * 0x10 + i]))
                {
                    fprintf(current_log_file, "%c", ((uint8_t*)data)[j * 0x10 + i]);
                }
                else
                {
                    fprintf(current_log_file, " ");
                }
            }
            fprintf(current_log_file, "\n");
        }
        fflush(current_log_file);
    }
    unlock_logger();

    va_end(args); 
}