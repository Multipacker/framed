#define MEMORY_DEBUG 1

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "log/log_inc.h"
#include "debug/debug_inc.h"
#include "meta/embed_inc.h"
#include "gfx/gfx_inc.h"
#include "render/render_inc.h"
#include "image/image_inc.h"
#include "ui/ui_inc.h"
#include "net/net_inc.h"

#define FRAMED_IMPLEMENTATION
#include "public/framed.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "log/log_inc.c"
#include "debug/debug_inc.c"
#include "meta/embed_inc.c"
#include "gfx/gfx_inc.c"
#include "render/render_inc.c"
#include "image/image_inc.c"
#include "ui/ui_inc.c"
#include "net/net_inc.c"

#include "framed/framed_ui.h"
#include "framed/framed_main.h"
#include "framed/zone_node.h"

global U64 framed_frame_counter;
Framed_State *framed_state;

#include "framed/framed_ui.c"
#include "framed/framed_views.c"
#include "framed/zone_node.c"

FRAMED_POP_MESSAGE_PROC(framed_pop_up_message_stub)
{
}

FRAMED_POP_MESSAGE_PROC(framed_pop_up_message)
{
    ui_parent(ui_ctx->normal_root)
    {
        ui_next_width(ui_fill());
        ui_next_height(ui_fill());
        ui_next_color(v4f32(0, 0, 0, 0.5f));
        UI_Box *parent_box = ui_box_make(UI_BoxFlag_DrawBackground |
                                         UI_BoxFlag_FixedPos |
                                         UI_BoxFlag_Clickable,
                                         str8_lit("ParentPopup"));
        ui_parent(parent_box)
        {
            ui_spacer(ui_fill());
            ui_next_width(ui_pct(1, 1));
            ui_row()
            {
                ui_spacer(ui_fill());

                ui_next_width(ui_em(20, 1));
                ui_next_height(ui_em(10, 1));
                ui_next_border_color(framed_ui_color_from_theme(FramedUI_Color_PanelBorderActive));
                UI_Box *box = ui_box_make(UI_BoxFlag_DrawBackground |
                                          UI_BoxFlag_DrawBorder |
                                          UI_BoxFlag_DrawDropShadow |
                                          UI_BoxFlag_AnimateDim,
                                          str8_lit("PopupMessageBackground"));
                ui_parent(box)
                {
                    ui_spacer(ui_fill());
                    ui_next_width(ui_fill());
                    ui_row()
                    {
                        ui_spacer(ui_fill());
                        ui_text(message);
                        ui_spacer(ui_fill());
                    }
                    ui_spacer(ui_fill());
                    ui_next_width(ui_fill());
                    ui_row()
                    {
                        ui_spacer(ui_fill());
                        if (ui_button(str8_lit("OK")).pressed)
                        {
                            framed_state->popup_message = framed_pop_up_message_stub;
                            framed_state->popup_string = str8_lit("");
                        }
                        ui_spacer(ui_em(1, 1));
                        ui_button(str8_lit("Show log"));
                        ui_spacer(ui_fill());
                    }
                    ui_spacer(ui_fill());
                }
                ui_spacer(ui_fill());
            }
            ui_spacer(ui_fill());
        }

        ui_comm_from_box(parent_box);
    }
}

internal Void
framed_show_popup(Str8 message, Framed_PopUpMessageProc *proc)
{
}

////////////////////////////////
//~ hampus: Zone parsing

