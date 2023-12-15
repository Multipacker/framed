#include "base/base_inc.h"
#include "os/os_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"

typedef struct S32Node S32Node;
struct S32Node
{
    S32Node *next;
    S32Node *prev;
    S32 value;
};

typedef struct S32Queue S32Queue;
struct S32Queue
{
    S32Node *first;
    S32Node *last;
};

internal S32
os_main(Str8List arguments)
{
	DateTime build_date = build_date_from_context();
	Arena_Temporary scratch = arena_get_scratch(0, 0);
    
    S32Node *node0 = push_struct(scratch.arena, S32Node); 
    node0->value = 1;
    
    S32Node *node1 = push_struct(scratch.arena, S32Node); 
    node1->value = 5;
    
    S32Queue queue = {0};
    stack_push(queue.first, node0);
    stack_push(queue.first, node1);
    
    stack_pop(queue.first);
    
    arena_release_scratch(scratch);
    return(0);
}