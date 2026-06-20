/* Local */
#include <internals/server/packets.h>
#include <internals/debug/logger.h>
#include <internals/types.h>
/* Standard Library */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
/* Other Libraries */
#include <string.h>
#include <memory.h>
#include <zlib.h>

#ifdef __cplusplus
#   define restrict __restrict
#else
#   define restrict restrict
#endif

extern inline void shortReverser(uint16_t *restrict s, const size_t sSize);

extern inline uint32_t intReverser(const uint32_t s);

extern inline uint64_t longReverser(const uint64_t s);

char* utf16_sConv(const uint16_t *restrict packet) {
    const int cLen = packet[0];
    char *m = calloc(1, cLen + 1);
    for (int i = 0; i < cLen; i++)
        m[i] = packet[i + 1] & 0xff;
    return m;
}

/*
 * This is a special function main will not use.
 *
 * This doesn't return the payload size, as it is
 * a constant: (1 + m[0]) * 2
 *
 * m[0] is the size of the original msg.
 */
static uint16_t* string_u16Conv(const char *restrict msg) {
    const int sLen = strlen(msg);
    uint16_t* m = calloc(2, 1 + sLen);
    m[0] = sLen;
    for (int i = 0; i < sLen; i++)
        m[i + 1] = msg[i] & 0xff;
    shortReverser(m, 1 + sLen);
    return m;
}

uint16_t* utf16_packetConv(const char *restrict msg, const int packetID) {
    const int msgLen  = strlen(msg);
    uint16_t *packet  = calloc(1, (2.5 + msgLen) * 2);
    uint8_t  *pCast   = (uint8_t*) (packet + 1);
    uint16_t *payload = string_u16Conv(msg);
    packet[0] = (1.5 + msgLen) * 2;
    pCast[0] = packetID & 0xff;
    memcpy(pCast + 1, payload, (1 + msgLen) * 2);
    free(payload);
    return packet;
}

#define SEPARATOR 0xa7
char* fetchServerStatus(
    const char *restrict serverName,
    const int            onlinePlayers,
    const int            maxPlayers
) {
    const int totalSize =
        strlen(serverName)
        + onlinePlayers / 10.0
        + maxPlayers    / 10.0
        + 5;
    char* s = malloc(totalSize);
    snprintf(
        s, totalSize,
        "%s%c%d%c%d",
        serverName, SEPARATOR,
        onlinePlayers, SEPARATOR,
        maxPlayers
    );
    return s;
}

uint16_t* spawnPPacket(uint32_t *restrict coords) {
    uint32_t translatedCoords[3];
    uint16_t *packet = malloc(3 + (sizeof(int) * 3));
    uint8_t  *pCast  = (uint8_t*) (packet + 1);
    packet[0] = (1 + (sizeof(int) * 3));
    pCast[0] = pid_spawnPos;
    for (int i = 0; i < 3; i++)
        translatedCoords[i] = intReverser(coords[i]);
    memcpy(pCast + 1, translatedCoords, sizeof(int) * 3);
    return packet;
}

#define xIndex 0
#define yIndex 1
#define zIndex 2
inline static void fillBlock(
    const int *restrict coords1,
    const int *restrict coords2,
    const char          blockID,
    uint8_t   *restrict chunk
) {
    int x = coords1[xIndex],
        y = coords1[yIndex],
        z = coords1[zIndex];
    for (; x < coords2[xIndex]; x++) {
    for (; z < coords2[zIndex]; z++) {
    for (; y < coords2[yIndex]; y++) {
        chunk[(x * 2048) + (z * 128) + y] = blockID & 0xff;
    }}}
}

