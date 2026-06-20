/*
 * I don't know how to sort these not gonna lie...
 *
 * Should I either sort them by need?
 * Like the first struct here is the first to be used.
 *
 * Or either sort them alphabetically or most used...
 */
#pragma once

/* Standard Library */
#include <stdio.h>
/* Network Libraries */
#include <netinet/in.h>
/* Other libraries */
#include <signal.h>

/*
 * This is supposed to line up with the Packet ID enum.
 */
static const int pcktSzTable[] = {
    4,
    6,
    2,  /*   String!   */
    2,
    8,
    10,
    14,
    9,
    8,
    13,
    1,
    33,
    9,
    41,
    11,
    12,
    2,
    14,
    5,
    5,
    4,
    24,
    8,
    21,
    20,
    4,  /*   String!   */
    18,
    0,  /* Unused. */
    10,
    4,
    7,
    9,
    18,
    5,
    8,
    5,  /* MetaData! */
    8,
    5,
    4,
    [50] = 9,  /* Server only */
    18, /* Server only */
    11, /*  MetaData! */
    11,
    12,
    32, /* Record Stream! */
    17,
    [70] = 2,
    17,
    [100] = 6, /* String! */
    1,
    10, /* Additional 3 bytes! */
    6,  /* Additional 3 bytes! */
    3, /* Server only! */
    5, /* Server only! */
    4,
    8,
    12, /* 3 fucking UTF-16 Strings */
    5, /* String! */
    5,
    2, /* String! */
    [0xfe] = 0,
    2 /* String! */
};

/*
 * This is to simplify the parameters a bit.
 * logFile may or may not exist. Which is fine.
 *
 * This is designed to also be passed to the connector.
 */
typedef struct server_config_t {
    int                     sockFD,
                            connFD;
    struct sockaddr_in      servAddr,
                            cliAddr;
    const char            * pathRoot;
    FILE                  * logFile;
    volatile sig_atomic_t * program_abort;
} server_config_t;

typedef struct playThread_conf_t {
    server_config_t         serv_c;
    int                   * gpcPtr;
} playThread_conf_t;

typedef struct block_data_t {
    char coords1[3],
         coords2[3],
         blockID;
} block_data_t;

#pragma pack(push, 1)

typedef struct login_res_pack_t {
    uint8_t  packetID;
    uint32_t playerEntID;
    uint16_t unusedStr[2];
    uint64_t seed;
    uint32_t servMode;
    uint8_t  dimension,
             difficulty,
             worldHeight,
             maxPlayers;
} login_res_pack_t;

/*
 * The chunk packet.
 *
 * NOTE: `chunkData` expects that it's size is
 *       still in the data.
 */
typedef struct chunk_pack_t {
    uint8_t    packetID;
    uint32_t   coordX;
    uint16_t   coordY;
    uint32_t   coordZ;
    uint8_t    chunkSize[3];
    uint8_t  * chunkData;
} chunk_pack_t;

#pragma pack(pop)

typedef enum packet_id_t {
    pid_stayAlive,
    pid_loginResponse,
    pid_handshake,
    pid_chatMsg,
    pid_timeUpd,
    pid_entEq,
    pid_spawnPos,
    pid_updHP,
    pid_respawn,
    pid_play,
    pid_play_pos,
    pid_play_look,
    pid_play_pos_look,
    pid_play_dig,
    pid_block_place,
    pid_hold_change,
    pid_use_bed,
    pid_ent_action,
    pid_named_ent_spwn,
    pid_spwn_item,
    pid_add_obj,
    pid_mob_spwn,
    pid_exp_orb,
    pid_unused_stance_upd,
    pid_ent_vel,
    pid_destroy_ent,
    pid_ent,
    pid_ent_rel_move,
    pid_ent_look,
    pid_ent_look_rel_move,
    pid_ent_tp,
    pid_ent_stat,
    pid_attach_ent,
    pid_ent_metdat,
    pid_ent_effect,
    pid_rem_ent_effect,
    pid_exp,
    // ..
    pid_prechunk = 0x32,
    pid_mapchunk,
    pid_multiblock_change,
    pid_block_change,
    pid_block_act,
    // ...
    pid_explosion = 0x3c,
    pid_sfx,
    // ..
    pid_new_inval_state = 0x46,
    pid_thunder,
    // ...
    pid_open_window = 0x64,
    pid_close_window,
    pid_window_click,
    pid_set_slot,
    pid_window_items,
    pid_upd_progress_bar,
    pid_transaction,
    pid_creative_inven_act,
    pid_upd_sign = 0x83,
    // ...
    pid_increment_stat = 0xc8,
    pid_player_list_item,
    // ...
    pid_statusPacket = 0xfe,
    pid_kick
} packet_id_t;

typedef enum server_mode_t {
    sm_awaiting,
    sm_login,
    sm_playing
} server_mode_t;
