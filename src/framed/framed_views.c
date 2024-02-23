////////////////////////////////
//~ hampus: Counter tab view
#define FRAMED_COUNTERS_COLUMN_VIEW(name) Void name(Void *values_array, U64 values_count, B32 profiling_per_frame, U64 tsc_total)
typedef FRAMED_COUNTERS_COLUMN_VIEW(FramedCountersColumnView);

FRAMED_COUNTERS_COLUMN_VIEW(framed_counter_column_name)
{
    Str8 *names = (Str8 *)values_array;
    for (U64 i = 0; i < values_count; ++i)
    {
        ui_text(names[i]);
    }
}

FRAMED_COUNTERS_COLUMN_VIEW(framed_counter_column_cycles)
{
    U64 *cycle_values = (U64 *)values_array;
    ui_text_align(UI_TextAlign_Right)
        ui_width(ui_pct(1, 1))
    {
        for (U64 i = 0; i < values_count; ++i)
        {
            Str8 string = {0};
            if (profiling_per_frame)
            {
                string = str8_pushf(ui_frame_arena(), "%"PRIU64" (%5.2f%%)", cycle_values[i],  ((F32)cycle_values[i] / (F32)tsc_total) * 100.0f);
            }
            else
            {
                string = str8_pushf(ui_frame_arena(), "%"PRIU64, cycle_values[i]);
            }
            ui_text(string);
        }
    }
}

FRAMED_COUNTERS_COLUMN_VIEW(framed_counter_column_cycles_with_children)
{
    U64 *cycle_values = (U64 *)values_array;
    ui_text_align(UI_TextAlign_Right)
        ui_width(ui_pct(1, 1))
    {
        for (U64 i = 0; i < values_count; ++i)
        {
            Str8 string = {0};
            if (profiling_per_frame)
            {
                string = str8_pushf(ui_frame_arena(), "%"PRIU64" (%5.2f%%)", cycle_values[i],  ((F32)cycle_values[i] / (F32)tsc_total) * 100.0f);
            }
            else
            {
                string = str8_pushf(ui_frame_arena(), "%"PRIU64, cycle_values[i]);
            }
            ui_text(string);
        }
    }
}

FRAMED_COUNTERS_COLUMN_VIEW(framed_counter_column_hit_count)
{
    U64 *hit_counts = (U64 *)values_array;
    ui_text_align(UI_TextAlign_Right)
        ui_width(ui_pct(1, 1))
    {
        for (U64 i = 0; i < values_count; ++i)
        {
            ui_textf("%"PRIU64, hit_counts[i]);
        }
    }
}

