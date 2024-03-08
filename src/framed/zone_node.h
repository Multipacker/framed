#ifndef ZONE_NODE_H
#define ZONE_NODE_H

typedef enum ZoneNodeFlag ZoneNodeFlag;
enum ZoneNodeFlag
{
    ZoneNodeFlag_Collapsed = (1 << 0),
};

typedef struct FreeZoneNode FreeZoneNode;
struct FreeZoneNode
{
    FreeZoneNode *next;
};

typedef struct ZoneNode ZoneNode;
struct ZoneNode
{
    ZoneNode *parent;
    ZoneNode *next;
    ZoneNode *prev;
    ZoneNode *first;
    ZoneNode *last;

    ZoneNode *hash_next;
    ZoneNode *hash_prev;

    ZoneNodeFlag flags;
    U64 id;

    Str8 name;
    F64 ms_elapsed_inc;
    F64 ms_elapsed_exc;
    F64 ms_min_elapsed_exc;
    F64 ms_max_elapsed_exc;
    U64 hit_count;
};

internal ZoneNode *zone_node_alloc(Void);
internal Void zone_node_free(ZoneNode *zone);
internal ZoneNode *zone_node_from_id(Str8 name, U64 id);
internal ZoneNode *zone_node_hierarchy_from_frame(Arena *arena, Frame *frame);
internal ZoneNode *zone_node_flatten(Arena *arena, ZoneNode *root);

#endif //ZONE_NODE_H
