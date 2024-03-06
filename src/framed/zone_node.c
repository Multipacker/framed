global U64 id_stack[4096];
global U64 id_stack_pos = 1;
global ZoneNode *zone_vis_node_map[4096];
global FreeZoneNode *first_free_zone_node;
global Arena *zone_node_arena;

internal ZoneNode *
zone_node_alloc(Void)
{
    ZoneNode *result = (ZoneNode *)first_free_zone_node;
    if (result == 0)
    {
        result = push_struct(zone_node_arena, ZoneNode);
    }
    else
    {
        first_free_zone_node = first_free_zone_node->next;
    }
    return(result);
}

internal Void
zone_node_free(ZoneNode *zone)
{
    FreeZoneNode *free_zone_node = (FreeZoneNode *)zone;
    free_zone_node->next = first_free_zone_node;
    first_free_zone_node = free_zone_node;
}

internal ZoneNode *
zone_node_from_id(U64 id)
{
    ZoneNode *result = 0;
    U64 slot_index = id % array_count(zone_vis_node_map);

    for (ZoneNode *node = zone_vis_node_map[slot_index]; node != 0; node = node->hash_next)
    {
        if (node->id == id)
        {
            result = node;
            break;
        }
    }

    if (result == 0)
    {
        result = zone_node_alloc();
        result->id = id;
        result->hash_next = zone_vis_node_map[slot_index];
        zone_vis_node_map[slot_index] = result;
    }

    return(result);
}

internal Void
zone_node_push_id(U64 id)
{
    id_stack[id_stack_pos] = id_stack[id_stack_pos-1] + id;
    id_stack_pos++;
}

internal Void
zone_node_pop_id(Void)
{
    assert(id_stack_pos > 0);
    id_stack_pos--;
}

internal U64
zone_node_hash_from_name(Str8 name)
{
    U64 result = hash_str8(name) + id_stack[id_stack_pos-1];
    return(result);
}

typedef struct ZoneTempValues ZoneTempValues;
struct ZoneTempValues
{
    Str8 name;
    F64 ms_elapsed_exc;
    U64 tsc_start;
    U64 tsc_end;
};

internal ZoneNode *
zone_node_hierarchy_from_frame(Frame *frame)
{
    id_stack_pos = 1;
    id_stack[0] = 0;

    Arena_Temporary scratch = get_scratch(0, 0);

    ZoneNode *root = push_struct_zero(frame->arena, ZoneNode);

    ZoneNode *parent_node = root;

    // NOTE(hampus): These are just some extra stats we want to
    // keep around while creating the hierarchy.
    ZoneTempValues *exc_values_stack_base = push_array(scratch.arena, ZoneTempValues, 512);
    ZoneTempValues *exc_values_stack = exc_values_stack_base;
    exc_values_stack->tsc_end = frame->end_tsc;
    exc_values_stack->tsc_start = 0;

    U64 cycles_per_second = frame->tsc_frequency;

    for (U64 i = 0; i < frame->zone_blocks_count; ++i)
    {
        ZoneBlock *zone = frame->zone_blocks + i;

        if (zone->end_tsc == 0)
        {
            continue;
        }

        while (!(zone->start_tsc >= exc_values_stack->tsc_start && zone->end_tsc <= exc_values_stack->tsc_end))
        {
            parent_node->ms_min_elapsed_exc = f64_min(parent_node->ms_min_elapsed_exc, exc_values_stack->ms_elapsed_exc);
            parent_node->ms_max_elapsed_exc = f64_max(parent_node->ms_max_elapsed_exc, exc_values_stack->ms_elapsed_exc);
            parent_node->ms_elapsed_exc += exc_values_stack->ms_elapsed_exc;
            exc_values_stack--;
            if (!str8_equal(parent_node->name, exc_values_stack->name))
            {
                parent_node = parent_node->parent;
                zone_node_pop_id();
            }
        }

        F64 ms_total = ((F64)(zone->end_tsc - zone->start_tsc)/(F64)cycles_per_second) * 1000.f;

        ZoneNode *node = parent_node;
        exc_values_stack->ms_elapsed_exc -= ms_total;
        if (!str8_equal(zone->name, node->name))
        {
            node = 0;
            for (ZoneNode *n = parent_node->first; n != 0; n = n->next)
            {
                if (str8_equal(zone->name, n->name))
                {
                    node = n;
                }
            }

            if (node == 0)
            {
                U64 id = zone_node_hash_from_name(zone->name);
                node = zone_node_from_id(id);
                node->parent = node->first = node->last = node->next = node->prev = 0;
                node->ms_elapsed_inc = node->ms_elapsed_exc = node->ms_max_elapsed_exc = 0;
                node->hit_count = 0;
                node->ms_min_elapsed_exc = (F64)U64_MAX;
                node->name = zone->name;
                dll_push_back(parent_node->first, parent_node->last, node);
                node->parent = parent_node;
            }

            parent_node = node;

            zone_node_push_id(hash_str8(node->name));

            node->ms_elapsed_inc += ms_total;
        }

        exc_values_stack++;
        exc_values_stack->tsc_start = zone->start_tsc;
        exc_values_stack->tsc_end = zone->end_tsc;
        exc_values_stack->ms_elapsed_exc = ms_total;
        exc_values_stack->name = zone->name;

        node->hit_count++;
    }

    while (exc_values_stack >= exc_values_stack_base)
    {
        parent_node->ms_min_elapsed_exc = f64_min(parent_node->ms_min_elapsed_exc, exc_values_stack->ms_elapsed_exc);
        parent_node->ms_max_elapsed_exc = f64_max(parent_node->ms_max_elapsed_exc, exc_values_stack->ms_elapsed_exc);
        parent_node->ms_elapsed_exc += exc_values_stack->ms_elapsed_exc;
        exc_values_stack--;

        if (!str8_equal(parent_node->name, exc_values_stack->name))
        {
            parent_node = parent_node->parent;
        }
    }

    release_scratch(scratch);

    return(root);
}

internal Void
zone_node_init(Void)
{
    zone_node_arena = arena_create("ZoneNodeArena");
}