/* Local */
#include "internals/server/play.h"
#include "internals/server/packets.h"
#include "internals/server/sockets.h"
#include "internals/debug/logger.h"
#include "internals/types.h"
/* Standard Library */
#include <stdlib.h>
#include <stdbool.h>
/* Other libraries */
#include <time.h>
#include <memory.h>
#include <math.h>

#define MAX 80
#define posXIdx      0
#define posYIdx      1
#define posStanceIdx 2
#define posZIdx      3
void* playingThread(void * pTh_conf) {
    playThread_conf_t * c = (playThread_conf_t*) pTh_conf;
    (*c->gpcPtr)++;

    ssize_t  readB;
    uint8_t  byteBuf[80],
             *bytePtr;
    uint16_t *shortPtr;
    int32_t keepaliveID = 0;
    char     *kickMSG = "Server Disconnect";

    double playerPos[4]  = {0},
           pPosCheck[2]  = {0};
    float  playerLook[2] = {0};

    bool play_onGround = true;

RereadConnection:
    if (*c->serv_c.program_abort) goto ServDiscon;
    readPacket(c->serv_c.connFD, &readB,  byteBuf, c->serv_c.logFile, &c->serv_c);
    if (*c->serv_c.program_abort) goto ServDiscon;
    //*
    logWithTime(
        "(PLAY THREAD) Packet Stream:",
        false,
        c->serv_c.logFile
    );
    for (int i = 0; i < readB; i++) {
        if (i % 8 == 0 && i != 0) puts("");;
        printf("%02X ", byteBuf[i]);
    }
    puts("\n");
    //*/

RereadPacket:
    switch (byteBuf[0]) {
        case pid_stayAlive:
            logWithTime(
                "(PLAY THREAD) Keep alive packet received.",
                false,
                c->serv_c.logFile
            );
            {
                int clientKeepAlive = *((uint32_t*) (byteBuf + 1));
                printf("Actual keepalive: %d -> Client Sent: %d\n", keepaliveID, clientKeepAlive);
                if (keepaliveID != clientKeepAlive) {
                    logWithTime(
                        "(PLAY THREAD) Keep alive packet recieved did not match, disconnecting.",
                        false,
                        c->serv_c.logFile
                    );
                    kickMSG = "Invalid keep alive packet.";
                    goto ServDiscon;
                }
            }
            keepaliveID = 0;
            printf(
                "(%f, %f, %f) Stance: %f\n",
                playerPos[posXIdx], playerPos[posYIdx],
                playerPos[posZIdx], playerPos[posStanceIdx]
            );
            fflush(stdout);
            break;
        case pid_play:
            play_onGround = byteBuf[1] != 0;
            printf("onGround: 0x%02X\n", byteBuf[1]);
            fflush(stdout);
            break;
        case pid_play_pos:
            logWithTime(
                "(PLAY THREAD) Player Position packet received.",
                false,
                c->serv_c.logFile
            );
            memcpy(playerPos, byteBuf + 1, sizeof(playerPos));
            const double distanceTraveled = 
                sqrt(playerPos[posXIdx] + playerPos[posZIdx])
                - sqrt(pPosCheck[0] + pPosCheck[1]);
            if (distanceTraveled > 100.0f || distanceTraveled < -100.0f) {
                logWithTime(
                    "(PLAY THREAD) Player seems to be speed hacking.",
                    false,
                    c->serv_c.logFile
                );
                kickMSG = "You're hacking.";
                goto ServDiscon;
            }
            if (
                playerPos[posStanceIdx] - playerPos[posYIdx] > 1.65f
                && playerPos[posStanceIdx] - playerPos[posYIdx] < 0.1f
            ) {
                logWithTime(
                    "(PLAY THREAD) Player's hitbox seems to be invalid.",
                    false,
                    c->serv_c.logFile
                );
                kickMSG = "Illegal stance.";
                goto ServDiscon;
            }
            break;
        case pid_play_pos_look:
            logWithTime(
                "(PLAY THREAD) Player position and look received.",
                false,
                c->serv_c.logFile
            );
            { /* Switch */
                const int StanceIdx = 1, YIdx = 0;
                double  * toSwap = (double*) (byteBuf + 1) + 1,
                          tempSwap = toSwap[YIdx];
                toSwap[YIdx] = toSwap[StanceIdx];
                toSwap[StanceIdx] = tempSwap;
            }
            if (*c->serv_c.program_abort) goto ServDiscon;
            {
                const int responseSize = pcktSzTable[pid_play_pos_look] + 1;
                bytePtr = malloc(responseSize);
                memcpy(bytePtr, byteBuf, responseSize);
                writeConsumeVoid(
                    c->serv_c.connFD,
                    pcktSzTable[pid_play_pos_look] + 1,
                    bytePtr,
                    c->serv_c.logFile
                );
                bytePtr = NULL;
            }
            break;
        case pid_kick:
            logWithTime(
                "(PLAY THREAD) Client Disconnected.",
                false,
                c->serv_c.logFile
            );
            goto ClieDiscon;
        default:
            logWithTime(
                "Special packet! :",
                false,
                c->serv_c.logFile
            );
            printf("\n0x%02X\n", byteBuf[0]);
            fprintf(c->serv_c.logFile, "\n0x%02X\n", byteBuf[0]);
    }
    if (*c->serv_c.program_abort) goto ServDiscon;
    if (keepaliveID == 0) {
        srand(time(NULL));
        keepaliveID = intReverser(rand() % UINT32_MAX);
        bytePtr = calloc(1, 5);
        bytePtr[0] = pid_stayAlive;
        memcpy(bytePtr + 1, &keepaliveID, 4);
        writeConsumeVoid(c->serv_c.connFD, 5, bytePtr, c->serv_c.logFile);
        keepaliveID = intReverser(keepaliveID);
        printf("Actual keepalive: %08X\n", keepaliveID);
        bytePtr = NULL;
    }
    goto RereadConnection;

ServDiscon:
    shortPtr = utf16_packetConv(kickMSG, pid_kick);
    writeConsumeU16(c->serv_c.connFD, shortPtr, c->serv_c.logFile);

ClieDiscon:
    return NULL;
}
