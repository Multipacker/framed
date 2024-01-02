#include "base/base_inc.h"
#include "os/os_inc.h"
#include "log/log_inc.h"
#include "gfx/gfx_inc.h"
#include "render/render_inc.h"
#include "ui/ui_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "log/log_inc.c"
#include "gfx/gfx_inc.c"
#include "render/render_inc.c"
#include "ui/ui_inc.c"

internal Void
ui_logger(B32 *log_keep)
{
	R_FontKey mono = render_key_from_font(str8_lit("data/fonts/liberation-mono.ttf"), 7);

	ui_next_child_layout_axis(Axis2_X);

	UI_Box *log_window = ui_box_make(
									 UI_BoxFlag_DrawBackground |
									 UI_BoxFlag_AnimateHeight,
									 str8_lit("LogWindow")
									 );

	ui_parent(log_window)
	{
		ui_next_width(ui_fill());
		ui_next_height(ui_fill());
		ui_next_extra_box_flags(UI_BoxFlag_AnimateHeight);
		ui_push_scrollable_region(str8_lit("LogEntries"));
		ui_push_font(mono);

		local B32 only_info = false;

		U32 entry_count = 0;
		Log_QueueEntry *entries = log_get_entries(&entry_count);

		for (S32 i = (S32) entry_count - 1; i >= 0; --i)
		{
			Log_QueueEntry *entry = &entries[i];

			Vec4F32 color = { 0 };
			switch (entry->level)
			{
				case Log_Level_Info:    color = v4f32_mul_f32(v4f32(229.0f, 229.0f, 229.0f, 255.0f), 1.0f / 255.0f); break;
				case Log_Level_Warning: color = v4f32_mul_f32(v4f32(229.0f, 227.0f,  91.0f, 255.0f), 1.0f / 255.0f); break;
				case Log_Level_Error:   color = v4f32_mul_f32(v4f32(229.0f, 100.0f,  91.0f, 255.0f), 1.0f / 255.0f); break;
				case Log_Level_Trace:   color = v4f32_mul_f32(v4f32(121.4f, 229.0f,  91.4f, 255.0f), 1.0f / 255.0f); break;
				invalid_case;
			}

			// NOTE(simon): Skip the trailing new-line.
			Str8 message = str8_chop(log_format_entry(ui_frame_arena(), entry), 1);
			ui_next_text_color(color);
			ui_text(message);
		}

		ui_pop_font();
		ui_pop_scrollable_region();

		ui_column()
		{
			ui_spacer(ui_em(0.4f, 1));

			ui_row()
			{
				ui_spacer(ui_em(0.4f, 1));
				ui_check(log_keep, str8_lit("LogKeep"));
				ui_spacer(ui_em(0.4f, 1));
				ui_text(str8_lit("Keep entries"));
			}

			ui_row()
			{
				ui_spacer(ui_em(0.4f, 1));
				ui_check(&only_info, str8_lit("LogOnlyInfo"));
				ui_spacer(ui_em(0.4f, 1));
				ui_text(str8_lit("Only info"));
			}
		}
	}
}

