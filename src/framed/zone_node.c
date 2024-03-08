#define ZONE_NODE_LOOKUP_BY_NAME 1

global U64 id_stack[4096];
global U64 id_stack_pos = 2;
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
zone_node_from_id(Str8 name, U64 id)
{
    ZoneNode *result = 0;
    U64 slot_index = id % array_count(zone_vis_node_map);

    for (ZoneNode *node = zone_vis_node_map[slot_index]; node != 0; node = node->hash_next)
    {
#if ZONE_NODE_LOOKUP_BY_NAME
        if (memory_match(node->name.data, name.data, u64_min(node->name.size, name.size)) && node->name.size == name.size)
#else
        if (node->id == id)
#endif
        {
            result = node;
            break;
        }
    }

    if (result == 0)
    {
        result = zone_node_alloc();
        result->id = id;
        result->name = str8_copy(zone_node_arena, name);
        result->hash_next = zone_vis_node_map[slot_index];
        zone_vis_node_map[slot_index] = result;
    }

    return(result);
}

internal Void
zone_node_push_id(U64 id)
{
    id_stack[id_stack_pos] = id;
    id_stack_pos++;
}

internal Void
zone_node_pop_id(Void)
{
    assert(id_stack_pos > 0);
    id_stack_pos--;
}

internal U64
zone_node_hash_from_name(U64 seed, Str8 name)
{
    // TODO(hampus): Better seeding. Don't just add the value
    U64 result = hash_str8(name) + seed;
    return(result);
}

typedef struct ZoneTempValues ZoneTempValues;
struct ZoneTempValues
{
    U64 id;
    F64 ms_elapsed_exc;
    U64 tsc_start;
    U64 tsc_end;
};

internal ZoneNode *
zone_node_hierarchy_from_frame(Arena *arena, Frame *frame)
{
    profile_begin_function();

    id_stack_pos = 2;
    id_stack[0] = 0;

    Arena_Temporary scratch = get_scratch(&arena, 1);

    ZoneNode *root = push_struct_zero(arena, ZoneNode);

    ZoneNode *parent_node = root;

    // NOTE(hampus): These are just some extra stats we want to
    // keep around while creating the hierarchy.
    ZoneTempValues *exc_values_stack_base = push_array(scratch.arena, ZoneTempValues, 4096);
    ZoneTempValues *exc_values_stack = exc_values_stack_base;
    exc_values_stack->tsc_end = frame->end_tsc;
    exc_values_stack->tsc_start = 0;

    U64 cycles_per_second = frame->tsc_frequency;

    profile_begin_block("Zone block loop");
    for (U64 i = 0; i < frame->zone_blocks_count; ++i)
    {
        ZoneBlock *zone = frame->zone_blocks + i;

        if (zone->end_tsc == 0)
        {
            continue;
        }

        profile_begin_block("Parent stack walking");
        while (!(zone->start_tsc >= exc_values_stack->tsc_start && zone->end_tsc <= exc_values_stack->tsc_end))
        {
            parent_node->ms_min_elapsed_exc = f64_min(parent_node->ms_min_elapsed_exc, exc_values_stack->ms_elapsed_exc);
            parent_node->ms_max_elapsed_exc = f64_max(parent_node->ms_max_elapsed_exc, exc_values_stack->ms_elapsed_exc);
            parent_node->ms_elapsed_exc += exc_values_stack->ms_elapsed_exc;
            exc_values_stack--;
            if (parent_node->id != exc_values_stack->id)
            {
                parent_node = parent_node->parent;
                zone_node_pop_id();
            }
        }
        profile_end_block();

        profile_begin_block("Zone making");
        F64 ms_total = ((F64)(zone->end_tsc - zone->start_tsc)/(F64)cycles_per_second) * 1000.f;

        ZoneNode *node = parent_node;
        exc_values_stack->ms_elapsed_exc -= ms_total;

        U64 name_hash = hash_str8(zone->name);

        B32 recursive = false;
        // NOTE(hampus): Get what the id of the parent would be if
        // parent_node->name == zone->name
        U64 parent_id_if_recursive = name_hash + id_stack[id_stack_pos-2];
#if ZONE_NODE_LOOKUP_BY_NAME
        recursive = memory_match(zone->name.data, parent_node->name.data, u64_min(zone->name.size, parent_node->name.size)) && zone->name.size == parent_node->name.size;
#else
        recursive = node->id == parent_id_if_recursive;
#endif
        if (!recursive)
        {
            U64 id = name_hash + id_stack[id_stack_pos-1];
            node = 0;
            for (ZoneNode *n = parent_node->first; n != 0; n = n->next)
            {
#if ZONE_NODE_LOOKUP_BY_NAME
                if (memory_match(zone->name.data, n->name.data, u64_min(zone->name.size, n->name.size)) && zone->name.size == n->name.size)
#else
                if (id == n->id)
#endif
                {
                    node = n;
                }
            }

            if (node == 0)
            {
                node = zone_node_from_id(zone->name, id);
                node->parent = node->first = node->last = node->next = node->prev = 0;
                node->ms_elapsed_inc = node->ms_elapsed_exc = node->ms_max_elapsed_exc = 0;
                node->hit_count = 0;
                node->ms_min_elapsed_exc = (F64)U64_MAX;
                dll_push_back(parent_node->first, parent_node->last, node);
                node->parent = parent_node;
            }

            parent_node = node;

            zone_node_push_id(node->id);

            node->ms_elapsed_inc += ms_total;
        }

        exc_values_stack++;
        exc_values_stack->tsc_start = zone->start_tsc;
        exc_values_stack->tsc_end = zone->end_tsc;
        exc_values_stack->ms_elapsed_exc = ms_total;
        exc_values_stack->id = node->id;

        node->hit_count++;
        profile_end_block();
    }
    profile_end_block();

    profile_begin_block("Final parent stack walking");
    while (exc_values_stack >= exc_values_stack_base)
    {
        parent_node->ms_min_elapsed_exc = f64_min(parent_node->ms_min_elapsed_exc, exc_values_stack->ms_elapsed_exc);
        parent_node->ms_max_elapsed_exc = f64_max(parent_node->ms_max_elapsed_exc, exc_values_stack->ms_elapsed_exc);
        parent_node->ms_elapsed_exc += exc_values_stack->ms_elapsed_exc;
        exc_values_stack--;

        if (parent_node->id != exc_values_stack->id)
        {
            parent_node = parent_node->parent;
            zone_node_pop_id();
        }
    }
    profile_end_block();

    release_scratch(scratch);

    profile_end_function();

    return(root);
}

