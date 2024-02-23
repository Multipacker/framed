// [ ] Logging paths

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

// NOTE(hampus): A global frame counter that everyone can read from
global U64 framed_frame_counter;

#include "public/framed.h"

#include "framed/framed_ui.h"
#include "framed/framed_ui.c"

typedef struct ZoneBlock ZoneBlock;
struct ZoneBlock
{
    Str8 name;
    U64 tsc_elapsed;
    U64 tsc_elapsed_root;
    U64 tsc_elapsed_children;
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
};

typedef struct ProfilingState ProfilingState;
struct ProfilingState
{
    Arena *zone_arena;

    CapturedFrame latest_captured_frame;

    ZoneBlock zone_blocks[MAX_NUMBER_OF_UNIQUE_ZONES];
    ZoneStack zone_stack[1024];
    U32 zone_stack_size;
    U64 frame_begin_tsc;
    U64 frame_end_tsc;
    U64 tsc_frequency;
    U64 frame_index;

    B32 found_connection;
    Net_Socket listen_socket;
    Net_Socket client_socket;

    F64 seconds_accumulator;

    // NOTE(hampus): These are per time unit, which is set at the
    // end of the frame where these values are calculated
    U64 parsed_average_rate;
    U64 bandwidth_average_rate;

    U64 parsed_bytes;
    F64 parsed_time_accumulator;

    U64 bytes_from_client;
};

typedef struct Framed_State Framed_State;
struct Framed_State
{
    Arena *perm_arena;
    ProfilingState *profiling_state;
};

Framed_State *framed_state;

#include "framed/framed_views.c"

internal ZoneBlock *
framed_get_zone_block(Arena *arena, Str8 name)
{
    ProfilingState *profiling_state = framed_state->profiling_state;

    U64 hash = hash_str8(name);
    U64 slot_index = hash % array_count(profiling_state->zone_blocks);
    while (profiling_state->zone_blocks[slot_index].name.data)
    {
        if (str8_equal(profiling_state->zone_blocks[slot_index].name, name))
        {
            break;
        }

        slot_index = (slot_index + 1) % array_count(profiling_state->zone_blocks);
    }

    ZoneBlock *result = &profiling_state->zone_blocks[slot_index];

    if (!result->name.data)
    {
        result->name = str8_copy(arena, name);
    }

    return(result);
}

