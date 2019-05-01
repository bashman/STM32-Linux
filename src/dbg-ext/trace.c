/*
*****************************************************************************
**
**  (C) 2019 Andrii Bilynskyi <andriy.bilynskyy@gmail.com>
**
**  This code is licenced under the MIT.
**
*****************************************************************************
*/

#include "trace.h"
#include <SEGGER_RTT.h>
#include <string.h>
#include <ctype.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

static volatile LOG_level_t trace_level = LOG_DEBUG;
static bool trace_inited = false;
static SemaphoreHandle_t xSemaphore = NULL;

#define GENERATE_STRING(STRING) #STRING,

static const char * dbg_level_str[] = {
    FOREACH_DBGLEV(GENERATE_STRING)
    NULL
};

#undef GENERATE_STRING

void trace_init(void)
{
    if(!trace_inited)
    {
        SEGGER_RTT_Init();
        xSemaphore = xSemaphoreCreateMutex();
        if(xSemaphore)
        {
            trace_inited = true;
        }else{
            (void)SEGGER_RTT_WriteString(0, "can't create mutex. loger is inactive.\n");
        }
    }
}

void trace_deinit(void)
{
    if(trace_inited)
    {
        vSemaphoreDelete(xSemaphore);
        trace_inited = false;
    }
}

void trace_printf(LOG_level_t level, const char * sFormat, ...)
{
    va_list args;
    va_start(args, sFormat);
    if(level > LOG_CRITICAL)
    {
        level = LOG_CRITICAL;
    }
    if(level >= trace_level && trace_inited)
    {
        xSemaphoreTake(xSemaphore, portMAX_DELAY);
        (void)SEGGER_RTT_printf(0, "[%08u]", xTaskGetTickCount());
        (void)SEGGER_RTT_WriteString(0, " [");
        (void)SEGGER_RTT_WriteString(0, dbg_level_str[level]);
        (void)SEGGER_RTT_WriteString(0, "] ");
        (void)SEGGER_RTT_vprintf(0, sFormat, &args);
        SEGGER_RTT_PutChar(0, '\n');
        xSemaphoreGive(xSemaphore);
    }
    va_end(args);
}

void trace_show_buf(LOG_level_t level, void * data, unsigned int size, const char *comment, ...)
{
    va_list args;
    va_start(args, comment);

    if(level > LOG_CRITICAL)
    {
        level = LOG_CRITICAL;
    }

    if(level >= trace_level && trace_inited)
    {
        xSemaphoreTake(xSemaphore, portMAX_DELAY);
        (void)SEGGER_RTT_printf(0, "[%08u]", xTaskGetTickCount());
        (void)SEGGER_RTT_WriteString(0, " [");
        (void)SEGGER_RTT_WriteString(0, dbg_level_str[level]);
        (void)SEGGER_RTT_WriteString(0, "] ");
        (void)SEGGER_RTT_vprintf(0, comment, &args);
        for(uint32_t i = 0; i < size; i++)
        {
            (void)SEGGER_RTT_printf(0, " %02X", ((uint8_t*)data)[i]);
        }
        SEGGER_RTT_PutChar(0, '\n');
        xSemaphoreGive(xSemaphore);
    }
    va_end(args);
}

void trace_show_buflong(LOG_level_t level, void * data, unsigned int size, const char *comment, ...)
{
    va_list args;
    va_start(args, comment);

    if(level > LOG_CRITICAL)
    {
        level = LOG_CRITICAL;
    }

    unsigned int lines = size / 0x10 + ((size % 0x10) ? 1 : 0);

    if(level >= trace_level && trace_inited)
    {
        xSemaphoreTake(xSemaphore, portMAX_DELAY);
        (void)SEGGER_RTT_printf(0, "[%08u]", xTaskGetTickCount());
        (void)SEGGER_RTT_WriteString(0, " [");
        (void)SEGGER_RTT_WriteString(0, dbg_level_str[level]);
        (void)SEGGER_RTT_WriteString(0, "] ");
        (void)SEGGER_RTT_vprintf(0, comment, &args);
        SEGGER_RTT_PutChar(0, '\n');

        for(unsigned int j = 0; j < lines; j++)
        {
            (void)SEGGER_RTT_printf(0, "\t%08X  ", j * 0x10);
            for(unsigned int i = 0; i < 0x08; i++)
            {
                if(j * 0x10 + i < size)
                {
                    (void)SEGGER_RTT_printf(0, "%02X ", ((uint8_t*)data)[j * 0x10 + i]);
                }
                else
                {
                    (void)SEGGER_RTT_printf(0, "   ");
                }
            }
            (void)SEGGER_RTT_printf(0, " ");
            for(unsigned int i = 0x08; i < 0x10; i++)
            {
                if(j * 0x10 + i < size)
                {
                    (void)SEGGER_RTT_printf(0, "%02X ", ((uint8_t*)data)[j * 0x10 + i]);
                }
                else
                {
                    (void)SEGGER_RTT_printf(0, "   ");
                }
            }
            (void)SEGGER_RTT_printf(0, " ");
            for(unsigned int i = 0; i < 0x10; i++)
            {
                if(j * 0x10 + i < size && isprint(((uint8_t*)data)[j * 0x10 + i]))
                {
                    (void)SEGGER_RTT_printf(0, "%c", ((uint8_t*)data)[j * 0x10 + i]);
                }
                else
                {
                    (void)SEGGER_RTT_printf(0, " ");
                }
            }
            SEGGER_RTT_PutChar(0, '\n');
        }
        xSemaphoreGive(xSemaphore);
    }
    va_end(args); 
}

void trace_set_level(LOG_level_t level)
{
    if(level > LOG_CRITICAL)
    {
        level = LOG_CRITICAL;
    }
    trace_level = level;
}

LOG_level_t trace_get_level(void)
{
    return trace_level;
}

bool trace_set_level_str(const char * level)
{
    bool result = false;
    for(unsigned int i = 0; !result && dbg_level_str[i]; i++)
    {
        if(!strcmp(level, dbg_level_str[i]))
        {
            trace_level = i;
            result = true;
        }
    }
    return result;
}

const char * trace_get_level_str()
{
    return dbg_level_str[trace_level];
}

const char * trace_get_available_level_str(unsigned int index)
{
    const char * result = NULL;
    for(unsigned int i = 0; !result && dbg_level_str[i]; i++)
    {
        if(index == i)
        {
            result = dbg_level_str[i];
        }
    }
    return result; 
}