internal Void
ui_ctx_menu_test(Void)
{
	UI_Key my_ctx_menu = ui_key_from_string(ui_key_null(), str8_lit("MyContextMenu"));
	
	ui_ctx_menu(my_ctx_menu)
		ui_width(ui_em(4, 1))
	{
			if (ui_button(str8_lit("Test")).pressed)
			{
				printf("Hello world!");
			}
			
			if (ui_button(str8_lit("Test2")).pressed)
			{
				printf("Hello world!");
			}
			
			if (ui_button(str8_lit("Test3")).pressed)
			{
				printf("Hello world!");
			}
	}
	
	UI_Key my_ctx_menu2 = ui_key_from_string(ui_key_null(), str8_lit("MyContextMenu2"));
	
	ui_ctx_menu(my_ctx_menu2)
		ui_width(ui_em(4, 1))
	{
			if (ui_button(str8_lit("Test5")).pressed)
			{
				printf("Hello world!");
			}
		
		ui_row()
		{
			if (ui_button(str8_lit("Test52")).pressed)
			{
				printf("Hello world!");
			}
			if (ui_button(str8_lit("Test5251")).pressed)
			{
				printf("Hello world2!");
			}
		}
			if (ui_button(str8_lit("Test53")).pressed)
			{
				printf("Hello world!");
			}
	}
	
	ui_row() 
		ui_corner_radius(0)
		ui_width(ui_em(4, 1))
	{
		UI_Comm comm = ui_button(str8_lit("File"));
		
		if (comm.hovering)
		{
			if (ui_ctx_menu_is_open())
			{
				ui_ctx_menu_open(comm.box->key, v2f32(0, 0), my_ctx_menu);
			}
		}
		if (comm.pressed )
		{
			ui_ctx_menu_open(comm.box->key, v2f32(0, 0), my_ctx_menu);
		}
		
		UI_Comm comm2 = ui_button(str8_lit("Edit"));
		
		if (comm2.hovering)
		{
			if (ui_ctx_menu_is_open())
			{
				ui_ctx_menu_open(comm2.box->key, v2f32(0, 0), my_ctx_menu2);
			}
		}
		if (comm2.pressed )
		{
			ui_ctx_menu_open(comm2.box->key, v2f32(0, 0), my_ctx_menu2);
		}
		
			ui_button(str8_lit("View"));
			ui_button(str8_lit("Options"));
			ui_button(str8_lit("Help"));
	}
}

