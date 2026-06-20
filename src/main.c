/* POSIX define shenanigans */
#define _POSIX_C_SOURCE 199309L
/* Local */
#include "craftlib.h"
#include "internals/types.h"
/* Standard Library */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
/* UNIX Libraries */
#include <unistd.h>
/* Other Libraries */
#include <memory.h>
#include <pthread.h>
#include <signal.h>

#ifdef __cplusplus
#   define restrict __restrict
#else
#   define restrict restrict
#endif

volatile sig_atomic_t program_abort = 0;

void setabort(void) {
    program_abort = 1;
}

#define MAX                128
#define SERVER_DESC        "MyCraft build 3"
#define SERVER_MAX_PLAYERS 1                 // This is just for now :D
int main(void) {
    server_mode_t m = sm_awaiting;
    server_config_t c = {
        .logFile       = fopen("./server.log", "a"),
        .pathRoot      = ".",
        .program_abort = &program_abort
    };
    int globalPlayerCount = 0;
    uint8_t  byteBuf[MAX],
             *bytePtr;
    uint16_t *shortBuf;
    char     *userBuf = NULL;
    ssize_t  readB;

    switch (initSocket(&c)) {
        case 1:
            logWithTime(
                "(SERVER INIT) No errors! Though log was not created.",
                false,
                c.logFile
            );
        case 0:
            break;
        case -1:
            logWithTime(
                "(SERVER INIT) An error occurred, you may report this with the log.",
                false,
                c.logFile
            );
            freeConf(&c);
            return 1;
        case -2:
            logWithTime(
                "(SERVER INIT) An error occurred, no log file was created, please record this output.",
                false,
                c.logFile
            );
            freeConf(&c);
            return 1;
        default:
            logWithTime(
                "(SERVER INIT) initSocket() function was updated, but return handler was not.\n",
                false,
                c.logFile
            );
            freeConf(&c);
            return 1;
    }

    switch (initWorldStorage(c.pathRoot, c.logFile)) {
        case 1:
            logWithTime(
                "(SERVER INIT) Existing world storage found! Be sure it is valid.",
                false,
                c.logFile
            );
        case 0:
            break;
        case -1:
            freeConf(&c);
            return 1;
        default:
            logWithTime(
                "(SERVER INIT) World Storage creator has been updated, but not the return handler.",
                false,
                c.logFile
            );
            freeConf(&c);
            return 1;
    }

    struct sigaction sa;
    sa.sa_handler = (void (*)(int)) &setabort;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

Relisten:
    listenConnect(&c);
    if (program_abort) goto Exit;

RereadBuf:
    readPacket(c.connFD, &readB, byteBuf, c.logFile, &c);
    //*
    logWithTime(
        "(STATUS HANDLER) Packet Stream:",
        false,
        c.logFile
    );
    for (ssize_t i = 0; i < readB; i++) {
        if (i % 8 == 0 && i != 0) {
            puts("");
            fputs("\n", c.logFile);
        }
        printf("%02X ", byteBuf[i]);
        fprintf(c.logFile, "%02X ", byteBuf[i]);
    }
    puts("");
    fputs("\n", c.logFile);
    //*/

ResetReader:
    if (program_abort) goto Exit;
    switch (m) {
        case sm_awaiting:
            switch (byteBuf[0]) {
                case pid_statusPacket:
                    logWithTime(
                        "(STATUS HANDLER) Status packet received, returning status.",
                        false,
                        c.logFile
                    );
                    shortBuf = utf16_packetConv(
                        fetchServerStatus(
                            SERVER_DESC,
                            globalPlayerCount,
                            SERVER_MAX_PLAYERS
                        ),
                        pid_kick
                    );
                    writeConsumeU16(c.connFD, shortBuf, c.logFile);
                    shortBuf = NULL;
                    break;
                case pid_handshake:
                    if (globalPlayerCount >= SERVER_MAX_PLAYERS) {
                        logWithTime(
                            "(STATUS HANDLER) Handshake recieved, but server is full, disconnecting...",
                            false,
                            c.logFile
                        );
                        shortBuf = utf16_packetConv("Server is full.", pid_kick);
                        writeConsumeU16(c.connFD, shortBuf, c.logFile);
                        goto Relisten;
                    }
                    logWithTime(
                        "(STATUS HANDLER) Handshake packet received! Switching modes...",
                        false,
                        c.logFile
                    );
                    m = sm_login;
                    goto ResetReader;
                default:
                    logWithTime(
                        "(STATUS HANDLER) Client server desync! Sending kick packet and Relistening...",
                        false,
                        c.logFile
                    );
                    shortBuf = utf16_packetConv(
                        "Server error, please try again later.",
                        pid_kick
                    );
                    writeConsumeU16(c.connFD, shortBuf, c.logFile);
                    shortBuf = NULL;
            }
            goto Relisten;
        case sm_login:
            if (program_abort) goto Exit;
            switch (byteBuf[0]) {
                case pid_handshake:
                    {
                        char logBuf[64];
                        const int mSize = readB % 2 == 0 ? readB : readB + 1;
                        shortBuf = calloc(1, mSize);
                        memcpy(shortBuf, byteBuf + 1, readB - 1);
                        shortReverser(shortBuf, mSize / 2);
                        userBuf = utf16_sConv(shortBuf);
                        snprintf(
                            logBuf, sizeof(logBuf),
                            "(STATUS HANDLER) New login: %s", userBuf
                        );
                        logWithTime(
                            logBuf,
                            false,
                            c.logFile
                        );
                        fflush(c.logFile);
                    }
                    free(shortBuf);
                    shortBuf = utf16_packetConv("-", pid_handshake);
                    for (int i = 0; i < 3; i++) {
                      printf("%04X ", shortBuf[i]);
                    } puts("");
                    writeConsumeU16(c.connFD, shortBuf, c.logFile);
                    goto RereadBuf;
                case pid_loginResponse: {
                    logWithTime(
                        "(STATUS HANDLER) Writing login response packet...",
                        false,
                        c.logFile
                    );
                    /* Skip all the details of the login request */
                    uint16_t userLen;
                    memcpy(&userLen, byteBuf + 5, 2);
                    shortReverser(&userLen, 1);
                    readB = read(c.connFD, byteBuf + 7, userLen);
                    shortBuf = calloc(1, 2 + readB);
                    memcpy(shortBuf, byteBuf + 5, 2 + readB);
                    userBuf = utf16_sConv(shortBuf);
                    readB = read(c.connFD, byteBuf + 7 + readB, 15);
                    printf("Read Bytes: %zu\n", readB);
                    login_res_pack_t *lrPacket = calloc(1, sizeof(login_res_pack_t));
                    lrPacket->packetID     = pid_loginResponse;
                    lrPacket->playerEntID  = intReverser(1);
                    lrPacket->unusedStr[0] = 1 << 8; /* Reverse the endian */
                    lrPacket->seed         = longReverser(69420);
                    lrPacket->worldHeight  = 128;
                    lrPacket->maxPlayers   = SERVER_MAX_PLAYERS;
                    /* All other values are zero. */
                    writeConsumeVoid(c.connFD, sizeof(login_res_pack_t),  lrPacket, c.logFile);
                    if (program_abort) goto DisconnectRelisten;
                    goto RereadBuf;
                }
                case pid_kick:
                  logWithTime(
                      "(STATUS HANDLER) Client cancelled handshake...",
                      false,
                      c.logFile
                  );
                  m = sm_awaiting;
                  goto Relisten;
                default:
                    break;
            }
            {
                uint32_t *spawnCoords = findPlayer(
                    c.pathRoot,
                    userBuf,
                    c.logFile
                );
                if (userBuf) free(userBuf);
                userBuf = NULL;
                if (program_abort) goto DisconnectRelisten;
                logWithTime(
                    "(STATUS HANDLER) Writing spawn position packet...",
                    false,
                    c.logFile
                );
                shortBuf = spawnPPacket(spawnCoords);
                writeConsumeU16(c.connFD, shortBuf, c.logFile);
                shortBuf = NULL;
                if (program_abort) goto DisconnectRelisten;
                logWithTime(
                    "(STATUS HANDLER) Writing spawn chunk...",
                    false,
                    c.logFile
                );
                FILE *fChunk = fopen(
                    findChunkF(c.pathRoot, c.logFile),
                    "rb"
                );
                bytePtr = fetchChunk(fChunk, c.logFile);
                if (!fChunk) goto DisconnectRelisten;
                fclose(fChunk);
                fChunk = NULL;
                uint8_t* pChunk = chunkComp(bytePtr, c.logFile);
                const int pSize = *((int*) pChunk);
                uint8_t *p = chunkPckt(pChunk, spawnCoords);
                writeConsumeVoid(
                    c.connFD,
                    pSize + 18,
                    p,
                    c.logFile
                );
                free(pChunk);
                free(spawnCoords);
            }
            if (program_abort) goto DisconnectRelisten;
            m = sm_playing;
            goto ResetReader;
        case sm_playing:
            {
                logWithTime(
                    "(STATUS HANDLER) Creating new play thread.",
                    false,
                    c.logFile
                );
                pthread_t         t;
                playThread_conf_t *ptConf = malloc(sizeof(playThread_conf_t));
                ptConf->serv_c        = c;
                ptConf->gpcPtr        = &globalPlayerCount;
                const int rval = pthread_create(
                    &t,
                    NULL,
                    playingThread,
                    ptConf
                );
                if (rval != 0) {
                    logWithTime(
                        "(STATUS HANDLER) Could not create thread, disconnecting...",
                        false,
                        c.logFile
                    );
                    goto DisconnectRelisten;
                }
                pthread_detach(t);
                logWithTime(
                    "(STATUS HANDLER) Successfully created thread.",
                    false,
                    c.logFile
                );
            }
            m = sm_awaiting;
            break;
    }
    goto Relisten;

DisconnectRelisten:
    m = sm_awaiting;
    shortBuf = utf16_packetConv("Internal Server Error...", pid_kick);
    writeConsumeU16(c.connFD, shortBuf, c.logFile);
    shortBuf = NULL;
    goto Relisten;

Exit:
    logWithTime(
        "Shutdown signal recieved, exitting gracefully.",
        false,
        c.logFile
    );
    fflush(stdout);
    fflush(c.logFile);
    freeConf(&c);
    return 0;
}
