typedef struct D_ClipNode D_ClipNode;
struct D_ClipNode
{
    D_ClipNode *next;
    RectF32 clip_rect;
};

typedef struct Draw_State Draw_State;
struct Draw_State
{
    Arena *frame_arena;

    D_ClipNode *clip_stack;
    D_ClipNode *clip_freelist;
    U64 current_generation;

    R_BatchList batches;
    U64 batch_generation;
};

global Draw_State draw_state;

// NOTE(simon): Initialization

internal Void
d_init(Void)
{
    draw_state.frame_arena = arena_create("DrawFrame");
}

// NOTE(simon): Frame markers

internal Void
d_begin_frame(Void)
{
    // NOTE(simon): Empty for now.
    arena_pop_to(draw_state.frame_arena, 0);
    memory_zero_struct(&draw_state.batches);
    draw_state.batch_generation   = 0;
    draw_state.current_generation = 0;
    draw_state.clip_stack         = 0;
    draw_state.clip_freelist      = 0;
}

internal Void
d_submit(Void)
{
    // NOTE(simon): Empty for now.
    r_submit(draw_state.batches);
}

// NOTE(simon): Draw commands

internal R_Batch *
d_create_batch(Void)
{
    draw_state.batch_generation = draw_state.current_generation;

    R_Batch *batch   = push_struct_zero(draw_state.frame_arena, R_Batch);
    batch->clip_rect = draw_state.clip_stack->clip_rect;
    batch->texture   = r_texture_zero();

    queue_push(draw_state.batches.first, draw_state.batches.last, batch);
    ++draw_state.batches.count;

    return batch;
}

internal R_Shape *
d_rect_(Vec2F32 min, Vec2F32 max, D_ShapeParams *parameters)
{
    R_Batch *batch = draw_state.batches.last;

    // NOTE(simon): Create a new batch if we don't have one or there are new
    // draw parameters.
    if (!batch || draw_state.batch_generation != draw_state.current_generation)
    {
        batch = d_create_batch();
    }

    // NOTE(simon): Use the requested texture if this batch doesn't have one
    // already.
    if (r_texture_equal(batch->texture, r_texture_zero()))
    {
        batch->texture = parameters->slice.texture;
    }

    // NOTE(simon): Create a new batch if the requested texture isn't
    // compatible with the batch.
    if (!r_texture_equal(parameters->slice.texture, r_texture_zero()) && !r_texture_equal(batch->texture, parameters->slice.texture))
    {
        batch          = d_create_batch();
        batch->texture = parameters->slice.texture;
    }

    // NOTE(simon): Allocate shape
    R_ShapeChunk *chunk = batch->shapes.last;
    if (!chunk || chunk->count >= chunk->capacity)
    {
        chunk           = push_struct_zero(draw_state.frame_arena, R_ShapeChunk);
        chunk->capacity = 1024;
        chunk->shapes   = push_array_zero(draw_state.frame_arena, R_Shape, chunk->capacity);

        queue_push(batch->shapes.first, batch->shapes.last, chunk);
        ++batch->shapes.chunk_count;
    }

    R_Shape *shape = &chunk->shapes[chunk->count];
    ++chunk->count;
    ++batch->shapes.shape_count;

    shape->min.x            = f32_round(min.x);
    shape->min.y            = f32_round(min.y);
    shape->max.x            = f32_round(max.x);
    shape->max.y            = f32_round(max.y);
    shape->min_uv           = parameters->slice.region.min;
    shape->max_uv           = parameters->slice.region.max;
    shape->colors[0]        = parameters->color;
    shape->colors[1]        = parameters->color;
    shape->colors[2]        = parameters->color;
    shape->colors[3]        = parameters->color;
    shape->radies[0]        = parameters->radius;
    shape->radies[1]        = parameters->radius;
    shape->radies[2]        = parameters->radius;
    shape->radies[3]        = parameters->radius;
    shape->softness         = parameters->softness;
    shape->border_thickness = parameters->border_thickness;
    shape->omit_texture     = r_texture_equal(parameters->slice.texture, r_texture_zero()) ? 1.0f : 0.0f;
    shape->is_subpixel_text = (F32) parameters->is_subpixel_text;
    shape->use_nearest      = (F32) parameters->use_nearest;

    return shape;
}

internal Void
d_push_clip(Vec2F32 min, Vec2F32 max, B32 clip_to_parent)
{
    D_ClipNode *node = draw_state.clip_freelist;
    if (node)
    {
        stack_pop(draw_state.clip_freelist);
    }
    else
    {
        node = push_struct_zero(draw_state.frame_arena, D_ClipNode);
    }

    memory_zero_struct(node);

    if (clip_to_parent && draw_state.clip_stack)
    {
        D_ClipNode *parent = draw_state.clip_stack;
        node->clip_rect    = rectf32_intersect_rectf32(rectf32(min, max), parent->clip_rect);
    }
    else
    {
        node->clip_rect.min = min;
        node->clip_rect.max = max;
    }

    stack_push(draw_state.clip_stack, node);
    ++draw_state.current_generation;
}

internal Void
d_pop_clip(Void)
{
    D_ClipNode *node = draw_state.clip_stack;
    stack_pop(draw_state.clip_stack);
    stack_push(draw_state.clip_freelist, node);
    ++draw_state.current_generation;
}

internal Void
d_character_internal(Vec2F32 min, U32 codepoint, R_Font *font, Vec4F32 color)
{
    r_character_internal(min, codepoint, font, color);
}

internal Void
d_text_internal(Vec2F32 min, Str8 text, R_Font *font, Vec4F32 color)
{
    r_text_internal(min, text, font, color);
}

internal Vec2F32
d_measure_character(R_Font *font, U32 codepoint)
{
    return r_measure_character(font, codepoint);
}

internal Vec2F32
d_measure_text(R_Font *font, Str8 text)
{
    return r_measure_text(font, text);
}

internal Vec2F32
d_measure_text_length(R_Font *font, Str8 text, U64 length)
{
    return d_measure_text(font, str8_prefix(text, length));
}
