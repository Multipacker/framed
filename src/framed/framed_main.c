////////////////////////////////
// Stuff to get done before going public
//
// [ ] Buffer up packets on another thread

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "log/log_inc.h"
#include "debug/debug_inc.h"
#include "gfx/gfx_inc.h"
#include "render/render_inc.h"
#include "image/image_inc.h"
#include "ui/ui_inc.h"
#include "net/net_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "log/log_inc.c"
#include "debug/debug_inc.c"
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

typedef struct CapturedFrame CapturedFrame;
struct CapturedFrame
{
    U64 total_tsc;
    ZoneBlock zone_blocks[4096];
};

typedef struct ProfilingState ProfilingState;
struct ProfilingState
{
    CapturedFrame latest_captured_frame;

    ZoneBlock zone_blocks[4096];
    ZoneStack zone_stack[1024];
    U32 zone_stack_size;
    U64 frame_begin_tsc;
    U64 frame_end_tsc;

    Net_Socket listen_socket;
    Net_Socket client_socket;
};

global ProfilingState *profiling_state;

////////////////////////////////
//~ hampus: Tab views

FRAME_UI_TAB_VIEW(framed_ui_tab_view_logger)
{
    ui_next_width(ui_fill());
    ui_next_height(ui_fill());
    B32 view_info_data_initialized = view_info->data != 0;
    LogUI_State *log_ui = framed_ui_get_view_data(view_info, LogUI_State);
    if (!view_info_data_initialized)
    {
        log_ui->perm_arena = arena_create("LogUIPerm");
    }
    ui_logger(log_ui);
}

FRAME_UI_TAB_VIEW(framed_ui_tab_view_texture_viewer)
{
    ui_next_width(ui_fill());
    ui_next_height(ui_fill());
    Render_TextureSlice texture = *(Render_TextureSlice *) view_info->data;
    ui_texture_view(texture);
}

FRAME_UI_TAB_VIEW(framed_ui_tab_view_debug)
{
    ui_next_width(ui_fill());
    ui_next_height(ui_fill());
    B32 view_info_data_initialized = view_info->data != 0;
    UI_ColorPickerData *color_picker_data = framed_ui_get_view_data(view_info, UI_ColorPickerData);
    ui_debug(color_picker_data);
}

