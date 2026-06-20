/*
 * This provides the socket setup and
 * listen loop abstraction.
 *
 * This is to simplify the process, and
 * make the main focus the server to client
 * logic, instead.
 */
#pragma once

/* Local */
#include "../types.h"
#include "../debug/logger.h"
/* Standard Library */
#include <stdio.h>
#include <stdlib.h>
/* UNIX Libraries */
#include <unistd.h>

#ifdef __cplusplus
#   define restrict __restrict
#else
#   define restrict restrict
#endif

/*
 * This is to initialize the socket.
 *
 * This needs a valid server_config_t first.
 * So we have to call setConf() first before
 * initializing anything.
 *
 * This returns an int with errors:
 * -2 = Log not opened and error
 * -1 = Error, but logged.
 *  0 = Normal exit
 *  1 = No errors, no log.
 */
int initSocket(server_config_t *restrict c);

inline void freeConf(server_config_t *restrict c) {
    if (c->sockFD)  close(c->sockFD);
    if (c->logFile) fclose(c->logFile);
}

static inline void listenConnect(server_config_t *restrict c) {
    unsigned int len = sizeof(c->cliAddr);

Start:
    if (*c->program_abort) {
        logWithTime(
            "Abort signal recieved.",
            false,
            c->logFile
        );
        goto CleanupAndAbort;
    }
    if (listen(c->sockFD, 1) == -1) {
        logWithTime(
            "(SOCKET LISTENER) Socket listen() has failed, shutting down...",
            false,
            c->logFile
        );
        goto CleanupAndAbort;
    } logWithTime(
        "(SOCKET LISTENER) Socket has heard a connection, attempting accept...",
        false,
        c->logFile
    );

    c->connFD = accept(c->sockFD, (struct sockaddr*) &c->cliAddr, &len);
    if (*c->program_abort) {
        logWithTime(
            "Abort signal recieved.",
            false,
            c->logFile
        );
        goto CleanupAndAbort;
    }
    if (c->connFD == -1) {
        logWithTime(
            "(SOCKET LISTENER) Connection failed, resetting...",
            false,
            c->logFile
        );
        goto Start;
    } logWithTime(
        "(SOCKET LISTENER) Successful connection! Reading bytes...",
        false,
        c->logFile
    );
    fflush(c->logFile);
    return;

CleanupAndAbort:
    fflush(c->logFile);
    freeConf(c);
    _exit(1);
}

inline void writeConsumeU16(
    const int          connFD,
    uint16_t *restrict buf,
    FILE     *restrict logOut
) {
    if (write(
        connFD, buf + 1, *buf
    ) == -1) logWithTime(
        "(PACKET WRITER) Sending packet failed...",
        false,
        logOut
    ); else logWithTime(
        "(PACKET WRITER) Sent packet successfully",
        false,
        logOut
    );
    if (buf) free(buf);
}

inline void writeConsumeVoid(
    const int      connFD,
    const int      bufSize,
    void *restrict buf,
    FILE *restrict logOut
) {
    if (write(
        connFD, buf, bufSize
    ) == -1) logWithTime(
        "(PACKET WRITER) Sending packet failed...",
        false,
        logOut
    );
    else logWithTime(
        "(PACKET WRITER) Sent packet successfully!",
        false,
        logOut
    );
    free(buf);
}

void readPacket(
    const int                 connFD,
          ssize_t   *restrict readB,
    uint8_t         *restrict byteBuf,
    FILE            *restrict logOut,
    server_config_t *restrict c
);
