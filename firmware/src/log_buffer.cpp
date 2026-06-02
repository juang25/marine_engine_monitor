#include <Arduino.h>
#include <stdarg.h>
#include <string.h>
#include "log_buffer.h"

static char lines[LOG_LINES][LOG_COLS];
static int head = 0;
static int count = 0;

void logPrintf(const char *fmt, ...)
{
    char buf[LOG_COLS];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, LOG_COLS, fmt, args);
    va_end(args);

    Serial.print(buf);

    strncpy(lines[head], buf, LOG_COLS - 1);
    lines[head][LOG_COLS - 1] = '\0';

    head = (head + 1) % LOG_LINES;
    if (count < LOG_LINES)
        count++;
}

int getLogCount()
{
    return count;
}

const char *getLogLine(int index)
{
    if (index < 0 || index >= count)
        return "";

    int pos;
    if (count < LOG_LINES)
        pos = index;
    else
        pos = (head + index) % LOG_LINES;

    return lines[pos];
}