#define bidIndex  6
#define cSize 16 * 16 * 128
#define SIGNATURE "MyCraft"
uint8_t* fetchChunk(FILE *restrict fChunk, FILE *restrict logOut) {
    if (!fChunk) {
        logWithTime(
            "(CHUNK FETCHER) Chunk file does not exist.",
            false,
            logOut
        );
        return NULL;
    }
    char buffer[8];
    uint8_t *pChunk;
    int rBytes = fread(buffer, 7, 1, fChunk);
    buffer[7] = '\0';
    if (
        rBytes == 0
        || strcmp(buffer, SIGNATURE) != 0
    ) {
        logWithTime(
            "(CHUNK FETCHER) Signature is invalid.",
            false,
            logOut
        );
        return NULL;
    }
    pChunk = calloc((int) (cSize * 2.5), 1);
    int coords1[3], coords2[3];
    while (
        (rBytes = fread(buffer, 7, 1, fChunk)) == 7
    ) {
        for (int i = 0; i < 3; i++ ) {
            coords1[i] = buffer[i];
            coords2[i] = buffer[i + 3];
        }
        fillBlock(coords1, coords2, buffer[bidIndex], pChunk);
    }
    /* Skipping metadata since, it is all 0. */
    /* Set all light data to Max (15) */
    memset(pChunk + (int) (cSize * 1.5), 0xff, cSize);
    return pChunk;
}

#define CHUNK 16 * 1024
uint8_t* chunkComp(uint8_t *restrict chunk, FILE* logOut) {
    if (chunk == NULL) {
        logWithTime(
            "Chunk is invalid.",
            false,
            logOut
        );
        return NULL;
    }
    const int trueCSize = cSize * 2.5;
    int retryc = -1,
        rval,
        rembytes,
        flush,
        inOffset   = 0,
        outOffset  = 0;
    uLongf maxSize = compressBound(trueCSize);
    unsigned char outBuf[CHUNK];
    uint32_t *buffer  = calloc(1, maxSize);
    uint8_t  *compBuf = (uint8_t*) buffer + 4;
    z_stream strm;

Retry:
    retryc++;
    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;
    rval = deflateInit(&strm, 2);
    if (rval != Z_OK) {
        logWithTime("Zip compression failed, aborting connection...",
            false, logOut
        );
    }

InitInLoop:
    rembytes = CHUNK < (trueCSize - inOffset) ? CHUNK : (trueCSize - inOffset);
    if (rembytes == 0)
        goto Success;
    strm.next_in  = chunk + inOffset;
    strm.avail_in = rembytes;
    flush = rembytes == CHUNK && rembytes - CHUNK != 0 ? Z_NO_FLUSH : Z_FINISH;

InitOutLoop:
    strm.avail_out = CHUNK;
    strm.next_out  = outBuf;
    rval = deflate(&strm, flush);
    if (rval == Z_STREAM_ERROR) {
        if (retryc >= 3) {
            logWithTime(
                "Zip failed to compress, aborting connection...",
                false, logOut
            );
            goto Err;
        }
        logWithTime(
            "Zip failed to compress, retrying...",
            false, logOut
        );
        goto Retry;
    }
    memcpy(compBuf + outOffset, outBuf, (CHUNK - strm.avail_out));
    outOffset += (CHUNK - strm.avail_out);
    fflush(stdout);
    fflush(logOut);
    if (strm.avail_out == 0) goto InitOutLoop;
    inOffset += rembytes;
    if (flush != Z_FINISH)   goto InitInLoop;

Success:
    (void) deflateEnd(&strm);
    buffer[0] = strm.total_out;
    return (uint8_t*) buffer;

Err:
    (void) deflateEnd(&strm);
    fflush(logOut);
    fflush(stdout);
    return NULL;
}

uint8_t* chunkPckt(
    uint8_t  *restrict compChunk,
    uint32_t *restrict coords
) {
    if (!compChunk) return NULL;
    uint32_t CCSize = *((uint32_t*) compChunk);
    uint8_t   *packet = calloc(1, 18 + CCSize);
    const uint8_t sizePayload[3] = {127, 15, 127};

    packet[0] = pid_mapchunk;
    {
        uint16_t ycoord = (uint16_t) coords[1];
        shortReverser(&ycoord, 1);
        memcpy(packet + 5, &ycoord, 2);
    }
    coords[0] = intReverser(coords[0]);
    coords[2] = intReverser(coords[2]);
    memcpy(packet + 1, coords, 4);
    memcpy(packet + 7, coords + 2, 4);
    memcpy(packet + 11, sizePayload, 3);
    memcpy(packet + 14, compChunk, 4 + CCSize);

    CCSize = intReverser(CCSize);
    memcpy(packet + 14, &CCSize, 4);

    return packet;
}
