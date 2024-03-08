// [ ] @feature Sorting by column value

////////////////////////////////
//~ hampus: Zones tab view

internal Void
display_zone_min_exc(ZoneNode *root)
{
    TimeInterval time = time_interval_from_ms(root->ms_min_elapsed_exc);
    ui_textf("%'.2f%"PRISTR8, time.amount, str8_expand(time.unit));
    if (!(root->flags & ZoneNodeFlag_Collapsed))
    {
        for (ZoneNode *node = root->first; node != 0; node = node->next)
        {
            display_zone_min_exc(node);
        }
    }
}

internal Void
display_zone_max_exc(ZoneNode *root)
{
    TimeInterval time = time_interval_from_ms(root->ms_max_elapsed_exc);
    ui_textf("%'.2f%"PRISTR8, time.amount, str8_expand(time.unit));
    if (!(root->flags & ZoneNodeFlag_Collapsed))
    {
        for (ZoneNode *node = root->first; node != 0; node = node->next)
        {
            display_zone_max_exc(node);
        }
    }
}

internal Void
display_zone_hit_count(ZoneNode *root)
{
    ui_textf("%'"PRIU64, root->hit_count);if (!(root->flags & ZoneNodeFlag_Collapsed))
    {
        for (ZoneNode *node = root->first; node != 0; node = node->next)
        {
            display_zone_hit_count(node);
        }
    }
}

internal Void
display_zone_exc_avg(ZoneNode *root)
{
    TimeInterval time = time_interval_from_ms(root->ms_elapsed_exc / (F64) root->hit_count);
    ui_textf("%'.2f%"PRISTR8, time.amount, str8_expand(time.unit));
    if (!(root->flags & ZoneNodeFlag_Collapsed))
    {
        for (ZoneNode *node = root->first; node != 0; node = node->next)
        {
            display_zone_exc_avg(node);
        }
    }
}

internal Void
display_zone_inc(ZoneNode *root)
{
    TimeInterval time = time_interval_from_ms(root->ms_elapsed_inc);
    ui_textf("%'.2f%"PRISTR8, time.amount, str8_expand(time.unit));
    if (!(root->flags & ZoneNodeFlag_Collapsed))
    {
        for (ZoneNode *node = root->first; node != 0; node = node->next)
        {
            display_zone_inc(node);
        }
    }
}

internal Void
display_zone_exc(ZoneNode *root)
{
    TimeInterval time = time_interval_from_ms(root->ms_elapsed_exc);
    ui_textf("%'.2f%"PRISTR8, time.amount, str8_expand(time.unit));
    if (!(root->flags & ZoneNodeFlag_Collapsed))
    {
        for (ZoneNode *node = root->first; node != 0; node = node->next)
        {
            display_zone_exc(node);
        }
    }
}

internal Void
display_zone_exc_pct(ZoneNode *root, F64 total_ms)
{
    F64 pct = (root->ms_elapsed_exc/total_ms);
    ui_textf("%.2f%%", pct*100.0f);
    if (!(root->flags & ZoneNodeFlag_Collapsed))
    {
        for (ZoneNode *node = root->first; node != 0; node = node->next)
        {
            display_zone_exc_pct(node, total_ms);
        }
    }
}