internal Void
framed_parse_zones(Void)
{
    ProfilingState *profiling_state = framed_state->profiling_state;

    Debug_Time time = debug_function_begin();

    //- hampus: Check connection state

    if (!net_socket_connection_is_alive(profiling_state->client_socket))
    {
        Net_AcceptResult accept_result = {0};
        if (profiling_state->found_connection)
        {
            // NOTE(hampus): The client disconnected
            log_info("Disconnected from client");
            profiling_state->found_connection = false;
        }
        accept_result = net_socket_accept(profiling_state->listen_socket);
        profiling_state->found_connection = accept_result.succeeded;
        if (profiling_state->found_connection)
        {
            profiling_state->client_socket = accept_result.socket;
            log_info("Connected to client");
            net_socket_set_blocking_mode(accept_result.socket, false);
            memory_zero_array(profiling_state->zone_blocks);
        }
    }

    U64 parse_start_time_ns = os_now_nanoseconds();

    //- hampus: Gather & process data from client
    if (net_socket_connection_is_alive(profiling_state->client_socket))
    {
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

            // NOTE(hampus): First two bytes are the size of the packet
            U8 *buffer_pointer = buffer + sizeof(U16);
            U8 *buffer_opl = buffer + recieve_result.bytes_recieved;
            while (buffer_pointer < buffer_opl && !terminate_connection)
            {
                if ((U64) (buffer_opl - buffer_pointer) < sizeof(PacketHeader))
                {
                    log_error("Not enough data for packet header, terminating connection");
                    terminate_connection = true;
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
                        }
                        else if ((U64) (buffer_opl - buffer_pointer) < sizeof(Packet))
                        {
                            log_error("Not enough data for zone name, terminating connection");
                            terminate_connection = true;
                        }
                        else
                        {
                            profiling_state->tsc_frequency = packet->tsc_frequency;
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

                        profiling_state->frame_index++;
                        Packet *packet = (Packet *) header;

                        profiling_state->frame_end_tsc = header->tsc;

                        CapturedFrame *frame = &profiling_state->latest_captured_frame;
                        frame->total_tsc = profiling_state->frame_end_tsc - profiling_state->frame_begin_tsc;
                        // TODO(hampus): We only actually have to save the active counters, not the whole 4096.
                        // But this is simple and easy and works for now.
                        memory_copy_array(frame->zone_blocks, profiling_state->zone_blocks);

                        //- hampus: Clear the new zone stats and begin a new frame

                        // NOTE(hampus): Keep the name so the display gets less messy
                        // from counters jumping everywhere
                        for (U64 i = 0; i < array_count(profiling_state->zone_blocks); ++i)
                        {
                            ZoneBlock *zone_block = profiling_state->zone_blocks + i;
                            zone_block->tsc_elapsed = 0;
                            zone_block->tsc_elapsed_root = 0;
                            zone_block->tsc_elapsed_children = 0;
                            zone_block->hit_count = 0;
                        }

                        profiling_state->frame_begin_tsc = header->tsc;

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
                        }
                        else if ((U64) (buffer_opl - buffer_pointer) < sizeof(Packet) + packet->name_length)
                        {
                            log_error("Not enough data for zone name, terminating connection");
                            terminate_connection = true;
                        }
                        else
                        {
                            Str8 name = str8(packet->name, packet->name_length);

                            assert(profiling_state->zone_stack_size < array_count(profiling_state->zone_stack));
                            ZoneStack *opening = &profiling_state->zone_stack[profiling_state->zone_stack_size++];
                            memory_zero_struct(opening);

                            ZoneBlock *zone = framed_get_zone_block(profiling_state->zone_arena, name);

                            opening->name = zone->name;
                            opening->tsc_start = packet->header.tsc;
                            opening->old_tsc_elapsed_root = zone->tsc_elapsed_root;

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

                        ZoneStack *opening = &profiling_state->zone_stack[--profiling_state->zone_stack_size];
                        ZoneBlock *zone = framed_get_zone_block(profiling_state->zone_arena, opening->name);

                        U64 tsc_elapsed = packet->header.tsc - opening->tsc_start;

                        zone->tsc_elapsed += tsc_elapsed;
                        zone->tsc_elapsed_root = opening->old_tsc_elapsed_root + tsc_elapsed;
                        zone->tsc_elapsed_children += opening->tsc_elapsed_children;
                        ++zone->hit_count;
                        profiling_state->zone_stack[profiling_state->zone_stack_size - 1].tsc_elapsed_children += tsc_elapsed;

                        entry_size = sizeof(Packet);
                    } break;
                    default:
                    {
                        log_error("Unknown profiling event id (%"PRIS32"), terminating connection", header->kind);
                        terminate_connection = true;
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
        }
    }

    U64 parse_end_time_ns = os_now_nanoseconds();

    profiling_state->parsed_time_accumulator += (F64)(parse_end_time_ns - parse_start_time_ns) / (F64)billion(1);

    debug_function_end(time);
}

////////////////////////////////
//~ hampus: Main

internal S32
os_main(Str8List arguments)
{
    debug_init();
    arena_scratch(0, 0)
    {
        Str8 binary_path = os_push_system_path(scratch, OS_SystemPath_Binary);
        Str8 log_file = str8_pushf(scratch, "%"PRISTR8"%cframed_log.txt", str8_expand(binary_path), PATH_SEPARATOR);
        log_init(log_file);
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
    profiling_state->zone_arena = arena_create("ZoneArena");
    profiling_state->zone_stack_size = 1;

    //- hampus: Allocate listen socket

    net_socket_init();
    profiling_state->listen_socket = net_socket_alloc(Net_Protocol_TCP, Net_AddressFamily_INET);
    Net_Address address =
    {
        .ip.u8[0] = 127,
        .ip.u8[1] = 0,
        .ip.u8[2] = 0,
        .ip.u8[3] = 1,
        .port = 1234,
    };

    net_socket_bind(profiling_state->listen_socket , address);;
    net_socket_set_blocking_mode(profiling_state->listen_socket , false);

    ////////////////////////////////
    //- hampus: Initialize UI

    Arena *framed_ui_perm_arena = arena_create("FramedUIPerm");
    framed_ui_state = push_struct(framed_ui_perm_arena, FramedUI_State);
    framed_ui_state->perm_arena = framed_ui_perm_arena;


    framed_ui_state->settings.font_size = 12;
    framed_ui_set_color(FramedUI_Color_Panel, v4f32(0.15f, 0.15f, 0.15f, 1.0f));
    framed_ui_set_color(FramedUI_Color_PanelBorderActive, v4f32(1.0f, 0.8f, 0.0f, 1.0f));
    framed_ui_set_color(FramedUI_Color_PanelBorderInactive, v4f32(0.9f, 0.9f, 0.9f, 1.0f));
    framed_ui_set_color(FramedUI_Color_PanelOverlayInactive, v4f32(0, 0, 0, 0.3f));
    framed_ui_set_color(FramedUI_Color_TabBar, v4f32(0.15f, 0.15f, 0.15f, 1.0f));
    framed_ui_set_color(FramedUI_Color_TabActive, v4f32(0.3f, 0.3f, 0.3f, 1.0f));
    framed_ui_set_color(FramedUI_Color_TabInactive, v4f32(0.1f, 0.1f, 0.1f, 1.0f));
    framed_ui_set_color(FramedUI_Color_TabTitle, v4f32(0.9f, 0.9f, 0.9f, 1.0f));
    framed_ui_set_color(FramedUI_Color_TabBorder, v4f32(0.9f, 0.9f, 0.9f, 1.0f));
    framed_ui_set_color(FramedUI_Color_TabBarButtons, v4f32(0.1f, 0.1f, 0.1f, 1.0f));

    UI_Context *ui = ui_init();

    framed_ui_state->tab_view_function_table[FramedUI_TabView_Counter]  = framed_ui_tab_view_counters;
    framed_ui_state->tab_view_function_table[FramedUI_TabView_Settings] = framed_ui_tab_view_settings;
    framed_ui_state->tab_view_function_table[FramedUI_TabView_About]    = framed_ui_tab_view_about;

    framed_ui_state->tab_view_string_table[FramedUI_TabView_Counter]  = str8_lit("Counter");
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

    framed_ui_panel_set_active_tab(master_window->root_panel, framed_ui_state->tab_view_table[FramedUI_TabView_Counter]);

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

        profiling_state->seconds_accumulator += dt;

        if (profiling_state->seconds_accumulator >= 0.5f)
        {
            profiling_state->bandwidth_average_rate = (U64)((F64) profiling_state->bytes_from_client / 0.5);
            profiling_state->bytes_from_client = 0;

            profiling_state->parsed_average_rate = (U64)((F64)profiling_state->parsed_bytes / (F64)profiling_state->parsed_time_accumulator);
            profiling_state->parsed_bytes = 0;
            profiling_state->parsed_time_accumulator = 0;

            profiling_state->seconds_accumulator = 0;
        }

        start_counter = end_counter;
        framed_frame_counter++;
    }

    return(0);
}
