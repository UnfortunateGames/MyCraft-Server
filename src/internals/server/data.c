/* Local */
#include <internals/types.h>
#include <internals/server/data.h>
#include <internals/debug/logger.h>
/* Standard Library */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
/* UNIX Libraries */
#include <sys/stat.h>
#include <sys/types.h>
/* Other Libraries */
#include <string.h>
#include <errno.h>

#ifdef __cplusplus
#   define restrict __restrict
#else
#   define restrict restrict
#endif

static const block_data_t spawnDat[] = {
    {
        .coords1 = {0, 0, 0},
        .coords2 = {15, 0, 15},
        .blockID = 2
    },
    {
        .coords1 = {6, 6, 6},
        .coords2 = {8, 6, 8},
        .blockID = 18
    },
    {
        .coords1 = {5, 5, 5},
        .coords2 = {9, 5, 9},
        .blockID = 18
    },
    {
        .coords1 = {7, 1, 7},
        .coords2 = {7, 7, 7},
        .blockID = 17
    }
};

static int checkFile(
    char       *restrict outBuf,
    const char *restrict compStr,
    FILE       *restrict pFile
) {
    const int compSize = strlen(compStr);
    if (!pFile) return 1;
    if (fread(
        outBuf, compSize, 1, pFile
    ) == 0) return 2;
    if (strncmp(outBuf, compStr, compSize) != 0) return 3;
    return 0;
}

#define SIGNATURE "MyCraft"
int initWorldStorage(const char *restrict pathRoot, FILE *restrict logOut) {
    const int pathLen = strlen(pathRoot),
              pBSize  = pathLen + strlen("/chunk/spawn.bin") + 1;
    const char worldDat[4] = {0, 10, 0, 42};
    char checkBuf[16];
    char pathBuf[pBSize];
    bool hasCreated = false;
    snprintf(
        pathBuf, pBSize,
        "%s/world.bin", pathRoot
    );
    FILE* filePtr = fopen(pathBuf, "rb");
    switch (checkFile(checkBuf, SIGNATURE, filePtr)) {
        case 0:
            break;
        case 1:
            logWithTime(
                "World binary doesn't exist.",
                false,
                logOut
            );
            goto CreateWorldBin;
        case 2:
            logWithTime(
                "World binary is incomplete.",
                false,
                logOut
            );
            goto CreateWorldBin;
        case 3:
            logWithTime(
                "World binary has a wrong signature.",
                false,
                logOut
            );
            goto CreateWorldBin;
    }
    if (fread(checkBuf, 4, 1, filePtr) == 0) {
        logWithTime("\"world.bin\" is incomplete.", false, logOut);
        goto CreateWorldBin;
    }
    logWithTime("Read \"world.bin\" is valid, and will be used.", false, logOut);
    fflush(logOut);
    goto CheckChunkDir;

    /*
     * World binary is currently a placeholder for now.
     */
CreateWorldBin:
    logWithTime("Creating new world binary...", false, logOut);
    hasCreated = true;
    if (filePtr) fclose(filePtr);
    filePtr = fopen(pathBuf, "wb");
    (void) fwrite(SIGNATURE, strlen(SIGNATURE), 1, filePtr);
    (void) fwrite(worldDat,  sizeof(worldDat),     1, filePtr);

CheckChunkDir:
    logWithTime("Checking \"chunk\" directory.", false, logOut);
    if (filePtr) fclose(filePtr);
    filePtr = NULL;
    snprintf(
        pathBuf, pBSize,
        "%s/chunk", pathRoot
    );
    if (mkdir(pathBuf, S_IRWXG | S_IRWXO | S_IRWXU) == -1) {
        if (errno == EEXIST) {
            logWithTime(
                "Director exists! Checking Spawn binary...",
                false,
                logOut
            );
            goto CheckSpawnBin;
        }
        else return -1;
    } logWithTime(
        "Directory did not exist, creating spawn chunk binary.",
        false,
        logOut
    );
    fflush(logOut);
    goto CreateSpawnBin;

CheckSpawnBin:
    logWithTime("Checking spawn binary...", false, logOut);
    filePtr = fopen(pathBuf, "rb");
    switch (checkFile(checkBuf, SIGNATURE, filePtr)) {
        case 0:
            /*
             * NOTE: We're not checking the contents, because
             *       we're trusting the user for this one.
             */
            logWithTime(
                "Spawn binary is a valid file!",
                false,
                logOut
            );
            goto Exit;
        case 1:
            logWithTime(
                "Spawn binary doesn't exist.",
                false,
                logOut
            );
            break;
        case 2:
            logWithTime(
                "Spawn binary is incomplete.",
                false,
                logOut
            );
            break;
        case 3:
            logWithTime(
                "Spawn binary has a wrong signature.",
                false,
                logOut
            );
    }
    fflush(logOut);

CreateSpawnBin:
    logWithTime("Creating spawn binary...", false, logOut);
    if (filePtr) fclose(filePtr);
    snprintf(
        pathBuf, pBSize,
        "%s/chunk/spawn.bin",
        pathRoot
    );
    filePtr = fopen(pathBuf, "wb");
    if (!filePtr) {
        logWithTime(
            "Spawn binary could not be created.",
            false,
            logOut
        );
        goto Exit;
    }
    fwrite(SIGNATURE, strlen(SIGNATURE), 1, filePtr);
    fwrite(spawnDat, sizeof(spawnDat), 1, filePtr);

Exit:
    fflush(logOut);
    if (filePtr) {
        fclose(filePtr);
        filePtr = NULL;
    }
    if (!hasCreated) return 1;
    return 0;
}

