internal B32
gfx_key_pressed(Gfx_EventList *event_list, Gfx_Key key, B32 eat_event, Gfx_KeyModifier modifiers)
{
	B32 result = false;
	for (Gfx_Event *event = event_list->first;
		 event != 0;
		 event = event->next)
	{
		if (event->kind == Gfx_EventKind_KeyPress &&
			event->key == key)
		{
			if (modifiers)
			{
				if (!(event->key_modifiers & modifiers))
				{
					continue;
				}
			}
			result = true;
			if (eat_event)
			{
				dll_remove(event_list->first, event_list->last, event);
			}
			break;
		}

	}
	return(result);
}

internal B32
gfx_key_released(Gfx_EventList *event_list, Gfx_Key key, B32 eat_event, Gfx_KeyModifier modifiers)
{
	B32 result = false;
	for (Gfx_Event *event = event_list->first;
		 event != 0;
		 event = event->next)
	{
		if (event->kind == Gfx_EventKind_KeyRelease &&
			event->key == key)
		{
			if (modifiers)
			{
				if (!(event->key_modifiers & modifiers))
				{
					continue;
				}
			}
			result = true;
			if (eat_event)
			{
				dll_remove(event_list->first, event_list->last, event);
			}
			break;
		}

	}
	return(result);
}
