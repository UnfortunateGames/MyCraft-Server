/*
 * This header will only define the only logger.
 * I haven't thought of anything else to implement.
 *
 * - ESplash
 */

#pragma once

/* Standard Library */
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
#   define restrict __restrict
#else
#   define restrict restrict
#endif

/*
 * This will log to logFile. This should also
 * append a newline, so no need for a newline
 * This should log with the structure of:
 *
 * [ TIME ] msg
 *
 * If the extraDebug is true, then it will add:
 *
 * [ TIME ] (DEBUG) msg
 */
void logWithTime(
    const char *restrict msg,
          bool           extraDebug,
          FILE *restrict logFile
);

