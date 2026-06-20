/* Local */
#include <internals/server/sockets.h>
#include <internals/types.h>
#include <internals/debug/logger.h>
/* Other libraries */
#include <memory.h>
/* Network Libraries */
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

#ifdef __cplusplus
#   define resrict __restrict
#else
#   define restrict restrict
#endif

#define PORT 25565
int initSocket(server_config_t *restrict c) {
    int rvalue = -1;
    if (!c->logFile)
        logWithTime(
            "(SOCKET CREATOR) Log file failed to open, please document this output.",
            false,
            c->logFile
        );
    c->sockFD = socket(AF_INET, SOCK_STREAM, 0);
    if (c->sockFD == -1) {
        logWithTime("", false, c->logFile);
        goto Cleanup;
    }

    memset(&c->servAddr, 0, sizeof(c->servAddr));
    c->servAddr.sin_family      = AF_INET;
    c->servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    c->servAddr.sin_port        = htons(PORT);

    if (bind(
        c->sockFD,
        (struct sockaddr*) &c->servAddr,
        sizeof(c->servAddr)
    ) != 0) {
        logWithTime(
            "(SOCKET CREATOR) Socket bind failed, another program might be also running.",
            false,
            c->logFile
        );
        goto Cleanup;
    }
    logWithTime("(SOCKET CREATOR) Socket created successfully.", false, c->logFile);
    rvalue = 0;
    if (!c->logFile) rvalue = 1;

Cleanup:
    fflush(c->logFile);
    if (c->sockFD && rvalue == -1) close(c->sockFD);
    return rvalue;
}

/*
 * This will just check if some `free`able
 * objects in the config are still in use.
 *
 * If true, then it will free them.
 */
extern inline void freeConf(server_config_t *restrict c);

extern inline void listenConnect(server_config_t *restrict c);

/*
 * This is the main listen loop, this is not very
 * special, so we can put it inline, since it's not
 * a point we can abort in.
 *
 * This function will retry if it can't connect.
 */
extern inline void listenConnect(server_config_t *restrict c);

/*
 * This writes a void pointer of size bufSize,
 * and frees the buf after use.
 *
 * This also expects buf has the first element of
 * all the packet creator functions, which are the
 * size of the whole packet.
 */
extern inline void writeConsumeU16(
    const int            connFD,
    uint16_t  * restrict buf,
    FILE      * restrict logOut
);

/*
 * This will accept a void pointer, though, under
 * the assumption that buf has no other data other
 * than the packet data itself, thus why bufSize is
 * a separate parameter.
 */
extern inline void writeConsumeVoid(
    const int             connFD,
    const int             bufSize,
          void * restrict buf,
          FILE * restrict logOut
);

#define MAX 128
void readPacket(
    const int                 connFD,
          ssize_t   *restrict readB,
    uint8_t         *restrict byteBuf,
    FILE            *restrict logOut,
    server_config_t *restrict c
) {
    memset(byteBuf, 0, MAX);
    *readB = read(connFD, byteBuf, 1);
    if (*readB == -1) {
        logWithTime(
            "(PACKET READER) Reading packet ID has failed, aborting...",
            false,
            logOut
        );
        freeConf(c);
        _exit(1);
    } *readB += read(connFD, byteBuf + 1, pcktSzTable[*byteBuf]);
    if (*readB == -1) {
        logWithTime(
            "(PACKET READER) Reading payload has failed, aborting...",
            false,
            logOut
        );
        freeConf(c);
        _exit(1);
    } if (
        *byteBuf == 0x02
    ) {
        uint16_t u16_len;
        memcpy(&u16_len, byteBuf + 1, 2);
        *readB += read(connFD, byteBuf + 1 + pcktSzTable[*byteBuf], u16_len);
        if (*readB == -1) {
            logWithTime(
                "(PACKET READER) Reading string failed, aboritng...",
                false,
                logOut
            );
            freeConf(c);
            _exit(1);
        }
    }
}