char* findChunkF(
    const char *restrict pathRoot,
    /* const int *restrict coords, */
    FILE *restrict logOut
) {
    logWithTime(
        "For this version we will only pass the spawn chunk.",
        false,
        logOut
    );
    const int pBSize = strlen(pathRoot) + strlen("/chunk/spawn.bin") + 1;
    char *pathBuf = malloc(pBSize);
    snprintf(
        pathBuf, pBSize,
        "%s/chunk/spawn.bin", pathRoot
    );
    return pathBuf;
}

uint32_t* findPlayer(
    const char *restrict pathRoot,
    const char *restrict playerName,
    FILE *restrict logOut
) {
    const int pBSize =
        strlen(pathRoot)
        + strlen(playerName)
        + strlen(".bin")
        + 2;
    char pathBuf[pBSize];
    char checkBuf[8];
    char coordBuf[3] = {0};
    uint32_t  *coordOut = calloc(1, 12);
    FILE* filePtr;

    snprintf(
        pathBuf, pBSize,
        "%s/%s.bin", pathRoot, playerName
    );

    filePtr = fopen(pathBuf, "rb");
    switch (checkFile(checkBuf, SIGNATURE, filePtr)) {
        case 0:
            logWithTime(
                "Player file has valid signature...",
                false,
                logOut
            );
            goto ReturnCoords;
        case 1:
            logWithTime(
                "Player file doesn't exist.",
                false,
                logOut
            );
            break;
        case 2:
            logWithTime(
                "Player file is incomplete.",
                false,
                logOut
            );
            break;
        case 3:
            logWithTime(
                "Player file has an invalid signature.",
                false,
                logOut
            );
    }

    /* Default coords */
    coordOut[1] = 10;
    if (filePtr) {
        fclose(filePtr);
        filePtr = NULL;
    }
    return coordOut;

ReturnCoords:
    if (filePtr) fclose(filePtr);
    filePtr = fopen(pathBuf, "rb");
    fseek(filePtr, strlen(SIGNATURE), SEEK_SET);
    /*
     * We're ignoring some cases, sinc we've already
     * tested them with the other checkFile() call.
     */
    switch (checkFile(coordBuf, "NOT", filePtr)) {
        case 3:
            /* [Fallthrough] */
        case 0:
            for (int i = 0; i < 3; i++)
                { coordOut[i] = coordBuf[i]; }
            break;
        default:
            logWithTime(
                "Player coordinate fail.",
                false,
                logOut
            );
            free(coordOut);
            coordOut = NULL;
            break;
    }
    fclose(filePtr);
    return coordOut;
}