FRAME_UI_TAB_VIEW(framed_ui_tab_view_settings)
{
    B32 data_initialized = view_info->data != 0;

    typedef struct TabViewData TabViewData;
    struct TabViewData
    {
        UI_ColorPickerData color_picker_data;

        UI_TextEditState font_size_text_edit_state;
        U8 *font_size_text_buffer;
        U64 font_size_text_buffer_size;
        U64 font_size_string_length;
    };

    TabViewData *data = framed_ui_get_view_data(view_info, TabViewData);

    if (!data_initialized)
    {
        data->font_size_text_buffer_size = 3;
        data->font_size_text_buffer = push_array(view_info->arena, U8, data->font_size_text_buffer_size);
        arena_scratch(0, 0)
        {
            Str8 text_buffer_initial_string = str8_pushf(scratch, "%"PRIU32, framed_ui_state->settings.font_size);
            U64 initial_string_length = u64_min(text_buffer_initial_string.size, data->font_size_text_buffer_size);
            memory_copy_typed(data->font_size_text_buffer, text_buffer_initial_string.data, initial_string_length);
            data->font_size_string_length = initial_string_length;
        }
    }

    //- hampus: General settings

    ui_next_font_size(framed_ui_font_size_from_scale(FramedUI_FontScale_Larger));
    ui_text(str8_lit("General"));
    ui_spacer(ui_em(0.5f, 1));

    ui_row()
    {
        ui_column()
        {
            ui_row()
            {
                ui_text(str8_lit("Font size:"));
                ui_next_width(ui_em(3, 1));
                UI_Comm comm = ui_line_edit(&data->font_size_text_edit_state, data->font_size_text_buffer, data->font_size_text_buffer_size, &data->font_size_string_length, str8_lit("FontSizeLineEdit"));
                if (!ui_box_is_focused(comm.box))
                {
                    U32 u32 = 0;
                    u32_from_str8(str8(data->font_size_text_buffer, data->font_size_string_length), &u32);
                    framed_ui_state->settings.font_size = u32_clamp(1, u32, 30);
                    arena_scratch(0, 0)
                    {
                        Str8 text_buffer_str8 = str8_pushf(scratch, "%"PRIU32, framed_ui_state->settings.font_size);
                        data->font_size_string_length = u64_min(text_buffer_str8.size, data->font_size_text_buffer_size);
                        memory_copy_typed(data->font_size_text_buffer, text_buffer_str8.data, data->font_size_string_length);
                    }
                }
                else
                {
                }
            }
        }
    }

    ui_spacer(ui_em(0.5f, 1));

    //- hampus: Appearance

    ui_next_font_size(framed_ui_font_size_from_scale(FramedUI_FontScale_Larger));
    ui_text(str8_lit("Appearance"));
    ui_spacer(ui_em(0.5f, 1));

    UI_ColorPickerData *color_picker_data = &data->color_picker_data;

    UI_Key theme_color_ctx_menu = ui_key_from_string(ui_key_null(), str8_lit("ThemeColorCtxMenu"));

    ui_ctx_menu(theme_color_ctx_menu)
    {
        ui_color_picker(color_picker_data);
    }

    ui_column()
    {
        for (FramedUI_Color color = (FramedUI_Color) 0; color < FramedUI_Color_COUNT; ++color)
        {
            ui_next_width(ui_em(20, 1));
            ui_row()
            {
                Str8 string = framed_ui_string_from_color(color);
                ui_text(string);
                ui_spacer(ui_fill());
                ui_next_color(framed_ui_color_from_theme(color));
                ui_next_hover_cursor(Gfx_Cursor_Hand);
                ui_next_corner_radius(5);
                ui_next_width(ui_em(1, 1));
                ui_next_height(ui_em(1, 1));
                UI_Box *box = ui_box_make(
                                          UI_BoxFlag_DrawBackground |
                                          UI_BoxFlag_DrawBorder |
                                          UI_BoxFlag_Clickable |
                                          UI_BoxFlag_HotAnimation |
                                          UI_BoxFlag_ActiveAnimation,
                                          string
                                          );
                UI_Comm comm = ui_comm_from_box(box);
                if (comm.clicked)
                {
                    ui_ctx_menu_open(box->key, v2f32(0, 0), theme_color_ctx_menu);
                    color_picker_data->rgba = &framed_ui_state->settings.theme_colors[color];
                }
            }
            ui_spacer(ui_em(0.5f, 1));
        }
        arena_scratch(0, 0)
        {
            ui_row()
            {
                ui_spacer(ui_em(0.3f, 1));
                ui_next_width(ui_text_content(1));
                if (ui_button(str8_lit("Dump theme to file")).clicked)
                {
                    Str8List string_list = {0};
                    for (FramedUI_Color color = (FramedUI_Color) 0; color < FramedUI_Color_COUNT; ++color)
                    {
                        Str8 label = framed_ui_string_from_color(color);
                        Vec4F32 color_value = framed_ui_color_from_theme(color);
                        str8_list_push(scratch, &string_list, label);
                        str8_list_pushf(
                                        scratch, &string_list, ": %.2f, %.2f, %.2f, %.2f\n",
                                        color_value.r,
                                        color_value.g,
                                        color_value.b,
                                        color_value.a
                                        );
                    }
                    Str8 dump_data = str8_join(scratch, &string_list);
                    Str8 theme_dump_file_name = str8_lit("theme_dump.framed");
                    os_file_write(theme_dump_file_name, dump_data, OS_FileMode_Replace);
                }
            }
        }
    }
}