internal Void
framed_parse_zones(Void)
{
    ProfilingState *profiling_state = framed_state->profiling_state;

    Debug_Time time = debug_function_begin();

    //- hampus: Check connection state

    if (!net_socket_connection_is_alive(profiling_state->client_socket))
    {
        Net_AcceptResult accept_result = {0};
        accept_result = net_socket_accept(profiling_state->listen_socket);
        profiling_state->found_connection = accept_result.succeeded;
        if (profiling_state->found_connection)
        {
            profiling_state->client_socket = accept_result.socket;
            log_info("Connected to client");
            net_socket_set_blocking_mode(accept_result.socket, false);
        }
    }

    U64 parse_start_time_ns = os_now_nanoseconds();

    //- hampus: Gather & process data from client

    B32 terminate_connection = false;
    U16 buffer_size = 0;
    Net_RecieveResult size_result = net_socket_peek(profiling_state->client_socket, (U8 *)&buffer_size, sizeof(buffer_size));
    while (size_result.bytes_recieved == sizeof(buffer_size) && !terminate_connection)
    {
        Arena_Temporary scratch = get_scratch(0, 0);
        U8 *buffer = push_array(scratch.arena, U8, buffer_size);
        Net_RecieveResult recieve_result = net_socket_receive(profiling_state->client_socket, buffer, buffer_size);

        if (recieve_result.bytes_recieved != buffer_size)
        {
            // TODO(simon): What do we do in this case? At the moment
            // the rest of the code assumes that this never happens.
        }

        profiling_state->bytes_from_client += buffer_size;
        profiling_state->parsed_bytes += buffer_size;

        profiling_state->time_since_last_recieve = 0;

        // NOTE(hampus): First two bytes are the size of the packet
        U8 *buffer_pointer = buffer + sizeof(U16);
        U8 *buffer_opl = buffer + recieve_result.bytes_recieved;
        while (buffer_pointer < buffer_opl && !terminate_connection)
        {
            if ((U64) (buffer_opl - buffer_pointer) < sizeof(PacketHeader))
            {
                log_error("Not enough data for packet header, terminating connection");
                terminate_connection = true;
                framed_state->popup_string = str8_lit("Parsing failed! Terminating connection");
                break;
            }

            U64 entry_size = 0;
            PacketHeader *header = (PacketHeader *) buffer_pointer;
            switch (header->kind)
            {
                case Framed_PacketKind_Init:
                {
#pragma pack(push, 1)
                    typedef struct Packet Packet;
                    struct Packet
                    {
                        PacketHeader header;
                        Framed_U64 tsc_frequency;
                        Framed_U16 version;
                    };
#pragma pack(pop)

                    Packet *packet = (Packet *) header;

                    if ((U64) (buffer_opl - buffer_pointer) < sizeof(Packet))
                    {
                        log_error("Not enough data for zone packet, terminating connection");
                        terminate_connection = true;
                        framed_state->popup_message = framed_pop_up_message;
                        framed_state->popup_string = str8_lit("Parsing failed! Terminating connection");
                    }
                    else if ((U64) (buffer_opl - buffer_pointer) < sizeof(Packet))
                    {
                        log_error("Not enough data for zone name, terminating connection");
                        terminate_connection = true;
                        framed_state->popup_message = framed_pop_up_message;
                        framed_state->popup_string = str8_lit("Parsing failed! Terminating connection");
                    }
                    else
                    {
                        profiling_state->profile_begin_tsc = packet->header.tsc;
                        profiling_state->tsc_frequency = packet->tsc_frequency;

                        Frame *frame = &profiling_state->current_frame;
                        arena_pop_to(frame->arena, 0);
                        frame->zone_blocks = push_array(frame->arena, ZoneBlock, 4096*4096*10);
                        frame->tsc_frequency = profiling_state->tsc_frequency;

                        entry_size = sizeof(Packet);
                    }
                } break;
                case Framed_PacketKind_FrameStart:
                {
#pragma pack(push, 1)
                    typedef struct Packet Packet;
                    struct Packet
                    {
                        PacketHeader header;
                    };
#pragma pack(pop)
                    // NOTE(simon): No need to ensure size, the event is only the header.

                    //- hampus: Save the last frame's data and produce a fresh captured frame

                    Packet *packet = (Packet *) header;

                    Frame *frame = &profiling_state->current_frame;

                    frame->end_tsc = header->tsc;

                    swap(profiling_state->current_frame, profiling_state->finished_frame, Frame);
                    frame = &profiling_state->current_frame;
                    arena_pop_to(frame->arena, 0);
                    frame->zone_blocks_count = 0;
                    frame->begin_tsc = 0;
                    frame->end_tsc = 0;
                    frame->zone_blocks = push_array(frame->arena, ZoneBlock, 4096*4096*10);
                    frame->tsc_frequency = profiling_state->tsc_frequency;
                    profiling_state->zone_stack_pos = 0;

                    frame->begin_tsc = header->tsc;

                    profiling_state->frame_index++;

                    entry_size = sizeof(Packet);
                } break;
                case Framed_PacketKind_ZoneBegin:
                {
#pragma pack(push, 1)
                    typedef struct Packet Packet;
                    struct Packet
                    {
                        PacketHeader header;
                        Framed_U8 name_length;
                        Framed_U8 name[];
                    };
#pragma pack(pop)

                    Packet *packet = (Packet *) header;

                    if ((U64) (buffer_opl - buffer_pointer) < sizeof(Packet))
                    {
                        log_error("Not enough data for zone packet, terminating connection");
                        terminate_connection = true;
                        framed_state->popup_message = framed_pop_up_message;
                        framed_state->popup_string = str8_lit("Parsing failed! Terminating connection");
                    }
                    else if ((U64) (buffer_opl - buffer_pointer) < sizeof(Packet) + packet->name_length)
                    {
                        log_error("Not enough data for zone name, terminating connection");
                        terminate_connection = true;
                        framed_state->popup_message = framed_pop_up_message;
                        framed_state->popup_string = str8_lit("Parsing failed! Terminating connection");
                    }
                    else
                    {
                        Str8 name = str8(packet->name, packet->name_length);

                        Frame *frame = &profiling_state->current_frame;

                        ZoneBlock *zone = frame->zone_blocks + frame->zone_blocks_count;

                        zone->name = str8_copy(frame->arena, name);
                        zone->start_tsc = packet->header.tsc;

                        ZoneStackEntry *stack_entry = profiling_state->zone_stack + profiling_state->zone_stack_pos;
                        stack_entry->zone_block = zone;

                        frame->zone_blocks_count++;

                        profiling_state->zone_stack_pos++;

                        assert(profiling_state->zone_stack_pos < array_count(profiling_state->zone_stack));

                        entry_size = sizeof(Packet) + packet->name_length;
                    }
                } break;
                case Framed_PacketKind_ZoneEnd:
                {
#pragma pack(push, 1)
                    typedef struct Packet Packet;
                    struct Packet
                    {
                        PacketHeader header;
                    };
#pragma pack(pop)
                    // NOTE(simon): No need to ensure size, the event is only the header.

                    Packet *packet = (Packet *) header;

                    Frame *frame = &profiling_state->current_frame;

                    profiling_state->zone_stack_pos--;

                    ZoneStackEntry *stack_entry = profiling_state->zone_stack + profiling_state->zone_stack_pos;
                    ZoneBlock *zone = stack_entry->zone_block;

                    zone->end_tsc = header->tsc;

                    entry_size = sizeof(Packet);

                    profiling_state->profile_end_tsc = header->tsc;
                    frame->end_tsc = header->tsc;
                } break;
                default:
                {
                    log_error("Unknown profiling event id (%"PRIS32"), terminating connection", header->kind);
                    terminate_connection = true;
                    framed_state->popup_message = framed_pop_up_message;
                    framed_state->popup_string = str8_lit("Parsing failed! Terminating connection");
                } break;
            }
            buffer_pointer += entry_size;
        }
        release_scratch(scratch);
        size_result = net_socket_peek(profiling_state->client_socket, (U8 *)&buffer_size, sizeof(buffer_size));
    }

    if (terminate_connection)
    {
        net_socket_free(profiling_state->client_socket);
        memory_zero_struct(&profiling_state->client_socket);
    }

    U64 parse_end_time_ns = os_now_nanoseconds();

    profiling_state->parsed_time_accumulator += (F64)(parse_end_time_ns - parse_start_time_ns) / (F64)billion(1);

    debug_function_end(time);
}