FRAMED_UI_TAB_VIEW(framed_ui_tab_view_counters)
{
    typedef struct TabViewData TabViewData;
    struct TabViewData
    {
        U64 column_sort_index;
        B32 column_sort_ascending;
    };

    TabViewData *data = framed_ui_get_view_data(view_info, TabViewData);

    Arena_Temporary scratch = get_scratch(0, 0);

    //- hampus: Prepare for zone parsing into another format

    typedef struct ZoneValues ZoneValues;
    struct ZoneValues
    {
        Str8 *names;
        U64 *tsc_elapsed_without_children;
        U64 *tsc_elapsed_with_children;
        U64 *hit_count;
    };

    U64 zone_values_count = 0;
    ZoneValues *zone_values = push_struct(scratch.arena, ZoneValues);

    U64 max_number_unique_zones = 4096;

    zone_values->names = push_array(scratch.arena, Str8, max_number_unique_zones);
    zone_values->tsc_elapsed_without_children = push_array(scratch.arena, U64, max_number_unique_zones);
    zone_values->tsc_elapsed_with_children = push_array(scratch.arena, U64, max_number_unique_zones);
    zone_values->hit_count = push_array(scratch.arena, U64, max_number_unique_zones);

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

    //- hampus: Parse zone blocks into a nicer format

    for (U64 i = 0; i < zone_block_array_count; ++i)
    {
        ZoneBlock *zone_block = zone_block_array + i;
        if (zone_block->name.size)
        {
            U64 tsc_without_children = zone_block->tsc_elapsed - zone_block->tsc_elapsed_children;
            U64 tsc_with_children = zone_block->tsc_elapsed_root;
            zone_values->names[zone_values_count] = zone_block->name;
            zone_values->tsc_elapsed_without_children[zone_values_count] = tsc_without_children;
            zone_values->tsc_elapsed_with_children[zone_values_count] = tsc_with_children;
            zone_values->hit_count[zone_values_count] = zone_block->hit_count;
            zone_values_count++;

            if (zone_values_count == max_number_unique_zones)
            {
                log_error("Too many unique zones");
                break;
            }
        }
    }

    //- hampus: Display zone values

    F32 name_column_width_pct = 0.25f;
    F32 cycles_column_width_pct = 0.25f;
    F32 cycles_children_column_width_pct = 0.25f;
    F32 hit_count_column_width_pct = 0.25f;

    Str8 column_names[] =
    {
        str8_lit("Name"),
        str8_lit("Cycles"),
        str8_lit("Cycles w/ children"),
        str8_lit("Hit count"),
    };

    FramedCountersColumnView *column_views[] =
    {
        framed_counter_column_name,
        framed_counter_column_cycles,
        framed_counter_column_cycles_with_children,
        framed_counter_column_hit_count,
    };

    ui_spacer(ui_em(0.3f, 1));

    ui_next_width(ui_pct(1, 1.0f));
    ui_next_height(ui_pct(1, 1.0f));
    ui_row()
    {
        ui_next_width(ui_em(50, 0.5f));
        ui_next_height(ui_pct(1, 1.0f));
        ui_row()
        {
            for (U64 i = 0; i < array_count(column_names); ++i)
            {
                ui_next_width(ui_pct(name_column_width_pct, 0));
                ui_next_extra_box_flags(UI_BoxFlag_Clip);
                ui_column()
                {
                    ui_next_width(ui_pct(1, 1));
                    ui_next_height(ui_em(1, 1));
                    ui_row()
                    {
                        ui_next_width(ui_fill());
                        ui_next_text_align(UI_TextAlign_Left);
                        ui_text(column_names[i]);

                        if (data->column_sort_index == i)
                        {
                            ui_spacer(ui_fill());

                            ui_next_icon(RENDER_ICON_DOWN);
                            ui_next_height(ui_pct(1, 1));
                            ui_next_width(ui_em(1, 1));
                            ui_box_make(UI_BoxFlag_DrawText, str8_lit(""));

                            ui_spacer(ui_em(0.5f, 1));
                        }
                    }

                    ui_spacer(ui_em(0.3f, 1));

                    ui_next_width(ui_pct(1, 1));
                    ui_next_height(ui_pixels(1, 1));
                    ui_next_corner_radius(0);
                    ui_next_color(v4f32(0.9f, 0.9f, 0.9f, 1));
                    ui_box_make(UI_BoxFlag_DrawBackground, str8_lit(""));

                    ui_spacer(ui_em(0.3f, 1));

                    Void *values = (Void *)((U64 *)zone_values)[i];

                    column_views[i](values, zone_values_count, profiling_per_frame, tsc_total);
                }

                ui_next_width(ui_pixels(1, 1));
                ui_next_height(ui_em(60, 1));
                ui_next_corner_radius(0);
                ui_next_color(v4f32(0.9f, 0.9f, 0.9f, 1));
                ui_box_make(UI_BoxFlag_DrawBackground, str8_lit(""));
            }
        }

        //- hampus: Display extra stats

        ui_column()
        {
            //- hampus: Misc profiling stats

            ui_spacer(ui_em(0.3f, 1));
            ui_textf("Timestamp counter frequency: %"PRIU64" cycles/second", profiling_state->tsc_frequency);

            // TODO(hampus): What to do if we are not profiling per frame
            if (profiling_per_frame)
            {
                ui_spacer(ui_em(0.3f, 1));
                F32 ms = ((F32)tsc_total/(F32)profiling_state->tsc_frequency) * 1000.0f;
                ui_textf("Total cycles for frame: %"PRIU64" (%.2fms)", tsc_total, ms);
                ui_spacer(ui_em(0.3f, 1));
                ui_textf("Frames captured: %"PRIU64, profiling_state->frame_index-1);
            }

            ui_spacer(ui_em(0.5f, 1));

            //- hampus: Zone parsing & gathering stats

            ui_next_font_size(framed_ui_font_size_from_scale(FramedUI_FontScale_Larger));
            ui_text(str8_lit("Zone gathering stats"));

            ui_next_height(ui_pixels(1, 1));
            ui_next_width(ui_em(15, 1));
            ui_next_corner_radius(0);
            ui_next_color(v4f32(0.9f, 0.9f, 0.9f, 1));
            ui_box_make(UI_BoxFlag_DrawBackground, str8_lit(""));

            ui_spacer(ui_em(0.5f, 1));

            B32 critical = profiling_state->bandwidth_average_rate > profiling_state->parsed_average_rate;

            Vec4F32 text_color = ui_top_text_style()->color;
            if (critical)
            {
                text_color = v4f32(0.9f, 0, 0, 1.0f);
            }

            ui_next_width(ui_em(20, 1));
            ui_row()
            {
                MemorySize avg_parsing_rate = memory_size_from_bytes(profiling_state->parsed_average_rate);
                ui_text(str8_lit("Zone parsing rate:"));
                ui_spacer(ui_fill());
                ui_textf("%.2f%2"PRISTR8"/s", avg_parsing_rate.amount, str8_expand(avg_parsing_rate.unit));
            }

            ui_next_width(ui_em(20, 1));
            ui_row()
                ui_text_color(text_color)
            {
                MemorySize avg_bandwidth_rate = memory_size_from_bytes(profiling_state->bandwidth_average_rate);
                ui_text(str8_lit("Bandwidth rate:"));
                ui_spacer(ui_fill());
                ui_textf("%.2f%2"PRISTR8"/s", avg_bandwidth_rate.amount, str8_expand(avg_bandwidth_rate.unit));
            }
        }
    }

    release_scratch(scratch);
}

