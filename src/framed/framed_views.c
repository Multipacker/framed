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

internal int
framed_zone_block_compare_names(const Void *a, const Void *b)
{
    ZoneBlock *zone_block0 = (ZoneBlock *)a;
    ZoneBlock *zone_block1 = (ZoneBlock *)b;
    int result = (int) str8_are_codepoints_earliear(zone_block1->name, zone_block0->name);
    return(result);
}

internal int
framed_zone_block_compare_cycles_without_children(const Void *a, const Void *b)
{
    ZoneBlock *zone_block0 = (ZoneBlock *)a;
    ZoneBlock *zone_block1 = (ZoneBlock *)b;
    U64 tsc0 = zone_block0->tsc_elapsed - zone_block0->tsc_elapsed_children;
    U64 tsc1 = zone_block1->tsc_elapsed - zone_block1->tsc_elapsed_children;
    return(tsc0 < tsc1);
}

internal int
framed_zone_block_compare_cycles_with_children(const Void *a, const Void *b)
{
    ZoneBlock *zone_block0 = (ZoneBlock *)a;
    ZoneBlock *zone_block1 = (ZoneBlock *)b;
    U64 tsc0 = zone_block0->tsc_elapsed_root;
    U64 tsc1 = zone_block1->tsc_elapsed_root;
    return(tsc0 < tsc1);
}

internal int
framed_zone_block_compare_hit_count(const Void *a, const Void *b)
{
    ZoneBlock *zone_block0 = (ZoneBlock *)a;
    ZoneBlock *zone_block1 = (ZoneBlock *)b;
    return(zone_block0->hit_count < zone_block1->hit_count);
}