internal S32
os_main(Str8List arguments)
{
	log_init(str8_lit("log.txt"));

	Gfx_Context gfx = gfx_init(0, 0, 720, 480, str8_lit("Title"));

	R_Context *renderer = render_init(&gfx);
	Arena *frame_arenas[2];
	frame_arenas[0] = arena_create();
	frame_arenas[1] = arena_create();

	UI_Context *ui = ui_init();

	U64 start_counter = os_now_nanoseconds();
	F64 dt = 0;

	R_TextureSlice slice = render_create_texture_slice(renderer, str8_lit("data/test.png"), R_ColorSpace_sRGB);

	gfx_show_window(&gfx);
	B32 running = true;
	while (running)
	{
		Arena *current_arena  = frame_arenas[0];
		Arena *previous_arena = frame_arenas[1];

		local B32 show_log = false;
		local B32 show_atlas = false;
		local B32 log_keep = false;

		Gfx_EventList events = gfx_get_events(current_arena, &gfx);
		for (Gfx_Event *event = events.first;
			 event != 0;
			 event = event->next)
		{
			switch (event->kind)
			{
				case Gfx_EventKind_Quit:
				{
					running = false;
				} break;

				case Gfx_EventKind_KeyPress:
				{
					if (event->key == Gfx_Key_F11)
					{
						gfx_toggle_fullscreen(&gfx);
					}
					else if (event->key == Gfx_Key_F1)
					{
						show_log = !show_log;
					}
					log_info("Key press!");
				} break;

				default:
				{
				} break;
			}
		}

		render_begin(renderer);

		ui_begin(ui, &events, renderer, dt);
		
		local S32 font_size = 15;
		
		R_FontKey font = render_key_from_font(str8_lit("data/fonts/Inter-Regular.ttf"), (U32) font_size);
		R_FontKey font2 = render_key_from_font(str8_lit("data/fonts/segoeuib.ttf"), 16);
		R_FontKey icon_font = render_key_from_font(str8_lit("data/fonts/fontello.ttf"), 16);
		
		ui_push_font(font);
		
		ui_ctx_menu_test();
			
		ui_next_width(ui_pct(1, 1));
		ui_row()
		{
			ui_spacer(ui_fill());
			if (ui_button(str8_lit("Increase font size")).pressed)
			{
				font_size += 1;
			}

			if (ui_button(str8_lit("Decrease font size")).pressed)
			{
				font_size -= 1;
			}
		}

		ui_row()
		{
			ui_check(&show_log, str8_lit("ShowLog"));
			ui_spacer(ui_em(0.4f, 1));
			ui_text(str8_lit("Show Log"));
		}

		ui_row()
		{
			ui_check(&show_atlas, str8_lit("ShowAtlas"));
			ui_spacer(ui_em(0.4f, 1));
			ui_text(str8_lit("Show Atlas"));
		}

		if (show_log)
		{
			ui_next_width(ui_fill());
			ui_next_height(ui_pct(0.25, 1));
			ui_logger(&log_keep);
		}

		if (show_atlas)
		{
			ui_next_width(ui_em(20, 1));
			ui_next_height(ui_em(20, 1));
			ui_next_color(v4f32(0.5, 0.5, 0.5, 1));
			UI_Box *atlas_parent = ui_box_make(UI_BoxFlag_DrawBackground |
											   UI_BoxFlag_Clip |
											   UI_BoxFlag_Clickable |
											   UI_BoxFlag_ViewScroll |
											   UI_BoxFlag_AnimateDim,
											   str8_lit("FontParent")
											   );
			ui_parent(atlas_parent)
			{
				local Vec2F32 offset = { 0 };
				local F32 scale = 1;

				ui_next_relative_pos(Axis2_X, offset.x);
				ui_next_relative_pos(Axis2_Y, offset.y);
				ui_next_width(ui_pct(1.0f / scale, 1));
				ui_next_height(ui_pct(1.0f / scale, 1));

				R_TextureSlice font_slice = render_slice_from_texture(renderer->font_atlas->texture,
																	  rectf32(v2f32(0, 0), v2f32(1, 1))
																	  );
				ui_next_slice(font_slice);
				ui_next_color(v4f32(1, 1, 1, 1));

				ui_next_corner_radius(0);
				UI_Box *atlas_box = ui_box_make(UI_BoxFlag_FloatingPos |
												UI_BoxFlag_DrawBackground,
												str8_lit("FontAtlas")
												);

				UI_Comm atlas_comm = ui_comm_from_box(atlas_parent);

				F32 old_scale = scale;
				scale *= f32_pow(2, atlas_comm.scroll.y * 0.1f);

				// TODO(simon): Slightly broken math, but mostly works.
				Vec2F32 scale_offset = v2f32_mul_f32(v2f32_sub_v2f32(atlas_comm.rel_mouse, offset), scale / old_scale - 1.0f);
				offset = v2f32_add_v2f32(v2f32_sub_v2f32(offset, atlas_comm.drag_delta), scale_offset);
			}
		}

		ui_next_color(v4f32(1, 1, 1, 1));
		ui_next_width(ui_em(10, 1));
		ui_next_height(ui_em(10, 1));
		ui_image(slice, str8_lit("TestSlice"));

		U64 result = 0;

		local U32 icon = R_ICON_STAR;

		ui_next_width(ui_em(1.0f, 1));
		ui_next_height(ui_em(1.0f, 1));
		ui_next_icon(icon);
		UI_Comm star_comm = ui_comm_from_box(ui_box_make(UI_BoxFlag_DrawText |
														 UI_BoxFlag_DrawBackground |
														 UI_BoxFlag_Clickable |
														 UI_BoxFlag_HotAnimation |
														 UI_BoxFlag_ActiveAnimation |
														 UI_BoxFlag_DrawBorder,
														 str8_lit("Star")));

		if (star_comm.pressed)
		{
			if (icon == R_ICON_STAR)
			{
				icon = R_ICON_STAR_EMPTY;
			}
			else
			{
				icon = R_ICON_STAR;
			}
		}

		UI_Comm comm = ui_button(str8_lit("Helloaa!##a"));

		if (comm.hovering)
		{
			ui_tooltip()
			{
				ui_text(str8_lit("Tooltip!"));
			}
		}

		if (comm.pressed)
		{
			printf("Pressed\n");
		}

		if (comm.released)
		{
			printf("Released\n");
		}

		if (comm.double_clicked)
		{
			printf("Double clicked!\n");
		}

		if (comm.right_pressed)
		{
			printf("Right pressed!\n");
		}

		if (comm.right_released)
		{
			printf("Right released!\n");
		}

		ui_next_corner_radius(0);
		ui_next_vert_gradient(v4f32(0, 0, 0, 1), v4f32(0, 1, 0, 1));
		ui_button(str8_lit("Helloaa!##b"));

		ui_row()
		{
			ui_check(&g_ui_ctx->show_debug_lines, str8_lit("MyTestBool"));
			ui_spacer(ui_em(0.4f, 1));
			ui_text(str8_lit("Show debug lines"));
		}

		ui_next_softness(5);
		ui_next_hori_gradient(v4f32(0, 1, 1, 1), v4f32(1, 1, 0, 1));
		ui_buttonf("Hehe%d", 5);

		ui_text(str8_lit("Text!"));

		UI_Box *parent = 0;

		ui_row()
		{
			ui_color(v4f32(1, 0, 0, 1))
				ui_border_color(v4f32(0, 1, 0, 1))
			{
				ui_next_width(ui_em(10, 1));
				ui_next_height(ui_em(10, 1));
				UI_Box *box = ui_box_make(UI_BoxFlag_DrawBackground |
										  UI_BoxFlag_DrawBorder |
										  UI_BoxFlag_DrawText |
										  UI_BoxFlag_AnimateDim |
										  UI_BoxFlag_AnimatePos,
										  str8_lit("Box2"));

				ui_box_equip_display_string(box, str8_lit("Hello!"));

				ui_next_width(ui_em(20, 1));
				ui_next_height(ui_em(20, 1));
				ui_next_child_layout_axis(Axis2_Y);
				parent = ui_box_make(UI_BoxFlag_DrawBackground |
									 UI_BoxFlag_DrawBorder |
									 UI_BoxFlag_DrawDropShadow |
									 UI_BoxFlag_AnimateDim |
									 UI_BoxFlag_AnimatePos |
									 UI_BoxFlag_ViewScroll |
									 UI_BoxFlag_Clip,
									 str8_lit("Parent"));

				UI_Comm parent_comm = ui_comm_from_box(parent);
				parent->scroll.y += (F32)(parent_comm.scroll.y*dt*5000);
			}
			ui_parent(parent)
			{
				ui_next_vert_corner_radius(20, 0);
				ui_next_width(ui_em(10, 1));
				ui_next_height(ui_em(10, 1));
				ui_next_font(font2);
				UI_Box *box1 = ui_box_make(UI_BoxFlag_DrawBackground |
										   UI_BoxFlag_DrawBorder |
										   UI_BoxFlag_DrawText,
										   str8_lit(""));

				ui_box_equip_display_string(box1, str8_lit("Font change!"));

				ui_spacer(ui_fill());
				ui_button(str8_lit("Centered button!"));
				ui_spacer(ui_fill());
			}

			ui_next_width(ui_children_sum(1));
			ui_next_height(ui_em(20, 1));
			ui_next_extra_box_flags(UI_BoxFlag_AnimatePos);
			ui_push_scrollable_region(str8_lit("Test"));

#if 0
			ui_row()
			{
				for (U64 i = 0; i < 5; ++i)
				{
					ui_spacer(ui_em(0.25f, 1));

					ui_column()
					{
						for (U64 j = 0; j < 20; ++j)
						{
							ui_spacer(ui_em(0.25f, 1));
							ui_next_width(ui_em(7.5f, 1));
							ui_buttonf("Hello %d%d", i, j);
							ui_spacer(ui_em(0.25f, 1));
						}
					}

					ui_spacer(ui_em(0.25f, 1));
				}
			}
#endif

			ui_pop_scrollable_region();

		}

		ui_end();

		render_end(renderer);

		if (!log_keep)
		{
			log_update_entries(1000);
		}

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);

		U64 end_counter = os_now_nanoseconds();
		dt = (F64) (end_counter - start_counter) / (F64) billion(1);
		start_counter = end_counter;
	}

	return(0);
}
