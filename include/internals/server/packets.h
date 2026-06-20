/*
 * This header is for the packet manipulation.
 *
 * This is to turn many things into packets with IDs
 * to send to the client.
 */
/* Local */
#include <internals/types.h>
/* Standard Library */
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
#   define restrict __restrict
#else
#   define restrict restrict
#endif

/*
 * Reverses the endian of a short (uint16_t*)
 */
inline void shortReverser(uint16_t *restrict s, const size_t sSize) {
    for (size_t i = 0; i < sSize; i++)
        s[i] = (s[i] << 8) + (s[i] >> 8);
}

/*
 * Reverses the endian of an int (uint32_t*)
 */
inline uint32_t intReverser(const uint32_t s) {
    return ((s << 8 * 3) & 0xff000000) |
           ((s << 8)     &   0xff0000) |
           ((s >> 8)     &     0xff00) |
           ((s >> 8 * 3) &       0xff);
}

/*
 * Reverses the endian of a long (uint64_t*)
 */
inline uint64_t longReverser(const uint64_t s) {
    return ((s << 8 * 7) & 0xff00000000000000ULL) |
           ((s << 8 * 5) &   0xff000000000000ULL) |
           ((s << 8 * 3) &     0xff0000000000ULL) |
           ((s << 8)     &       0xff00000000ULL) |
           ((s >> 8)     &         0xff000000ULL) |
           ((s >> 8 * 3) &           0xff0000ULL) |
           ((s >> 8 * 5) &             0xff00ULL) |
           ((s >> 8 * 7) &               0xffULL);
}

/*
 * This assumes the packet is cut of any other
 * packet data, like packet ID, and nothing else.
 */
char* utf16_sConv(const uint16_t *restrict packet);

/*
 * This turns the string into a sendable packet.
 * It will return a value with the structure of:
 * 
 * [Packet Size][Packet ID][Size 2 bytes][UTF-16 String]
 */
uint16_t* utf16_packetConv(const char *restrict string, const int packetID);

/*
 * This will return the server status string as
 * a `char*``, so this may be passed to `utf16_packetConv()`.
 * 
 * NOTE: The returned value is heap allocated to do free it
 *       after use.
 */
char* fetchServerStatus(
    const char *restrict serverName,
    const int            onlinePlayers,
    const int            maxPlayers
);

/*
 * This returns the spawn point packet.
 * This must be sent after the login response packet.
 */
uint16_t* spawnPPacket(uint32_t *restrict coords);

/*
 * This returns the compressed chunk.
 * The first element is an integer, so it can
 * handle a bigger size and errors.
 *
 * NOTE: This is computationally expensive.
 *       Probably try to not call this often.
 *
 * Errors:
 * NULL = File doesn't exist or failed to open.
 *  dereferences:
 *      1 = Invalid signature
 *      2 = Invalid chunk
 */
uint8_t* fetchChunk(FILE *restrict fChunk, FILE *restrict logOut);

/*
 * This function expects the return value of
 * fetch chunk, and returns then zip compressed
 * chunk, so we can pass it over to `chunkPackF()`.
 *
 * This also frees the `fetchChunk()` so no need
 * to free it outside of this function.
 *
 * The returned value is also heap allocated,
 * so free it while you can. This follows the structure:
 *
 * [SIZE][COMPRESSED CHUNK DATA]...
 */
uint8_t* chunkComp(uint8_t *restrict chunk, FILE *restrict logOut);

/*
 * This accepts the compressed chunk of
 * `chunkComp()`, and expects the coords are
 * extracted from main.
 *
 * This assumes that `compChunk` isn't modified.
 * So it can retrieve the size acordingly.
 *
 * This won't return an element of a size, as
 * main already has `compSize`, we can calculate
 * that outside of this; `18 + compSize`.
 */
uint8_t* chunkPckt(
    uint8_t  *restrict compChunk,
    uint32_t *restrict coords
);