////////////////////////////////
//~ hampus: Path helpers

internal Str8
framed_get_data_folder_path(Void)
{
    if (framed_state->data_folder_path.data == 0)
    {
        Arena_Temporary scratch = get_scratch(0, 0);
        Str8List data_folder_path_list = {0};
        str8_list_push(scratch.arena, &data_folder_path_list, os_push_system_path(scratch.arena, OS_SystemPath_UserData));
        str8_list_push(scratch.arena, &data_folder_path_list, str8_lit("/framed/"));
        framed_state->data_folder_path = str8_join(framed_state->perm_arena, &data_folder_path_list);
        release_scratch(scratch);
        os_file_create_directory(framed_state->data_folder_path);
    }
    Str8 result = framed_state->data_folder_path;
    return(result);
}

internal Str8
framed_get_default_user_settings_file_path(Void)
{
    if (framed_state->default_user_settings_file_path.data == 0)
    {
        Arena_Temporary scratch = get_scratch(0, 0);
        Str8List user_settings_file_path = {0};
        str8_list_push(scratch.arena, &user_settings_file_path, framed_get_data_folder_path());
        str8_list_push(scratch.arena, &user_settings_file_path, str8_lit("default.framed_settings"));
        framed_state->default_user_settings_file_path = str8_join(framed_state->perm_arena, &user_settings_file_path);
        release_scratch(scratch);
    }
    Str8 result = framed_state->default_user_settings_file_path;
    return(result);
}

////////////////////////////////
//~ hampus: User settings parsing

internal Str8
framed_get_next_line(Str8 string, U64 *bytes_parsed)
{
    Str8 result = string;
    U8 *string_end = string.data + string.size;
    U8 *at = string.data;
    U8 *comment_start = 0;
    for (;at < string_end;)
    {
        if (at[0] == '\n')
        {
            break;
        }
        if ((at+1) < string_end)
        {
            if (at[0] == '/' && at[1] == '/')
            {
                comment_start = at;
            }
        }
        at++;
    }
    *bytes_parsed = int_from_ptr(at) - int_from_ptr(string.data);
    if (comment_start)
    {
        at = comment_start;
    }
    result.size = int_from_ptr(at) - int_from_ptr(string.data);
    return(result);
}