FRAMED_UI_TAB_VIEW(framed_ui_tab_view_about)
{
    ui_push_width(ui_pct(1, 1));

    DateTime date = build_date_from_context();
    ui_next_font_size(framed_ui_font_size_from_scale(FramedUI_FontScale_Larger));
    ui_text(str8_lit("Framed"));
    ui_text(str8_lit("Copyright (c) 2024 Hampus Johansson and Simon Renhult"));
    ui_textf("Compiled %u/%u-%d %u:%u:%u", date.day + 1, date.month + 1, date.year, date.hour, date.minute, date.second);

    ui_spacer(ui_em(1, 1));

    ui_text(str8_lit("Framed is released under the MIT License, but some of the code is"));
    ui_text(str8_lit("released under different open source licenses. All licenses are"));
    ui_text(str8_lit("available at (https://github.com/Multipacker/framed/tree/main/licenses)."));

    ui_spacer(ui_em(1, 1));
    ui_text(str8_lit("Portions of this software are copyright (c) 2024 The FreeType Project"));
    ui_text(str8_lit("(www.freetype.org) and is licensed under the FreeType License"));
    ui_text(str8_lit("All rights reserved."));

    ui_spacer(ui_em(1, 1));
    ui_text(str8_lit("This software uses SDL 2 which is licensed under the zlib license."));

    ui_spacer(ui_em(1, 1));
    ui_text(str8_lit("This software bundles Noto Sans Mono Medium which is licensed under the"));
    ui_text(str8_lit("SIL Open Font License, Version 1.1 and is copyright (c) 2022"));
    ui_text(str8_lit("The Noto Project Authors (https://github.com/notofonts/latin-greek-cyrillic)."));

    ui_spacer(ui_em(1, 1));
    ui_text(str8_lit("This software bundles Font Awesome Free download which is licensed under the"));
    ui_text(str8_lit("SIL Open Font License, Version 1.1 and is copyright (c) 2023 Fonticons, Inc."));
    ui_text(str8_lit("(https://fontawesome.com)."));

    ui_pop_width();
}

FRAMED_UI_TAB_VIEW(framed_ui_tab_view_settings)
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
FRAMED_UI_TAB_VIEW(framed_ui_tab_view_logger)
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

FRAMED_UI_TAB_VIEW(framed_ui_tab_view_texture_viewer)
{
    ui_next_width(ui_fill());
    ui_next_height(ui_fill());
    Render_TextureSlice texture = {0};
    if (view_info->data)
    {
        texture = *(Render_TextureSlice *) view_info->data;
    }
    ui_texture_view(texture);
}

FRAMED_UI_TAB_VIEW(framed_ui_tab_view_debug)
{
    ui_next_width(ui_fill());
    ui_next_height(ui_fill());
    B32 view_info_data_initialized = view_info->data != 0;
    UI_ColorPickerData *color_picker_data = framed_ui_get_view_data(view_info, UI_ColorPickerData);
    ui_debug(color_picker_data);
}