internal Void
display_zone_name(ZoneNode *root)
{
    if (root->first)
    {
        ui_next_extra_box_flags(UI_BoxFlag_Clickable | UI_BoxFlag_HotAnimation | UI_BoxFlag_ActiveAnimation);
        ui_next_hover_cursor(Gfx_Cursor_Hand);
        ui_next_width(ui_pct(1, 0));
        UI_Box *row = ui_named_row_beginf("%"PRIU64, root->id);
        UI_Comm comm = ui_comm_from_box(row);
        if (comm.hovering)
        {
            row->flags |= UI_BoxFlag_DrawBackground;
        }
        if (comm.pressed)
        {
            root->flags ^= ZoneNodeFlag_Collapsed;
        }
        ui_next_width(ui_em(1, 1));
        ui_next_height(ui_em(1, 1));
        if (root->flags & ZoneNodeFlag_Collapsed)
        {
            ui_next_icon(RENDER_ICON_RIGHT_OPEN);
        }
        else
        {
            ui_next_icon(RENDER_ICON_DOWN_OPEN);
        }
        ui_box_make(UI_BoxFlag_DrawText, str8_lit(""));
        ui_next_text_padding(Axis2_X, ui_top_font_line_height()*0.2f);
        ui_text(root->name);
        ui_named_row_end();
    }
    else
    {
        ui_text(root->name);
    }

    if (!(root->flags & ZoneNodeFlag_Collapsed))
    {
        ui_next_width(ui_pct(1, 0));
        ui_row()
        {
            ui_spacer(ui_em(1.2f, 1));
            ui_next_width(ui_pct(1, 0));
            ui_column()
            {
                for (ZoneNode *node = root->first; node != 0; node = node->next)
                {
                    display_zone_name(node);
                }
            }
        }
    }
}

