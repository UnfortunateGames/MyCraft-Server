/*
 * This handles the storage of data in the server.
 *
 * Such as player data, chunk data, etc. This is
 * mainly to make the data manipulation not so bloated.
 */
#pragma once

/* Stanadrd Library */
#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
#   define restrict __restrict
#else
#   define restrict restrict
#endif

/*
 * Ensures that the server has storage for storing
 * the data, and the files themselves.
 *
 * This returns an int of the following:
 * -1 = Could not create storage.
 *  0 = Success
 *  1 - Existing data.
 */
int initWorldStorage(const char *restrict pathRoot, FILE *restrict logOut);

/*
 * This creates another chunk. [FUTURE]
 */
// int createChunk(const int *restrict coords);

/*
 * This expects that the `coords` parameter is
 * the coordinate array like `XYZ`.
 *
 * This should return a file. Though if no file
 * is found, it should return NULL.
 *
 * NOTE: THIS WILL ONLY RETURN spawn.dat IN
 *       THE CHUNK FOLDER!
 */
char* findChunkF(
    const char *restrict pathRoot,
    /* const int *restrict coords, */
    FILE *restrict logOut
);

/*
 * This returns the coordinates the player will
 * spawn in.
 *
 * If the player isn't found, it will spawn in
 * the default spawn, which is 0 60 0.
 */
uint32_t* findPlayer(
    const char *restrict pathRoot,
    const char *restrict playerName,
    FILE *restrict logOut
);
