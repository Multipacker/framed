#ifndef FRAMED_MAIN_H
#define FRAMED_MAIN_H

#define FRAMED_SETTINGS_VERSION (1)

typedef struct ZoneBlock ZoneBlock;
struct ZoneBlock
{
    Str8 name;
    U64 tsc_elapsed_inc;
    U64 tsc_elapsed_exc;
    U64 hit_count;
};

typedef struct ZoneStack ZoneStack;
struct ZoneStack
{
    Str8 name;
    U64 tsc_start;
    U64 old_tsc_elapsed_root;
    U64 tsc_elapsed_children;
};

#define MAX_NUMBER_OF_UNIQUE_ZONES (4096)
typedef struct CapturedFrame CapturedFrame;
struct CapturedFrame
{
    U64 total_tsc;
    ZoneBlock zone_blocks[MAX_NUMBER_OF_UNIQUE_ZONES];
    U64 start_frame_index;
    U64 end_frame_index;
};

typedef struct ProfilingState ProfilingState;
struct ProfilingState
{
    Arena *zone_arena;

    CapturedFrame latest_captured_frame;

    U64 next_sample_size;
    U64 sample_size;
    U64 frame_index;
    U64 frame_index_accumulator;

    U64 profile_begin_tsc;
    U64 profile_end_tsc;

    // NOTE(hampus): Per index capture frequency

    ZoneBlock zone_blocks[MAX_NUMBER_OF_UNIQUE_ZONES];
    U64 frame_tsc;

    // NOTE(hampus): Per frame

    ZoneStack zone_stack[1024];
    U32 zone_stack_size;
    U64 frame_begin_tsc;
    U64 frame_end_tsc;

    // NOTE(hampus): Init stats

    U64 tsc_frequency;

    // NOTE(hampus): Networking

    B32 found_connection;
    Net_Socket listen_socket;
    Net_Socket client_socket;

    // NOTE(hampus): Zone gathering stats

    // NOTE(hampus): These are per time unit, which is set at the
    // end of the frame where these values are calculated
    F64 stats_seconds_accumulator;

    U64 parsed_average_rate;
    U64 bandwidth_average_rate;

    U64 parsed_bytes;
    F64 parsed_time_accumulator;

    U64 bytes_from_client;

    F64 time_since_last_recieve;
};

typedef struct Framed_State Framed_State;
struct Framed_State
{
    Arena *perm_arena;
    ProfilingState *profiling_state;
    Str8 data_folder_path;
    Str8 default_user_settings_file_path;
    Str8 current_user_settings_file_path;
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