FRAMED_UI_TAB_VIEW(framed_ui_tab_view_counters)
{
    B32 data_initialized = view_info->data != 0;
    typedef struct TabViewData TabViewData;
    struct TabViewData
    {
        U64 column_sort_index;
        B32 column_sort_ascending;
        F32 column_sizes_in_pct[4];
    };

    TabViewData *data = framed_ui_get_view_data(view_info, TabViewData);

    if (!data_initialized)
    {
        data->column_sizes_in_pct[0] = 0.25f;
        data->column_sizes_in_pct[1] = 0.25f;
        data->column_sizes_in_pct[2] = 0.25f;
        data->column_sizes_in_pct[3] = 0.25f;
    }

    ProfilingState *profiling_state = framed_state->profiling_state;

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

    ZoneValues *zone_values = push_struct(scratch.arena, ZoneValues);

    zone_values->names = push_array(scratch.arena, Str8, MAX_NUMBER_OF_UNIQUE_ZONES);
    zone_values->tsc_elapsed_without_children = push_array(scratch.arena, U64, MAX_NUMBER_OF_UNIQUE_ZONES);
    zone_values->tsc_elapsed_with_children = push_array(scratch.arena, U64, MAX_NUMBER_OF_UNIQUE_ZONES);
    zone_values->hit_count = push_array(scratch.arena, U64, MAX_NUMBER_OF_UNIQUE_ZONES);

    //- hampus: Parse zone blocks into a nicer format

    ZoneBlock *zone_blocks = 0;
    U64 zone_blocks_count = 0;
    U64 tsc_total = 0;
    B32 profiling_per_frame = profiling_state->frame_begin_tsc != 0;;

    debug_block("Count up zone blocks")
    {
        ZoneBlock *zone_block_array = 0;

        if (profiling_per_frame)
        {
            // NOTE(hampus): We have got a full frame, take the values from there
            CapturedFrame *frame = &profiling_state->latest_captured_frame;
            zone_block_array = frame->zone_blocks;
            tsc_total = frame->total_tsc;
        }
        else
        {
            // NOTE(hampus): If we haven't got any frame yet, just take the current values
            zone_block_array = profiling_state->zone_blocks;
        }

        zone_blocks = push_array(scratch.arena, ZoneBlock, MAX_NUMBER_OF_UNIQUE_ZONES);
        for (U64 i = 0; i < MAX_NUMBER_OF_UNIQUE_ZONES; ++i)
        {
            ZoneBlock *zone_block = zone_block_array + i;
            if (zone_block->name.size)
            {
                if (zone_blocks_count == MAX_NUMBER_OF_UNIQUE_ZONES)
                {
                    log_error("Maximum number of unique zones was exceeded");
                    break;
                }

                zone_blocks[zone_blocks_count] = *zone_block;
                zone_blocks_count++;
            }
        }
    }

    debug_block("Sort counter columns")
    {
        switch (data->column_sort_index)
        {
            case 0:
            {
                qsort(zone_blocks, zone_blocks_count, ZoneBlock, framed_zone_block_compare_names);
            } break;

            case 1:
            {
                qsort(zone_blocks, zone_blocks_count, ZoneBlock, framed_zone_block_compare_cycles_without_children);
            } break;

            case 2:
            {
                qsort(zone_blocks, zone_blocks_count, ZoneBlock, framed_zone_block_compare_cycles_with_children);
            } break;

            case 3:
            {
                qsort(zone_blocks, zone_blocks_count, ZoneBlock, framed_zone_block_compare_hit_count);
            } break;
        }
    }

    debug_block("Parse zone blocks into SOA")
    {
        if (data->column_sort_ascending)
        {
            for (U64 i = 0; i < zone_blocks_count; ++i)
            {
                ZoneBlock *zone_block = zone_blocks + (zone_blocks_count-1-i);

                U64 tsc_without_children = zone_block->tsc_elapsed - zone_block->tsc_elapsed_children;
                U64 tsc_with_children = zone_block->tsc_elapsed_root;

                zone_values->names[i] = zone_block->name;
                zone_values->tsc_elapsed_without_children[i] = tsc_without_children;
                zone_values->tsc_elapsed_with_children[i] = tsc_with_children;
                zone_values->hit_count[i] = zone_block->hit_count;
            }
        }
        else
        {
            for (U64 i = 0; i < zone_blocks_count; ++i)
            {
                ZoneBlock *zone_block = zone_blocks + i;

                U64 tsc_without_children = zone_block->tsc_elapsed - zone_block->tsc_elapsed_children;
                U64 tsc_with_children = zone_block->tsc_elapsed_root;

                zone_values->names[i] = zone_block->name;
                zone_values->tsc_elapsed_without_children[i] = tsc_without_children;
                zone_values->tsc_elapsed_with_children[i] = tsc_with_children;
                zone_values->hit_count[i] = zone_block->hit_count;
            }
        }
    }
    //- hampus: Display zone values

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

    F32 new_column_pcts[4] = {0};
    memory_copy_array(new_column_pcts, data->column_sizes_in_pct);

    ui_spacer(ui_em(0.3f, 1));

    ui_next_width(ui_pct(1, 1.0f));
    ui_next_height(ui_pct(1, 1.0f));
    ui_row()
    {
        ui_next_width(ui_em(50, 0.5f));
        ui_next_height(ui_pct(1, 1.0f));
        UI_Box *row_parent = ui_named_row_begin(str8_lit("ZoneDisplayContainer"));
        {
            for (U64 i = 0; i < array_count(column_names); ++i)
            {
                ui_next_width(ui_pct(data->column_sizes_in_pct[i], 0));
                ui_next_extra_box_flags(UI_BoxFlag_Clip);
                ui_named_columnf("%"PRISTR8"Column", str8_expand(column_names[i]))
                {
                    ui_next_width(ui_pct(1, 1));
                    ui_next_height(ui_em(1.2f, 1));
                    ui_row()
                    {
                        ui_next_width(ui_fill());
                        ui_next_height(ui_pct(1, 1));
                        ui_next_text_align(UI_TextAlign_Left);
                        ui_next_hover_cursor(Gfx_Cursor_Hand);
                        ui_next_child_layout_axis(Axis2_X);
                        UI_Box *header_box = ui_box_make(UI_BoxFlag_DrawText |
                                                         UI_BoxFlag_Clickable |
                                                         UI_BoxFlag_HotAnimation |
                                                         UI_BoxFlag_ActiveAnimation |
                                                         UI_BoxFlag_DrawBackground,
                                                         column_names[i]);
                        UI_Comm header_comm = ui_comm_from_box(header_box);

                        if (data->column_sort_index == i)
                        {
                            ui_parent(header_box)
                            {
                                ui_next_text_align(UI_TextAlign_Right);
                                ui_next_icon(data->column_sort_ascending ? RENDER_ICON_UP : RENDER_ICON_DOWN);
                                ui_next_height(ui_pct(1, 1));
                                ui_next_width(ui_pct(1, 0));
                                ui_box_make(UI_BoxFlag_DrawText, str8_lit(""));
                                ui_spacer(ui_em(0.75f, 1));
                            }
                        }

                        if (header_comm.clicked)
                        {
                            if (data->column_sort_index == i)
                            {
                                data->column_sort_ascending ^= 1;
                            }
                            data->column_sort_index = i;
                        }
                    }

                    ui_next_width(ui_pct(1, 1));
                    ui_next_height(ui_pixels(1, 1));
                    ui_next_corner_radius(0);
                    ui_next_color(v4f32(0.9f, 0.9f, 0.9f, 1));
                    ui_box_make(UI_BoxFlag_DrawBackground, str8_lit(""));

                    ui_spacer(ui_em(0.3f, 1));

                    Void *values = (Void *)((U64 *)zone_values)[i];

                    column_views[i](values, zone_blocks_count, profiling_per_frame, tsc_total);
                }


                ui_next_width(ui_em(0.3f, 1));
                ui_next_height(ui_em(60, 1));
                ui_next_hover_cursor(Gfx_Cursor_SizeWE);
                ui_next_child_layout_axis(Axis2_X);
                UI_Box *column_divider_hitbox = ui_box_makef(UI_BoxFlag_Clickable,
                                                             "ColumnSplit%"PRIU64, i);

                UI_Comm column_comm = ui_comm_from_box(column_divider_hitbox);
                F32 drag_delta = column_comm.drag_delta.x;
                if (column_comm.dragging && i != 3)
                {
                    F32 pct_delta = drag_delta / row_parent->fixed_size.x;
                    F32 column0_max_pct_delta = new_column_pcts[i] - 0.01f;
                    F32 column1_max_pct_delta = -(new_column_pcts[i+1] - 0.01f);

                    pct_delta = f32_clamp(column1_max_pct_delta, pct_delta, column0_max_pct_delta);

                    new_column_pcts[i] -= pct_delta;
                    new_column_pcts[i+1] += pct_delta;
                }

                ui_parent(column_divider_hitbox)
                {
                    ui_spacer(ui_fill());

                    ui_next_width(ui_pixels(1, 1));
                    ui_next_height(ui_pct(1, 1));
                    ui_next_corner_radius(0);
                    ui_next_color(v4f32(0.9f, 0.9f, 0.9f, 1));
                    UI_Box *column_divider = ui_box_make(UI_BoxFlag_DrawBackground,
                                                         str8_lit(""));

                    ui_spacer(ui_fill());
                }
            }
            ui_named_row_end();
        }

        memory_copy_array(data->column_sizes_in_pct, new_column_pcts);

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
////////////////////////////////
//~ hampus: About tab view

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

////////////////////////////////
//~ hampus: Settings tab view

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

    // NOTE(hampus): Create a snapshot of the settings so that
    // we can see at the end if anything has changed.

    FramedUI_Settings old_settings = framed_ui_state->settings;

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
                Str8 string = framed_ui_string_color_table[color];
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
    }
    if (!memory_match(&old_settings, &framed_ui_state->settings, sizeof(FramedUI_Settings)))
    {
        framed_save_current_settings_to_file();
    }
}

////////////////////////////////
//~ hampus: Logger tab view

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

////////////////////////////////
//~ hampus: Texture viewer tab view

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

////////////////////////////////
//~ hampus: Debug tab view

FRAMED_UI_TAB_VIEW(framed_ui_tab_view_debug)
{
    ui_next_width(ui_fill());
    ui_next_height(ui_fill());
    B32 view_info_data_initialized = view_info->data != 0;
    UI_ColorPickerData *color_picker_data = framed_ui_get_view_data(view_info, UI_ColorPickerData);
    ui_debug(color_picker_data);
}
