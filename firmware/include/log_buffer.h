#pragma once

#define LOG_LINES 30
#define LOG_COLS 200

void logPrintf(const char *fmt, ...);
int getLogCount();
const char *getLogLine(int index);