internal Str8
framed_get_next_setting_name_or_value(Str8 string, U64 *bytes_parsed)
{
    Str8 result = {0};
    U8 *string_end = string.data + string.size;
    U8 *data = string.data;
    while (data < string_end &&
           data[0] != '=' &&
           (data[0] == ' ' ||
            data[0] == '/'))
    {
        if ((data + 1) < string_end && data[0] == '/' && data[1] == '/')
        {
            while (data[0] && data[0] != '\n')
            {
                ++data;
            }
        }
        data++;
    }
    U8 *start = data;

    U8 *end = start;
    while (data < string_end && !(data[0] == '=' || data[0] == '\n'))
    {
        if (data[0] != ' ')
        {
            end = data;
        }
        if ((data + 1) < string_end && data[0] == '/' && data[1] == '/')
        {
            break;
        }
        data++;
    }
    *bytes_parsed = int_from_ptr(data) - int_from_ptr(string.data);
    result = str8_range(start, end+1);
    return(result);
}

internal Str8
framed_get_next_settings_word(Str8 string, U64 *bytes_parsed)
{
    Str8 result = {0};
    U8 *string_end = string.data + string.size;
    U8 *data = string.data;
    while (data < string_end &&
           data[0] != '=' &&
           (data[0] == ' ' ||
            data[0] == '/'))
    {
        if ((data + 1) < string_end && data[0] == '/' && data[1] == '/')
        {
            while (data[0] && data[0] != '\n')
            {
                ++data;
            }
        }
        data++;
    }
    U8 *start = data;
    while (data < string_end && data[0] != '=' && !(data[0] == ' ' || data[0] == '\r' || data[0] == '\n'))
    {
        if ((data + 1) < string_end && data[0] == '/' && data[1] == '/')
        {
            break;
        }
        data++;
    }
    *bytes_parsed = int_from_ptr(data) - int_from_ptr(string.data);
    result = str8_range(start, data);
    return(result);
}

internal Str8List
framed_lines_from_user_settings(Arena *arena, Str8 data)
{
    Str8List result = {0};
    U64 bytes_parsed = 0;
    for (;data.size > 0;)
    {
        Str8 line = framed_get_next_line(data, &bytes_parsed);
        data = str8_skip(data, bytes_parsed+1);
        str8_list_push(arena, &result, line);
    }
    return(result);
}

