global B32 texture_view_ui_filtering_mode = Render_TextureFilter_Nearest;

internal Void
ui_texture_view(Render_TextureSlice atlas)
{
	ui_next_child_layout_axis(Axis2_X);
	UI_Box *texture_viewer = ui_box_make(0, str8_lit(""));

	ui_parent(texture_viewer)
	{
		ui_next_width(ui_fill());
		ui_next_height(ui_fill());
		ui_next_color(v4f32(0.5, 0.5, 0.5, 1));
		UI_Box *atlas_parent = ui_box_make(UI_BoxFlag_DrawBackground |
										   UI_BoxFlag_Clip |
										   UI_BoxFlag_Clickable |
										   UI_BoxFlag_ViewScroll,
										   str8_lit("TextureViewer")
										   );
		ui_parent(atlas_parent)
		{
			local Vec2F32 offset = { 0 };
			local F32 scale = 1;

			ui_next_relative_pos(Axis2_X, offset.x);
			ui_next_relative_pos(Axis2_Y, offset.y);
			ui_next_width(ui_pct(1.0f / scale, 1));
			ui_next_height(ui_pct(1.0f / scale, 1));

			ui_next_texture_filter(texture_view_ui_filtering_mode);
			ui_next_slice(atlas);
			ui_next_color(v4f32(1, 1, 1, 1));

			ui_next_corner_radius(0);
			UI_Box *atlas_box = ui_box_make(UI_BoxFlag_FloatingPos |
											UI_BoxFlag_DrawBackground,
											str8_lit("Texture")
											);

			UI_Comm atlas_comm = ui_comm_from_box(atlas_parent);

			F32 old_scale = scale;
			scale *= f32_pow(2, atlas_comm.scroll.y * 0.1f);

			// TODO(simon): Slightly broken math, but mostly works.
			Vec2F32 scale_offset = v2f32_mul_f32(v2f32_sub_v2f32(atlas_comm.rel_mouse, offset), scale / old_scale - 1.0f);
			offset = v2f32_add_v2f32(v2f32_sub_v2f32(offset, atlas_comm.drag_delta), scale_offset);
		}


		ui_column()
		{
			Str8 names[Render_TextureFilter_COUNT] = {
				[Render_TextureFilter_Bilinear] = str8_lit("Bilinear"),
				[Render_TextureFilter_Nearest]  = str8_lit("Nearest"),
			};

			ui_combo_box(str8_lit("Filtering:"), &texture_view_ui_filtering_mode, names, array_count(names));
		}
	}
}
