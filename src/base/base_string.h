#ifndef BASE_STRING_H
#define BASE_STRING_H

typedef struct Arena Arena;

// NOTE(simon): These types do not gurantee that the data is null terminated.

typedef struct Str8 Str8;
struct Str8
{
    U8 *data;
    U64 size;
};

typedef struct Str16 Str16;
struct Str16
{
    U16 *data;
    U64  size;
};

typedef struct Str32 Str32;
struct Str32
{
    U32 *data;
    U64  size;
};

typedef struct Str8Node Str8Node;
struct Str8Node
{
    Str8Node *next;
    Str8Node *prev;
    Str8 string;
};

typedef struct Str8List Str8List;
struct Str8List
{
    Str8Node *first;
    Str8Node *last;
    U64 node_count;
    U64 total_size;
};

typedef char *CStr;
typedef U16  *CStr16;

#define str8_lit(literal) ((Str8) { .data = (U8 *) literal, .size = sizeof(literal) - 1})
#define str8_expand(string) (int) string.size, string.data

typedef struct StringDecode StringDecode;
struct StringDecode
{
    U32 codepoint;
    U64 size;
};

internal Str8 str8(U8 *data, U64 size);
internal Str8 str8_range(U8 *start, U8 *opl);
internal Str8 str8_copy(Arena *arena, Str8 string);
internal Str8 str8_cstr(CStr data);
internal Str8 str8_copy_cstr(Arena *arena, CStr data);

internal Str16 str16(U16 *data, U64 size);

internal Str8 str8_prefix(Str8 string, U64 size);
internal Str8 str8_postfix(Str8 string, U64 size);
internal Str8 str8_skip(Str8 string, U64 size);
internal Str8 str8_chop(Str8 string, U64 size);
internal Str8 str8_substring(Str8 string, U64 start, U64 size);

internal Str8 str8_pushfv(Arena *arena, CStr cstr, va_list args);
internal Str8 str8_pushf(Arena *arena, CStr cstr, ...);

// NOTE(simon): These functions are only reliable for ASCII.
// TODO(hampus): Add flags for comparison of strings
internal B32 str8_equal(Str8 a, Str8 b);
internal B32 str8_first_index_of(Str8 string, U32 codepoint, U64 *result_index);
internal B32 str8_last_index_of(Str8 string, U32 codepoint, U64 *result_index);
internal B32 str8_find_substr8(Str8 string, Str8 substring, U64 *result_index);

internal Void str8_list_push_explicit(Str8List *list, Str8 string, Str8Node *node);
internal Void str8_list_push(Arena *arena, Str8List *list, Str8 string);
internal Void str8_list_pushf(Arena *arena, Str8List *list, CStr fmt, ...);
internal Str8 str8_join(Arena *arena, Str8List *list);

internal Str8List str8_split_by_codepoints(Arena *arena, Str8 string, Str8 codepoints);

internal StringDecode string_decode_utf8(U8 *string, U64 size);
internal U64          string_encode_utf8(U8 *destination, U32 codepoint);
internal StringDecode string_decode_utf16(U16 *string, U64 size);
internal U64          string_encode_utf16(U16 *destination, U32 codepoint);

internal Str32 str32_from_str8(Arena *arena, Str8  string);
internal Str8  str8_from_str32(Arena *arena, Str32 string);
internal Str16 str16_from_str8(Arena *arena, Str8  string);
internal Str8  str8_from_str16(Arena *arena, Str16 string);
internal Str8  str8_from_cstr16(Arena *arena, CStr16 string);

internal CStr   cstr_from_str8(Arena *arena, Str8 string);
internal CStr16 cstr16_from_str8(Arena *arena, Str8 string);

internal U64 u64_from_str8(Str8 string, U64 *destination);
internal U64 u32_from_str8(Str8 string, U32 *destination);
internal U64 u16_from_str8(Str8 string, U16 *destination);
internal U64 u8_from_str8(Str8 string, U8 *destination);

internal U64 s64_from_str8(Str8 string, S64 *destination);
internal U64 s32_from_str8(Str8 string, S32 *destination);
internal U64 s16_from_str8(Str8 string, S16 *destination);
internal U64 s8_from_str8(Str8 string, S8 *destination);

internal U64 f64_from_str8(Str8 string, F64 *destination);

internal B32 is_num(U8 ch);

#endif // BASE_STRING_H