internal ZoneBlock *
framed_get_zone_block(Arena *arena, Str8 name)
{
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

FRAME_UI_TAB_VIEW(framed_ui_tab_view_counters)
{
    Arena_Temporary scratch = get_scratch(0, 0);

    //- hampus: Gather values into a nice format

    typedef struct CounterValues CounterValues;
    struct CounterValues
    {
        Str8 name[4096];
        U64 tsc_elapsed_without_children[4096];
        U64 tsc_elapsed_with_children[4096];
        U64 hit_count[4096];
    };

    U64 counter_values_count = 0;
    CounterValues *counter_values = push_struct(scratch.arena, CounterValues);

    U64 tsc_total = 0;

    ZoneBlock *zone_block_array = 0;
    U64 zone_block_array_count = 0;

    B32 profiling_per_frame = profiling_state->frame_begin_tsc != 0;;

    if (profiling_per_frame)
    {
        // NOTE(hampus): We have got a full frame, take the values from there
        CapturedFrame *frame = &profiling_state->latest_captured_frame;
        zone_block_array = frame->zone_blocks;
        zone_block_array_count = array_count(frame->zone_blocks);
        tsc_total = frame->total_tsc;
    }
    else
    {
        // NOTE(hampus): If we haven't got any frame yet, just take the current values
        zone_block_array = profiling_state->zone_blocks;
        zone_block_array_count = array_count(profiling_state->zone_blocks);
    }

    for (U64 i = 0; i < zone_block_array_count; ++i)
    {
        ZoneBlock *zone_block = zone_block_array + i;
        if (zone_block->name.size)
        {
            U64 tsc_without_children = zone_block->tsc_elapsed - zone_block->tsc_elapsed_children;
            U64 tsc_with_children = zone_block->tsc_elapsed_root;
            counter_values->name[counter_values_count] = zone_block->name;
            counter_values->tsc_elapsed_without_children[counter_values_count] = tsc_without_children;
            counter_values->tsc_elapsed_with_children[counter_values_count] = tsc_with_children;
            counter_values->hit_count[counter_values_count] = zone_block->hit_count;
            counter_values_count++;
        }
    }

    //- hampus: Display values

    F32 name_column_width_pct = 0.25f;
    F32 cycles_column_width_pct = 0.25f;
    F32 cycles_children_column_width_pct = 0.25f;
    F32 hit_count_column_width_pct = 0.25f;

    if (profiling_per_frame)
    {
        ui_spacer(ui_em(0.3f, 1));
        ui_textf("Total cycles for frame: %"PRIU64, tsc_total);
    }

    ui_spacer(ui_em(0.3f, 1));

    ui_next_width(ui_fill());
    ui_next_height(ui_fill());
    ui_row()
    {
        //- hampus: Name

        ui_next_width(ui_pct(name_column_width_pct, 0));
        ui_next_extra_box_flags(UI_BoxFlag_Clip);
        ui_column()
        {
            ui_next_width(ui_pct(1, 1));
            ui_row()
            {
                ui_next_width(ui_fill());
                ui_next_text_align(UI_TextAlign_Left);
                ui_text(str8_lit("Name"));
            }

            ui_spacer(ui_em(0.3f, 1));

            ui_next_width(ui_pct(1, 1));
            ui_next_height(ui_pixels(1, 1));
            ui_next_corner_radius(0);
            ui_next_color(v4f32(0.9f, 0.9f, 0.9f, 1));
            ui_box_make(UI_BoxFlag_DrawBackground, str8_lit(""));

            ui_spacer(ui_em(0.3f, 1));

            for (U64 i = 0; i < counter_values_count; ++i)
            {
                ui_text(counter_values->name[i]);
            }
        }

        ui_next_width(ui_pixels(1, 1));
        ui_next_height(ui_em(60, 1));
        ui_next_corner_radius(0);
        ui_next_color(v4f32(0.9f, 0.9f, 0.9f, 1));
        ui_box_make(UI_BoxFlag_DrawBackground, str8_lit(""));

        //- hampus: Cycles without children

        ui_next_width(ui_pct(cycles_column_width_pct, 0));
        ui_next_extra_box_flags(UI_BoxFlag_Clip);
        ui_column()
        {
            ui_next_width(ui_pct(1, 1));
            ui_row()
            {
                ui_next_width(ui_fill());
                ui_next_text_align(UI_TextAlign_Left);
                ui_text(str8_lit("Cycles"));
            }

            ui_spacer(ui_em(0.3f, 1));

            ui_next_width(ui_pct(1, 1));
            ui_next_height(ui_pixels(1, 1));
            ui_next_corner_radius(0);
            ui_next_color(v4f32(0.9f, 0.9f, 0.9f, 1));
            ui_box_make(UI_BoxFlag_DrawBackground, str8_lit(""));

            ui_spacer(ui_em(0.3f, 1));

            ui_text_align(UI_TextAlign_Right)
                ui_width(ui_pct(1, 1))
            {
                for (U64 i = 0; i < counter_values_count; ++i)
                {
                    Str8 string = {0};
                    if (profiling_per_frame)
                    {
                        string = str8_pushf(ui_frame_arena(), "%"PRIU64" (%5.2f%%)", counter_values->tsc_elapsed_without_children[i],  ((F32)counter_values->tsc_elapsed_without_children[i] / (F32)tsc_total) * 100.0f);
                    }
                    else
                    {
                        string = str8_pushf(ui_frame_arena(), "%"PRIU64, counter_values->tsc_elapsed_without_children[i]);
                    }
                    ui_text(string);
                }
            }
        }

        ui_next_width(ui_pixels(1, 1));
        ui_next_height(ui_em(60, 1));
        ui_next_corner_radius(0);
        ui_next_color(v4f32(0.9f, 0.9f, 0.9f, 1));
        ui_box_make(UI_BoxFlag_DrawBackground, str8_lit(""));

        //- hampus: Cycles with children

        ui_next_width(ui_pct(cycles_children_column_width_pct, 0));
        ui_next_extra_box_flags(UI_BoxFlag_Clip);
        ui_column()
        {
            ui_next_width(ui_pct(1, 1));
            ui_row()
            {
                ui_next_width(ui_fill());
                ui_next_text_align(UI_TextAlign_Left);
                ui_text(str8_lit("Cycles w/ children"));
            }

            ui_spacer(ui_em(0.3f, 1));

            ui_next_width(ui_pct(1, 1));
            ui_next_height(ui_pixels(1, 1));
            ui_next_corner_radius(0);
            ui_next_color(v4f32(0.9f, 0.9f, 0.9f, 1));
            ui_box_make(UI_BoxFlag_DrawBackground, str8_lit(""));

            ui_spacer(ui_em(0.3f, 1));

            ui_text_align(UI_TextAlign_Right)
                ui_width(ui_pct(1, 1))
            {
                for (U64 i = 0; i < counter_values_count; ++i)
                {
                    Str8 string = {0};
                    if (profiling_per_frame)
                    {
                        string = str8_pushf(ui_frame_arena(), "%"PRIU64" (%5.2f%%)", counter_values->tsc_elapsed_with_children[i],  ((F32)counter_values->tsc_elapsed_with_children[i] / (F32)tsc_total) * 100.0f);
                    }
                    else
                    {
                        string = str8_pushf(ui_frame_arena(), "%"PRIU64, counter_values->tsc_elapsed_with_children[i]);
                    }
                    ui_text(string);
                }
            }
        }

        ui_next_width(ui_pixels(1, 1));
        ui_next_height(ui_em(60, 1));
        ui_next_corner_radius(0);
        ui_next_color(v4f32(0.9f, 0.9f, 0.9f, 1));
        ui_box_make(UI_BoxFlag_DrawBackground, str8_lit(""));

        //- hampus: Hit count

        ui_next_width(ui_pct(hit_count_column_width_pct, 0));
        ui_next_extra_box_flags(UI_BoxFlag_Clip);
        ui_column()
        {
            ui_next_width(ui_pct(1, 1));
            ui_row()
            {
                ui_next_width(ui_fill());
                ui_next_text_align(UI_TextAlign_Left);
                ui_text(str8_lit("Hit count"));
            }

            ui_spacer(ui_em(0.3f, 1));

            ui_next_width(ui_pct(1, 1));
            ui_next_height(ui_pixels(1, 1));
            ui_next_corner_radius(0);
            ui_next_color(v4f32(0.9f, 0.9f, 0.9f, 1));
            ui_box_make(UI_BoxFlag_DrawBackground, str8_lit(""));

            ui_spacer(ui_em(0.3f, 1));

            ui_text_align(UI_TextAlign_Right)
                ui_width(ui_pct(1, 1))
            {
                for (U64 i = 0; i < counter_values_count; ++i)
                {
                    ui_textf("%"PRIU64, counter_values->hit_count[i]);
                }
            }
        }
    }

    release_scratch(scratch);
}

////////////////////////////////
//~ hampus: Main

internal S32
os_main(Str8List arguments)
{
    debug_init();
    log_init(str8_lit("log.txt"));

    Arena *perm_arena = arena_create("MainPerm");

    framed_ui_state = push_struct(perm_arena, FramedUI_State);
    framed_ui_state->perm_arena = perm_arena;

    Gfx_Context gfx = gfx_init(0, 0, 720, 480, str8_lit("Framed"));
    Render_Context *renderer = render_init(&gfx);
    Arena *frame_arenas[2];
    frame_arenas[0] = arena_create("MainFrame0");
    frame_arenas[1] = arena_create("MainFrame1");

    ////////////////////////////////
    //- hampus: Initialize profiling state

    profiling_state = push_struct(perm_arena, ProfilingState);
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

    UI_Context *ui = ui_init();

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

    framed_ui_state->tab_view_function_table[FramedUI_TabView_Counter] = framed_ui_tab_view_counters;
    framed_ui_state->tab_view_function_table[FramedUI_TabView_Settings] = framed_ui_tab_view_settings;

    framed_ui_state->tab_view_string_table[FramedUI_TabView_Counter] = str8_lit("Counter");
    framed_ui_state->tab_view_string_table[FramedUI_TabView_Settings] = str8_lit("Settings");

    Gfx_Monitor monitor = gfx_monitor_from_window(&gfx);
    Vec2F32 monitor_dim = gfx_dim_from_monitor(monitor);
    FramedUI_Window *master_window = framed_ui_window_make(v2f32(0, 0), monitor_dim);
    framed_ui_window_push_to_front(master_window);

    {
        framed_ui_state->tab_view_table[FramedUI_TabView_Counter] = framed_ui_tab_make(framed_ui_tab_view_counters, 0, str8_lit("Counter"));
        framed_ui_panel_insert_tab(master_window->root_panel, framed_ui_state->tab_view_table[FramedUI_TabView_Counter], true);

        framed_ui_state->tab_view_table[FramedUI_TabView_Settings] = framed_ui_tab_make(framed_ui_tab_view_settings, 0, str8_lit("Settings"));
        framed_ui_panel_insert_tab(master_window->root_panel, framed_ui_state->tab_view_table[FramedUI_TabView_Settings], false);

#if BUILD_MODE_DEBUG
        framed_ui_panel_insert_tab(master_window->root_panel, framed_ui_tab_make(0, 0, str8_lit("")), false);
#endif
    }

    framed_ui_state->master_window = master_window;
    framed_ui_state->next_focused_panel = master_window->root_panel;

    //- hampus: Initialize UI settings

    framed_ui_state->settings.font_size = 12;

    gfx_set_window_maximized(&gfx);
    gfx_show_window(&gfx);

    ////////////////////////////////
    //- hampus: Main loop

    U64 start_counter = os_now_nanoseconds();
    F64 dt = 0;
    B32 running = true;
    B32 found_connection = false;
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
                            framed_ui_panel_insert_tab(debug_window->root_panel, debug_tab, true);

                            FramedUI_Tab *log_tab = framed_ui_tab_make(framed_ui_tab_view_logger, 0, str8_lit("Log"));
                            framed_ui_panel_insert_tab(debug_window->root_panel, log_tab, false);

                            FramedUI_Tab *texture_viewer_tab= framed_ui_tab_make(framed_ui_tab_view_texture_viewer, 0, str8_lit("Texture Viewer"));
                            framed_ui_panel_insert_tab(debug_window->root_panel, texture_viewer_tab, false);
                        }
                    }
                }
            }
        }

        ////////////////////////////////
        //- hampus: Zone pass

        //- hampus: Check connection state

        if (!net_socket_connection_is_alive(profiling_state->client_socket))
        {
            Net_AcceptResult accept_result = {0};
            if (found_connection)
            {
                // NOTE(hampus): The client disconnected
                log_info("Disconnected from client");
                found_connection = false;
            }
            accept_result = net_socket_accept(profiling_state->listen_socket);
            found_connection = accept_result.succeeded;
            if (found_connection)
            {
                profiling_state->client_socket = accept_result.socket;
                log_info("Connected to client");
                net_socket_set_blocking_mode(accept_result.socket, false);
                memory_zero_array(profiling_state->zone_blocks);
            }
        }

        //- hampus: Gather & process data from client

        if (net_socket_connection_is_alive(profiling_state->client_socket))
        {
            // NOTE(hampus): We currently peek and process one packet at a time.
            U16 buffer_size = 0;
            Net_RecieveResult peek_result = net_socket_peek(profiling_state->client_socket, (U8 *)&buffer_size, sizeof(buffer_size));
            while (peek_result.bytes_recieved)
            {
                Arena_Temporary scratch = get_scratch(0, 0);
                U8 *buffer = push_array(scratch.arena, U8, buffer_size);
                Net_RecieveResult recieve_result = net_socket_recieve(profiling_state->client_socket, buffer, buffer_size);
                // NOTE(hampus): First two bytes are the size of the packet
                U8 *buffer_pointer = buffer + sizeof(U16);
                U8 *buffer_opl = buffer + recieve_result.bytes_recieved;
                while (buffer_pointer < buffer_opl)
                {
                    // TODO(simon): Make sure we don't try to read past the end of the buffer.
                    PacketHeader *header = (PacketHeader *) (buffer_pointer);

                    switch (header->kind)
                    {
                        case Framed_PacketKind_FrameStart:
                        {
#pragma pack(push, 1)
                            typedef struct Packet Packet;
                            struct Packet
                            {
                                PacketHeader header;
                            };
#pragma pack(pop)

                            //- hampus: Save the last frame's data and produce a fresh captured frame

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

                            buffer_pointer += sizeof(Packet);
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

                            Str8 name = str8(packet->name, packet->name_length);

                            assert(profiling_state->zone_stack_size < array_count(profiling_state->zone_stack));
                            ZoneStack *opening = &profiling_state->zone_stack[profiling_state->zone_stack_size++];
                            memory_zero_struct(opening);

                            ZoneBlock *zone = framed_get_zone_block(perm_arena, name);

                            opening->name = zone->name;
                            opening->tsc_start = packet->header.tsc;
                            opening->old_tsc_elapsed_root = zone->tsc_elapsed_root;

                            buffer_pointer += sizeof(Packet) + packet->name_length;
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

                            Packet *packet = (Packet *) header;

                            ZoneStack *opening = &profiling_state->zone_stack[--profiling_state->zone_stack_size];
                            ZoneBlock *zone = framed_get_zone_block(perm_arena, opening->name);

                            U64 tsc_elapsed = packet->header.tsc - opening->tsc_start;

                            zone->tsc_elapsed += tsc_elapsed;
                            zone->tsc_elapsed_root = opening->old_tsc_elapsed_root + tsc_elapsed;
                            zone->tsc_elapsed_children += opening->tsc_elapsed_children;
                            ++zone->hit_count;
                            profiling_state->zone_stack[profiling_state->zone_stack_size - 1].tsc_elapsed_children += tsc_elapsed;

                            buffer_pointer += sizeof(Packet);
                        } break;
                        invalid_case;
                    }
                }
                release_scratch(scratch);
                peek_result = net_socket_peek(profiling_state->client_socket, (U8 *)&buffer_size, sizeof(buffer_size));
            }
        }
        ////////////////////////////////
        //- hampus: UI pass

        render_begin(renderer);

        ui_begin(ui, &events, renderer, dt);
        ui_push_font(str8_lit("data/fonts/NotoSansMono-Medium.ttf"));
        ui_push_font_size(framed_ui_state->settings.font_size);

        //- hampus: Menu bar

        UI_Key view_dropdown_key = ui_key_from_string(ui_key_null(), str8_lit( "ViewDropdownMenu"));

        ui_ctx_menu(view_dropdown_key)
        {
            ui_corner_radius(0)
            {
                for (U64 i = 0; i < array_count(framed_ui_state->tab_view_table); ++i)
                {
                    Str8 string = framed_ui_state->tab_view_string_table[i];
                    B32 active = !framed_ui_tab_is_nil(framed_ui_state->tab_view_table[i]);
                    ui_next_hover_cursor(Gfx_Cursor_Hand);
                    ui_next_extra_box_flags(UI_BoxFlag_DrawBorder |
                                            UI_BoxFlag_DrawBackground |
                                            UI_BoxFlag_ActiveAnimation |
                                            UI_BoxFlag_HotAnimation |
                                            UI_BoxFlag_Clickable);
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
                            framed_ui_panel_insert_tab(master_window->root_panel, framed_ui_state->tab_view_table[i], true);
                        }
                    }
                }
            }
        }

        ui_next_extra_box_flags(UI_BoxFlag_DrawBackground);
        ui_next_width(ui_fill());
        ui_corner_radius(0)
            ui_softness(0)
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

        start_counter = end_counter;
        framed_frame_counter++;
    }

    return(0);
}