internal Void
zone_node_init(Void)
{
    zone_node_arena = arena_create("ZoneNodeArena");
}

internal ZoneNode *
zone_node_flatten(Arena *arena, ZoneNode *root)
{
    Arena_Temporary scratch = get_scratch(0, 0);

    ZoneNode *new_root = push_struct_zero(arena, ZoneNode);
    U64 map_size = 1024;
    ZoneNode **new_map = push_array_zero(arena, ZoneNode *, map_size);

    for (U64 i = 0; i < array_count(zone_vis_node_map); ++i)
    {
        ZoneNode *node = zone_vis_node_map[i];
        while (node)
        {
            U64 hash = hash_str8(node->name);
            U64 slot_index = hash % map_size;
            ZoneNode *map_entry = new_map[slot_index];
            while (map_entry)
            {
                if (memory_match(map_entry->name.data, node->name.data, u64_min(map_entry->name.size, node->name.size)) && map_entry->name.size == node->name.size)
                {
                    break;
                }

                map_entry = map_entry->hash_next;
            }

            if (!map_entry)
            {
                ZoneNode *new_map_entry = push_struct_zero(arena, ZoneNode);
                new_map_entry->name = node->name;
                new_map_entry->hash_next = new_map[slot_index];
                new_map[slot_index] = new_map_entry;
                new_map_entry->ms_min_elapsed_exc = (F64)U64_MAX;
                map_entry = new_map_entry;
                dll_push_back(new_root->first, new_root->last, new_map_entry);
            }

            map_entry->ms_elapsed_inc += node->ms_elapsed_inc;
            map_entry->ms_elapsed_exc += node->ms_elapsed_exc;
            map_entry->hit_count += node->hit_count;
            map_entry->ms_min_elapsed_exc = f64_min(map_entry->ms_min_elapsed_exc, node->ms_min_elapsed_exc);
            map_entry->ms_max_elapsed_exc = f64_max(map_entry->ms_max_elapsed_exc, node->ms_max_elapsed_exc);

            node = node->hash_next;
        }
    }

    release_scratch(scratch);

    return(new_root);
}