internal Void
framed_load_user_settings_from_memory(Str8 data_string)
{
    log_info("Loading settings from file...");

    Arena_Temporary scratch = get_scratch(0, 0);
    //- hampus: Get the version number

    U64 line_index = 0;
    U64 bytes_parsed = 0;

    U64 version = 0;
    {
        Str8 line = framed_get_next_line(data_string, &bytes_parsed);
        if (line.size != 0)
        {
            line_index++;
            data_string = str8_skip(data_string, bytes_parsed+1);
            Str8 first_word = framed_get_next_settings_word(line, &bytes_parsed);
            if (str8_equal(first_word, str8_lit("version")))
            {
                line = str8_skip(line, bytes_parsed);
                Str8 version_string = framed_get_next_settings_word(line, &bytes_parsed);
                u64_from_str8(version_string, &version);
            }
            else
            {
                log_error("User settings: missing version number");
            }
        }
    }

    //- hampus: Get the settings

    if (version == 1)
    {
        typedef enum SettingValKind SettingValKind;
        enum SettingValKind
        {
            SettingValKind_U32,
            SettingValKind_Color,
        };

        typedef struct SettingEntry SettingEntry;
        struct SettingEntry
        {
            Str8 name;
            SettingValKind kind;
            Void *dst;
            B32 found;
        };

#define SETTING_THEME_COLOR(i) \
{ \
framed_ui_string_color_table[i], \
SettingValKind_Color, \
&framed_ui_state->settings.theme_colors[i] \
}

        SettingEntry setting_entries_table[] =
        {
            {str8_lit("Font size"), SettingValKind_U32, &framed_ui_state->settings.font_size},

            SETTING_THEME_COLOR(FramedUI_Color_PanelBackground),
            SETTING_THEME_COLOR(FramedUI_Color_PanelBorderActive),
            SETTING_THEME_COLOR(FramedUI_Color_PanelBorderInactive),
            SETTING_THEME_COLOR(FramedUI_Color_PanelOverlayInactive),
            SETTING_THEME_COLOR(FramedUI_Color_TabBarBackground),
            SETTING_THEME_COLOR(FramedUI_Color_TabBackgroundActive),
            SETTING_THEME_COLOR(FramedUI_Color_TabBackgroundInactive),
            SETTING_THEME_COLOR(FramedUI_Color_TabForeground),
            SETTING_THEME_COLOR(FramedUI_Color_TabBorder),
            SETTING_THEME_COLOR(FramedUI_Color_TabBarButtonsBackground),
            SETTING_THEME_COLOR(FramedUI_Color_PanelBorderInactive),
        };

        Str8List lines = framed_lines_from_user_settings(scratch.arena, data_string);

        for (Str8Node *node = lines.first; node != 0; node = node->next)
        {
            Str8 line = node->string;

            if (line.size == 0)
            {
                continue;
            }

            //- hampus: Setting name

            Str8 setting_name = framed_get_next_setting_name_or_value(line, &bytes_parsed);
            line = str8_skip(line, bytes_parsed);
            U64 equal_sign_index = 0;
            if (str8_first_index_of(line, '=', &equal_sign_index))
            {
                //- hampus: Setting value

                // TODO(hampus): Check that the setting is an appropiate value

                line = str8_skip(line, equal_sign_index+1);
                Str8 setting_value = framed_get_next_setting_name_or_value(line, &bytes_parsed);
                B32 valid_value = false;

                for (U64 i = 0; i < array_count(setting_entries_table); ++i)
                {
                    SettingEntry *entry = setting_entries_table + i;
                    if (str8_equal(setting_name, entry->name))
                    {
                        switch (entry->kind)
                        {
                            case SettingValKind_U32:
                            {
                                U32 *dst = entry->dst;
                                u32_from_str8(setting_value, dst);
                                valid_value = true;
                            } break;

                            case SettingValKind_Color:
                            {
                                Vec4F32 *dst = entry->dst;
                                U32 color = 0;
                                u32hex_from_str8(setting_value, &color);
                                *dst = rgba_from_u32(color);
                                valid_value = true;
                            } break;

                            invalid_case;
                        }
                        entry->found = true;
                        break;
                    }
                }

                if (!valid_value)
                {
                    log_error("User settings line %"PRIU64": bad value (%"PRISTR8") for '%"PRISTR8"'", line_index, str8_expand(setting_value), str8_expand(setting_name));
                }
                line = str8_skip(line, bytes_parsed);
            }
            else
            {
                log_error("User settings line %"PRIU64": missing '=' for '%"PRISTR8"'", line_index, str8_expand(setting_name));
            }

            ++line_index;
        }

        for (U64 i = 0; i < array_count(setting_entries_table); ++i)
        {

            SettingEntry *entry = setting_entries_table + i;
            if (!entry->found)
            {
                // TODO(hampus): Write the default value to disk.
            }
        }
    }
    else
    {
        log_error("User settings: bad version: %"PRIU32, version);
    }
    release_scratch(scratch);
}

internal Void
framed_save_current_settings_to_file(Str8 path)
{
    // TODO(hampus): This overrides all settings. It should only override
    // the ones that have actually changed

    log_info("Saving settings to file...");
    Arena_Temporary scratch = get_scratch(0, 0);
    Str8List settings_file_data = {0};
    str8_list_pushf(scratch.arena, &settings_file_data, "version %"PRIU32"\n\n", FRAMED_SETTINGS_VERSION);
    str8_list_pushf(scratch.arena, &settings_file_data, "// General settings\n\n");
    str8_list_pushf(scratch.arena, &settings_file_data, "Font size = %"PRIU32"\n\n", framed_ui_state->settings.font_size);
    str8_list_pushf(scratch.arena, &settings_file_data, "// Colors\n\n");
    for (U64 i = 0; i < FramedUI_Color_TabBarButtonsBackground; ++i)
    {
        str8_list_pushf(scratch.arena, &settings_file_data, "%"PRISTR8" = ""0x%08x\n", str8_expand(framed_ui_string_color_table[i]), u32_from_rgba(framed_ui_state->settings.theme_colors[i]));
    }
    Str8 data = str8_join(scratch.arena, &settings_file_data);
    os_file_write(path, data, OS_FileMode_Replace);
    release_scratch(scratch);
}

////////////////////////////////
//~ hampus: Main

