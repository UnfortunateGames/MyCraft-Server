/* Local */
#include <internals/debug/logger.h>
/* Standard Library */
#include <stdbool.h>
#include <stdio.h>
/* Other libraries */
#include <time.h>
#include <string.h>

void logWithTime(
    const char *restrict msg,
          bool           extraDebug,
          FILE *restrict logFile
) {
    const time_t now     = time(NULL);
    struct tm    *nowF   = localtime(&now);

    const int lBufSize = 40 + strlen(msg);
    char timeBuf[20];
    char  logBuf[lBufSize];

    strftime(timeBuf, sizeof(timeBuf), "%b %d, %I:%M:%S %p", nowF);

    if (extraDebug) snprintf(
        logBuf, lBufSize,
        "[ %s ] (DEBUG) %s",
        timeBuf, msg
    );
    else snprintf(
        logBuf, lBufSize,
        "[ %s ] %s",
        timeBuf, msg
    );

    puts(logBuf);
    if (!logFile) return;
    fputs(logBuf, logFile);
    fputs("\n", logFile);
}