FRAMED_UI_TAB_VIEW(framed_ui_tab_view_zones)
{
    B32 data_initialized = view_info->data != 0;

    local Str8 column_names[] =
    {
        str8_comp("Name"),
        str8_comp("Exc."),
        str8_comp("Exc. (%)"),
        str8_comp("Inc."),
        str8_comp("Hit count"),
        str8_comp("Avg. Exc."),
        str8_comp("Min exc."),
        str8_comp("Max exc."),
    };

    typedef struct TabViewData TabViewData;
    struct TabViewData
    {
        B32 flatten;
        B32 ascending_sort;
        F32 column_sizes_in_pct[array_count(column_names)];
        U64 column_sort_index;

        UI_TextEditState port_text_edit_state;
        U8 *port_text_buffer;
        U64 port_text_buffer_size;
        U64 port_string_length;
    };

    TabViewData *view_data = framed_ui_get_view_data(view_info, TabViewData);

    ProfilingState *profiling_state = framed_state->profiling_state;

    if (!data_initialized)
    {
        view_data->column_sizes_in_pct[0] = 0.3f;

        for (U64 i = 1; i < (array_count(column_names)); ++i)
        {
            view_data->column_sizes_in_pct[i] = (1 - 0.3f) / (array_count(column_names) - 1);
        }

        view_data->port_text_buffer_size = 5;
        view_data->port_text_buffer = push_array(view_info->arena, U8, view_data->port_text_buffer_size);
        arena_scratch(0, 0)
        {
            Str8 text_buffer_initial_string = str8_pushf(scratch, "%"PRIU16, profiling_state->port);
            U64 initial_string_length = u64_min(text_buffer_initial_string.size, view_data->port_text_buffer_size);
            memory_copy_typed(view_data->port_text_buffer, text_buffer_initial_string.data, initial_string_length);
            view_data->port_string_length = initial_string_length;
        }
    }

    Frame *frame = 0;
    B32 profiling_per_frame = profiling_state->frame_index != 0;
    if (profiling_per_frame)
    {
        frame = &profiling_state->finished_frame;
    }
    else
    {
        frame = &profiling_state->current_frame;
    }

    Arena_Temporary scratch = get_scratch(0, 0);

    U64 parse_start_time_ns = os_now_nanoseconds();

    profiling_state->parsed_bytes += frame->zone_blocks_count * sizeof(ZoneBlock);
    ZoneNode *root = zone_node_hierarchy_from_frame(scratch.arena, frame);

    U64 parse_end_time_ns = os_now_nanoseconds();

    profiling_state->parsed_time_accumulator += (F64)(parse_end_time_ns - parse_start_time_ns) / (F64)billion(1);

    ui_row()
    {
        ui_text(str8_lit("Flatten hierarchy"));
        ui_spacer(ui_em(0.5f, 1));
        ui_check(&view_data->flatten, str8_lit("FlattenCheck"));
    }

    if (view_data->flatten)
    {
        ZoneNode *new_root = zone_node_flatten(scratch.arena, root);
        root = new_root;
    }

    ui_spacer(ui_em(0.5f, 1));

    F32 new_column_pcts[array_count(column_names)] = {0};
    memory_copy_array(new_column_pcts, view_data->column_sizes_in_pct);

    if (frame->tsc_frequency == 0)
    {
        frame->tsc_frequency = 1;
    }

    F64 total_ms = 0;
    F64 frame_time_ms = 0;
    if (profiling_per_frame)
    {
        total_ms = (F64)(frame->end_tsc - frame->start_tsc) / (F64)frame->tsc_frequency * 1000.0;
        frame_time_ms = total_ms;
    }
    else
    {
        total_ms = (F64)(profiling_state->profile_end_tsc - profiling_state->profile_start_tsc) / (F64)frame->tsc_frequency * 1000.0;
    }

    if (root)
    {
        CompareFunc *compare_func = zone_node_compare_name;
        switch (view_data->column_sort_index)
        {
            case 1: { compare_func = zone_node_compare_ms_elapsed_exc; } break;
            case 2: { compare_func = zone_node_compare_ms_elapsed_exc_pct; } break;
            case 3: { compare_func = zone_node_compare_ms_elapsed_inc; } break;
            case 4: { compare_func = zone_node_compare_hit_count; } break;
            case 5: { compare_func = zone_node_compare_avg_exc; } break;
            case 6: { compare_func = zone_node_compare_min_exc; } break;
            case 7: { compare_func = zone_node_compare_max_exc; } break;
        }
        sort_children(root, view_data->ascending_sort, compare_func);
    }

    ui_next_height(ui_pct(1, 1));
    ui_next_width(ui_pct(1, 1));
    ui_row()
    {
        ui_next_height(ui_pct(1, 1));
        ui_next_width(ui_pct(0.75f, 1));
        UI_Box *row_parent = ui_named_row_begin(str8_lit("ZoneDisplayContainer"));
        ui_corner_radius(0)
        {
            for (U64 i = 0; i < array_count(column_names); ++i)
            {
                ui_next_width(ui_pct(view_data->column_sizes_in_pct[i], 0));
                ui_next_extra_box_flags(UI_BoxFlag_Clip);
                ui_named_column_beginf("ZoneColumn%"PRIU64, i);

                ui_next_width(ui_pct(1, 1));
                ui_next_height(ui_em(1, 1));
                ui_next_child_layout_axis(Axis2_X);
                UI_Box *title_box = ui_box_makef(UI_BoxFlag_DrawText |
                                                 UI_BoxFlag_DrawBackground |
                                                 UI_BoxFlag_Clickable |
                                                 UI_BoxFlag_HotAnimation |
                                                 UI_BoxFlag_ActiveAnimation,
                                                 "ZoneTitleRow%"PRIU64, i);
                ui_box_equip_display_string(title_box, column_names[i]);

                ui_parent(title_box)
                {
                    ui_spacer(ui_fill());

                    ui_next_height(ui_em(1, 1));
                    ui_next_width(ui_em(1, 1));
                    if (view_data->ascending_sort)
                    {
                        ui_next_icon(RENDER_ICON_UP);
                    }
                    else
                    {
                        ui_next_icon(RENDER_ICON_DOWN);
                    }
                    ui_box_make(UI_BoxFlag_DrawText * (view_data->column_sort_index == i),
                                str8_lit(""));
                }

                UI_Comm title_box_comm = ui_comm_from_box(title_box);

                if (title_box_comm.pressed)
                {
                    if (view_data->column_sort_index == i)
                    {
                        view_data->ascending_sort = !view_data->ascending_sort;
                    }
                    else
                    {
                        view_data->column_sort_index = i;
                    }
                }

                ui_next_width(ui_pct(1, 1));
                ui_next_height(ui_em(0.05f, 1));
                ui_next_color(v4f32(0.9f, 0.9f, 0.9f, 1.0f));
                ui_box_make(UI_BoxFlag_DrawBackground, str8_lit(""));

                switch (i)
                {
                    case 0:
                    {
                        for (ZoneNode *node = root->first; node != 0; node = node->next)
                        {
                            display_zone_name(node);
                        }
                    } break;
                    case 1:
                    {
                        ui_text_align(UI_TextAlign_Right)
                            ui_width(ui_pct(1, 1))
                        {
                            for (ZoneNode *node = root->first; node != 0; node = node->next)
                            {
                                display_zone_exc(node);
                            }
                        }
                    } break;

                    case 2:
                    {
                        ui_text_align(UI_TextAlign_Right)
                            ui_width(ui_pct(1, 1))
                        {
                            for (ZoneNode *node = root->first; node != 0; node = node->next)
                            {
                                display_zone_exc_pct(node, total_ms);
                            }
                        }
                    } break;
                    case 3:
                    {
                        ui_text_align(UI_TextAlign_Right)
                            ui_width(ui_pct(1, 1))
                        {
                            for (ZoneNode *node = root->first; node != 0; node = node->next)
                            {
                                display_zone_inc(node);
                            }
                        }
                    } break;
                    case 4:
                    {

                        ui_text_align(UI_TextAlign_Right)
                            ui_width(ui_pct(1, 1))
                        {
                            for (ZoneNode *node = root->first; node != 0; node = node->next)
                            {
                                display_zone_hit_count(node);
                            }
                        }
                    } break;
                    case 5:
                    {

                        ui_text_align(UI_TextAlign_Right)
                            ui_width(ui_pct(1, 1))
                        {
                            for (ZoneNode *node = root->first; node != 0; node = node->next)
                            {
                                display_zone_exc_avg(node);
                            }
                        }
                    } break;
                    case 6:
                    {
                        ui_text_align(UI_TextAlign_Right)
                            ui_width(ui_pct(1, 1))
                        {
                            for (ZoneNode *node = root->first; node != 0; node = node->next)
                            {
                                display_zone_min_exc(node);
                            }
                        }
                    } break;
                    case 7:
                    {
                        ui_text_align(UI_TextAlign_Right)
                            ui_width(ui_pct(1, 1))
                        {
                            for (ZoneNode *node = root->first; node != 0; node = node->next)
                            {
                                display_zone_max_exc(node);
                            }
                        }
                    } break;
                    invalid_case;
                }

                ui_named_column_end();

                ui_next_width(ui_em(0.3f, 1));
                ui_next_height(ui_pct(1, 1));
                ui_next_hover_cursor(Gfx_Cursor_SizeWE);
                ui_next_child_layout_axis(Axis2_X);
                UI_Box *column_divider_hitbox = ui_box_makef(UI_BoxFlag_Clickable,
                                                             "ColumnSplit%"PRIU64, i);

                UI_Comm column_comm = ui_comm_from_box(column_divider_hitbox);
                F32 drag_delta = column_comm.drag_delta.x;
                if (column_comm.dragging && i != (array_count(column_names)-1))
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
        }
        ui_named_row_end();

        U16 next_port = profiling_state->port;
        ui_column()
        {
            ui_box_makef(UI_BoxFlag_Disabled * !profiling_per_frame |
                         UI_BoxFlag_DrawText,
                         "Frames captured: %"PRIU64,
                         profiling_state->frame_index);

            ui_spacer(ui_em(0.25f, 1));

            TimeInterval frame_time_interval = time_interval_from_ms(frame_time_ms);
            ui_box_makef(UI_BoxFlag_Disabled * !profiling_per_frame |
                         UI_BoxFlag_DrawText,
                         "Frame time: %.2f%"PRISTR8" (%"PRIU64" fps)",
                         frame_time_interval.amount, str8_expand(frame_time_interval.unit), (U64)(1.0 / frame_time_ms));


            ui_spacer(ui_em(0.25f, 1));

            ui_textf("Zone count: %'"PRIU64, frame->zone_blocks_count);

            ui_spacer(ui_em(0.25f, 1));

            F64 profiling_time = (F64)(profiling_state->profile_end_tsc - profiling_state->profile_start_tsc) / (F64)frame->tsc_frequency * 1000.0f;

            TimeInterval profiling_time_interval = time_interval_from_ms(profiling_time);
            ui_textf("Profiling time elapsed: %.2f%"PRISTR8, profiling_time_interval.amount, str8_expand(profiling_time_interval.unit));

            ui_spacer(ui_em(0.25f, 1));

            ui_row()
            {
                B32 connection_alive = net_socket_connection_is_alive(profiling_state->client_socket);
                if (connection_alive)
                {
                    ui_push_extra_box_flags(UI_BoxFlag_Disabled);
                }

                ui_text(str8_lit("Listen port:"));

                UI_Comm comm = ui_line_edit(&view_data->port_text_edit_state,
                                            view_data->port_text_buffer,
                                            view_data->port_text_buffer_size,
                                            &view_data->port_string_length,
                                            str8_lit("CaptureFrequencyLineEdit"));
                if (connection_alive)
                {
                    ui_pop_extra_box_flags();
                }

                if (!ui_box_is_focused(comm.box))
                {
                    U16 u16 = 0;
                    u16_from_str8(str8(view_data->port_text_buffer, view_data->port_string_length), &u16);
                    // NOTE(hampus): Ports lower than 1024 are reserved by the system and ports over 50000
                    // are used when dynamically assigning ports
                    next_port = u16_clamp(1024, u16, 50000);
                    Str8 text_buffer_str8 = str8_pushf(scratch.arena, "%"PRIU32, next_port);
                    view_data->port_string_length = u64_min(text_buffer_str8.size, view_data->port_text_buffer_size);
                    memory_copy_typed(view_data->port_text_buffer, text_buffer_str8.data, view_data->port_string_length);
                }
            }

            ui_spacer(ui_em(0.25f, 1));

            MemorySize zone_parsing_throughput = memory_size_from_bytes(profiling_state->parsed_average_rate);
            ui_textf("Zone parsing throughput: %.2f%"PRISTR8"/s", zone_parsing_throughput.amount, str8_expand(zone_parsing_throughput.unit));

            ui_spacer(ui_em(0.25f, 1));

            MemorySize zone_gathering_troughput = memory_size_from_bytes(profiling_state->gather_average_rate);
            ui_textf("Zone gathering throughput: %.2f%"PRISTR8"/s", zone_gathering_troughput.amount, str8_expand(zone_gathering_troughput.unit));

            ui_spacer(ui_em(0.25f, 1));

            MemorySize network_transfer_bandwidth = memory_size_from_bytes(profiling_state->bandwidth_average_rate);
            ui_textf("Network transfer bandwidth: %.2f%"PRISTR8"/s", network_transfer_bandwidth.amount, str8_expand(network_transfer_bandwidth.unit));

            ui_spacer(ui_em(0.25f, 1));

        }

        if (next_port != profiling_state->port)
        {
            net_socket_free(profiling_state->listen_socket);

            profiling_state->listen_socket = net_socket_alloc(Net_Protocol_TCP, Net_AddressFamily_INET);
            Net_Address address =
            {
                .ip.u8[0] = 127,
                .ip.u8[1] = 0,
                .ip.u8[2] = 0,
                .ip.u8[3] = 1,
                .port = next_port,
            };

            net_socket_bind(profiling_state->listen_socket , address);;
            net_socket_set_blocking_mode(profiling_state->listen_socket , false);

            profiling_state->port = next_port;
        }
    }

    memory_copy_array(view_data->column_sizes_in_pct, new_column_pcts);

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
                UI_Box *box = ui_box_make(UI_BoxFlag_DrawBackground |
                                          UI_BoxFlag_DrawBorder |
                                          UI_BoxFlag_Clickable |
                                          UI_BoxFlag_HotAnimation |
                                          UI_BoxFlag_ActiveAnimation,
                                          string);
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
        framed_save_current_settings_to_file(framed_state->current_user_settings_file_path);
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