internal S32
os_main(Str8List arguments)
{
    profile_init(1550);

    debug_init();
    arena_scratch(0, 0)
    {
        Str8 binary_path = os_push_system_path(scratch, OS_SystemPath_Binary);
        Str8 log_file = str8_pushf(scratch, "%"PRISTR8"%cframed_log.txt", str8_expand(binary_path), PATH_SEPARATOR);
        log_init(log_file, megabytes(40));
    }

    Gfx_Context gfx = gfx_init(0, 0, 720, 480, str8_lit("Framed"));
    Render_Context *renderer = render_init(&gfx);
    Arena *frame_arenas[2];
    frame_arenas[0] = arena_create("MainFrame0");
    frame_arenas[1] = arena_create("MainFrame1");

    ////////////////////////////////
    //- hampus: Initialize Framed state

    Arena *framed_perm_arena = arena_create("FramedPerm");
    framed_state = push_struct(framed_perm_arena, Framed_State);
    framed_state->perm_arena = framed_perm_arena;
    framed_state->profiling_state = push_struct(framed_perm_arena, ProfilingState);

    ProfilingState *profiling_state = framed_state->profiling_state;
    profiling_state->finished_frame.arena = arena_create("FinishedZoneFrameArena");
    profiling_state->current_frame.arena = arena_create("CurrentZoneFrameArena");

    framed_state->popup_message = framed_pop_up_message_stub;

    zone_node_init();

    //- hampus: Allocate listen socket

    net_socket_init();
    profiling_state->listen_socket = net_socket_alloc(Net_Protocol_TCP, Net_AddressFamily_INET);
    profiling_state->port = FRAMED_DEFAULT_PORT;
    Net_Address address =
    {
        .ip.u8[0] = 127,
        .ip.u8[1] = 0,
        .ip.u8[2] = 0,
        .ip.u8[3] = 1,
        .port = profiling_state->port,
    };


    net_socket_bind(profiling_state->listen_socket , address);;
    net_socket_set_blocking_mode(profiling_state->listen_socket , false);

    ////////////////////////////////
    //- hampus: Initialize UI

    Arena *framed_ui_perm_arena = arena_create("FramedUIPerm");
    framed_ui_state = push_struct(framed_ui_perm_arena, FramedUI_State);
    framed_ui_state->perm_arena = framed_ui_perm_arena;

    framed_ui_state->settings.font_size = 12;
    framed_ui_set_color(FramedUI_Color_PanelBackground, v4f32(0.15f, 0.15f, 0.15f, 1.0f));
    framed_ui_set_color(FramedUI_Color_PanelBorderActive, v4f32(1.0f, 0.8f, 0.0f, 1.0f));
    framed_ui_set_color(FramedUI_Color_PanelBorderInactive, v4f32(0.9f, 0.9f, 0.9f, 1.0f));
    framed_ui_set_color(FramedUI_Color_PanelOverlayInactive, v4f32(0, 0, 0, 0.3f));
    framed_ui_set_color(FramedUI_Color_TabBarBackground, v4f32(0.15f, 0.15f, 0.15f, 1.0f));
    framed_ui_set_color(FramedUI_Color_TabBackgroundActive, v4f32(0.3f, 0.3f, 0.3f, 1.0f));
    framed_ui_set_color(FramedUI_Color_TabBackgroundInactive, v4f32(0.1f, 0.1f, 0.1f, 1.0f));
    framed_ui_set_color(FramedUI_Color_TabForeground, v4f32(0.9f, 0.9f, 0.9f, 1.0f));
    framed_ui_set_color(FramedUI_Color_TabBorder, v4f32(0.9f, 0.9f, 0.9f, 1.0f));
    framed_ui_set_color(FramedUI_Color_TabBarButtonsBackground, v4f32(0.1f, 0.1f, 0.1f, 1.0f));

    Arena_Temporary scratch = get_scratch(0, 0);
    Str8 settings_path = framed_get_default_user_settings_file_path();
    for (Str8Node *node = arguments.first; node != 0; node = node->next)
    {
        U64 equal_sign_index = 0;
        Str8 first_two_characters = str8_substring(node->string, 0, 2);
        if (str8_equal(first_two_characters, str8_lit("--")))
        {
            node->string = str8_skip(node->string, 2);
            if (str8_first_index_of(node->string, '=', &equal_sign_index))
            {
                Str8 option_name = str8_prefix(node->string, equal_sign_index);
                if (str8_equal(option_name, str8_lit("settings")))
                {
                    // TODO(hampus): Check that it actually is a valid settings file path.
                    settings_path = str8_substring(node->string, equal_sign_index+1, node->string.size);
                }
            }
        }
    }

    framed_state->current_user_settings_file_path = settings_path;

    Str8 user_settings_data = {0};
    if (os_file_read(scratch.arena, settings_path, &user_settings_data))
    {
        framed_load_user_settings_from_memory(user_settings_data);
    }
    else
    {
        framed_save_current_settings_to_file(settings_path);
    }

    release_scratch(scratch);

    UI_Context *ui = ui_init();

    framed_ui_state->tab_view_function_table[FramedUI_TabView_Zones]    = framed_ui_tab_view_zones;
    framed_ui_state->tab_view_function_table[FramedUI_TabView_Settings] = framed_ui_tab_view_settings;
    framed_ui_state->tab_view_function_table[FramedUI_TabView_About]    = framed_ui_tab_view_about;

    framed_ui_state->tab_view_string_table[FramedUI_TabView_Zones]    = str8_lit("Zones");
    framed_ui_state->tab_view_string_table[FramedUI_TabView_Settings] = str8_lit("Settings");
    framed_ui_state->tab_view_string_table[FramedUI_TabView_About]    = str8_lit("About");

    Gfx_Monitor monitor = gfx_monitor_from_window(&gfx);
    Vec2F32 monitor_dim = gfx_dim_from_monitor(monitor);
    FramedUI_Window *master_window = framed_ui_window_make(v2f32(0, 0), monitor_dim);
    framed_ui_window_push_to_front(master_window);

    for (U64 i = 0; i < FramedUI_TabView_COUNT; ++i)
    {
        framed_ui_state->tab_view_table[i] = framed_ui_tab_make(framed_ui_state->tab_view_function_table[i], 0, framed_ui_state->tab_view_string_table[i]);
        framed_ui_panel_insert_tab(master_window->root_panel, framed_ui_state->tab_view_table[i]);
    }

#if BUILD_MODE_DEBUG
    framed_ui_panel_insert_tab(master_window->root_panel, framed_ui_tab_make(0, 0, str8_lit("Test")));
#endif

    framed_ui_panel_set_active_tab(master_window->root_panel, framed_ui_state->tab_view_table[FramedUI_TabView_Zones]);

    framed_ui_state->master_window = master_window;
    framed_ui_state->next_focused_panel = master_window->root_panel;

    gfx_set_window_maximized(&gfx);
    gfx_show_window(&gfx);

    ////////////////////////////////
    //- hampus: Main loop

    U64 start_counter = os_now_nanoseconds();
    F64 dt = 0;
    B32 running = true;
    FramedUI_Window *debug_window = 0;
    while (running)
    {
        profile_mark_frame_start();

        profile_begin_block("Main loop");

        Vec2F32 mouse_pos = gfx_get_mouse_pos(&gfx);
        Arena *current_arena  = frame_arenas[0];
        Arena *previous_arena = frame_arenas[1];

        framed_ui_state->frame_arena = current_arena;

        //- hampus: Gather events

        Gfx_EventList events = gfx_get_events(current_arena, &gfx);
        for (Gfx_Event *event = events.first; event != 0; event = event->next)
        {
            if (event->kind == Gfx_EventKind_Quit)
            {
                running = false;
            }
            else if (event->kind == Gfx_EventKind_KeyPress)
            {
                if (event->key == Gfx_Key_F11)
                {
                    gfx_toggle_fullscreen(&gfx);
                }
                else if (event->key == Gfx_Key_F1)
                {
                    if (debug_window)
                    {
                        framed_ui_window_close(debug_window);
                        debug_window = 0;
                    }
                    else
                    {
                        // TODO(hampus): Make debug window size dependent on window
                        // size. We can't go maximized and then query the window
                        // size, because on windows going maximized also shows the
                        // window which we don't want. So this will be a temporary
                        // solution
                        debug_window = framed_ui_window_make(v2f32(0, 50), v2f32(500, 500));
                        framed_ui_window_push_to_front(debug_window);
                        {
                            FramedUI_Tab *debug_tab = framed_ui_tab_make(framed_ui_tab_view_debug, 0, str8_lit("Debug"));
                            framed_ui_panel_insert_tab(debug_window->root_panel, debug_tab);

                            FramedUI_Tab *log_tab = framed_ui_tab_make(framed_ui_tab_view_logger, 0, str8_lit("Log"));
                            framed_ui_panel_insert_tab(debug_window->root_panel, log_tab);

                            FramedUI_Tab *texture_viewer_tab= framed_ui_tab_make(framed_ui_tab_view_texture_viewer, 0, str8_lit("Texture Viewer"));
                            framed_ui_panel_insert_tab(debug_window->root_panel, texture_viewer_tab);

                            framed_ui_panel_set_active_tab(debug_window->root_panel, debug_tab);
                        }
                    }
                }
            }
        }

        ////////////////////////////////
        //- hampus: Zone parsing

        framed_parse_zones();

        ////////////////////////////////
        //- hampus: UI pass

        render_begin(renderer);

        ui_begin(ui, &events, renderer, dt);
        ui_push_font(str8_lit("data/fonts/NotoSansMono-Medium.ttf"));
        ui_push_font_size(framed_ui_state->settings.font_size);

        framed_state->popup_message(framed_state->popup_string);

        //- hampus: Menu bar

        UI_Key view_dropdown_key = ui_key_from_string(ui_key_null(), str8_lit("ViewDropdownMenu"));

        ui_ctx_menu(view_dropdown_key)
        {
            ui_corner_radius(0)
            {
                for (U64 i = 0; i < array_count(framed_ui_state->tab_view_table); ++i)
                {
                    Str8 string = framed_ui_state->tab_view_string_table[i];
                    B32 active = !framed_ui_tab_is_nil(framed_ui_state->tab_view_table[i]);
                    ui_next_hover_cursor(Gfx_Cursor_Hand);
                    ui_next_extra_box_flags(UI_BoxFlag_ActiveAnimation |
                                            UI_BoxFlag_HotAnimation |
                                            UI_BoxFlag_Clickable);
                    ui_next_corner_radius(ui_top_font_line_height() * 0.1f);
                    ui_next_height(ui_em(1, 1));
                    UI_Box *row_box = ui_named_row_beginf("TabViewDropdownListEntry%"PRIU64, i);

                    ui_next_height(ui_em(1, 0.0f));
                    ui_next_width(ui_em(5, 1));
                    UI_Box *tab_box = ui_box_make(UI_BoxFlag_DrawText, str8_lit(""));

                    ui_next_icon(RENDER_ICON_CHECK);
                    ui_next_width(ui_em(1, 1));
                    ui_next_height(ui_pct(1, 1));
                    UI_Box *check_box = ui_box_make(0, str8_lit(""));

                    ui_spacer(ui_em(0.2f, 1));

                    if (active)
                    {
                        check_box->flags |= UI_BoxFlag_DrawText;
                    }

                    ui_box_equip_display_string(tab_box, string);
                    ui_named_row_end();
                    UI_Comm row_comm = ui_comm_from_box(row_box);
                    if (row_comm.pressed)
                    {
                        if (active)
                        {
                            FramedUI_CommandParams params = {0};
                            params.tab = framed_ui_state->tab_view_table[i];
                            framed_ui_command_push(FramedUI_CommandKind_CloseTab, params);
                        }
                        else
                        {
                            framed_ui_state->tab_view_table[i] = framed_ui_tab_make(framed_ui_state->tab_view_function_table[i], 0, framed_ui_state->tab_view_string_table[i]);
                            framed_ui_panel_insert_tab(master_window->root_panel, framed_ui_state->tab_view_table[i]);
                        }
                    }
                    if (row_comm.hovering)
                    {
                        row_box->flags |= UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder;
                    }
                }
            }
        }

        ui_next_extra_box_flags(UI_BoxFlag_DrawBackground);
        ui_next_width(ui_fill());
        ui_corner_radius(ui_top_font_line_height() * 0.1f)
            ui_row()
        {
            UI_Comm comm = ui_button(str8_lit("View"));
            if (comm.clicked)
            {
                ui_ctx_menu_open(comm.box->key, v2f32(0, 0), view_dropdown_key);
            }
        }

        //- hampus: Update panels

        framed_ui_update(renderer, &events);

        //- hampus: Status bar

        Str8 status_text = str8_lit("Not connected");
        if (net_socket_connection_is_alive(profiling_state->client_socket))
        {
            ui_next_color(v4f32(0, 0.3f, 0, 1));
            status_text = str8_lit("Connected");
        }
        else
        {
            ui_next_color(v4f32(0.8f, 0.3f, 0, 1));
        }
        ui_next_corner_radius(0);
        ui_next_width(ui_pct(1, 1));
        ui_next_height(ui_em(1.2f, 1));
        ui_next_text_align(UI_TextAlign_Left);
        UI_Box *status_bar_box = ui_box_make(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawText, str8_lit(""));
        ui_box_equip_display_string(status_bar_box, status_text);

        ////////////////////////////////
        //- hampus: Frame end

        ui_end();

        render_end(renderer);

        ui_debug_keep_alive((U32) framed_frame_counter);

        arena_pop_to(previous_arena, 0);
        swap(frame_arenas[0], frame_arenas[1], Arena *);

        U64 end_counter = os_now_nanoseconds();
        dt = (F64) (end_counter - start_counter) / (F64) billion(1);

        profiling_state->stats_seconds_accumulator += dt;

        if (profiling_state->stats_seconds_accumulator >= 0.5f)
        {
            profiling_state->bandwidth_average_rate = (U64)((F64) profiling_state->bytes_from_client / 0.5);
            profiling_state->bytes_from_client = 0;

            profiling_state->parsed_average_rate = (U64)((F64)profiling_state->parsed_bytes / (F64)profiling_state->parsed_time_accumulator);
            profiling_state->parsed_bytes = 0;
            profiling_state->parsed_time_accumulator = 0;

            profiling_state->stats_seconds_accumulator = 0;
        }

        start_counter = end_counter;
        framed_frame_counter++;

        profile_end_block();
    }

    return(0);
}
