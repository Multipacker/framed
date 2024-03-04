#ifndef FRAMED_MAIN_H
#define FRAMED_MAIN_H

#define FRAMED_SETTINGS_VERSION (1)

typedef struct ZoneBlock ZoneBlock;
    struct ZoneBlock
{
        Str8 name;
        U64 start_tsc;
        U64 end_tsc;
    };

typedef struct ZoneStackEntry ZoneStackEntry;
struct ZoneStackEntry
{
    ZoneBlock *zone_block;
};

typedef struct Frame Frame;
struct Frame
{
    Arena *arena;
    ZoneBlock *zone_blocks;
    U64 zone_blocks_count;
    U64 begin_tsc;
    U64 end_tsc;
};

    typedef struct ProfilingState ProfilingState;
    struct ProfilingState
{
        // NOTE(hampus): Finished sample

    Frame finished_frame;

    // NOTE(hampus): Current stats in progress

    Frame current_frame;
    ZoneStackEntry zone_stack[1024];
    U64 zone_stack_pos;

    // NOTE(hampus): Per session

    U64 frame_index;
    U64 profile_begin_tsc;
    U64 profile_end_tsc;
    U64 tsc_frequency;
    F64 stats_seconds_accumulator;
    U64 parsed_average_rate;
    U64 bandwidth_average_rate;
    U64 parsed_bytes;
    F64 parsed_time_accumulator;
    U64 bytes_from_client;
    F64 time_since_last_recieve;

    // NOTE(hampus): Networking

    B32 found_connection;
    Net_Socket listen_socket;
    Net_Socket client_socket;
};

#define FRAMED_POP_MESSAGE_PROC(name) Void name(Str8 message)
typedef FRAMED_POP_MESSAGE_PROC(Framed_PopUpMessageProc);

FRAMED_POP_MESSAGE_PROC(framed_pop_up_message);
FRAMED_POP_MESSAGE_PROC(framed_pop_up_message_stub);

typedef struct Framed_State Framed_State;
struct Framed_State
{
    Arena *perm_arena;
    ProfilingState *profiling_state;
    Str8 data_folder_path;
    Str8 default_user_settings_file_path;
    Str8 current_user_settings_file_path;

    Str8 popup_string;
    Framed_PopUpMessageProc *popup_message;
};

////////////////////////////////
//~ hampus: Zone parsing

internal ZoneBlock *framed_get_zone_block(Arena *arena, Str8 name);
internal Void framed_parse_zones(Void);

////////////////////////////////
//~ hampus: Path helpers

internal Str8 framed_get_data_folder_path(Void);
internal Str8 framed_get_default_user_settings_file_path(Void);

////////////////////////////////
//~ hampus: User settings parsing

internal Str8 framed_get_next_settings_word(Str8 string, U64 *bytes_parsed);
internal Void framed_load_user_settings_from_memory(Str8 data_string);
internal Void framed_save_current_settings_to_file(Str8 path);

////////////////////////////////
//~ hampus: Main

internal S32 os_main(Str8List arguments);

#endif //FRAMED_MAIN_H
