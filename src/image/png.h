// NOTE(simon): Specifications used:
//   * [PNG](https://www.w3.org/TR/png-3/)
//   * [ZLIB](https://www.rfc-editor.org/rfc/rfc1950)
//   * [DEFLATE](https://www.rfc-editor.org/rfc/rfc1951)

#ifndef PNG_H
#define PNG_H

#define PNG_CHUNK_MIN_SIZE (4 + 4 + 4)
#define PNG_CHUNK_ANCILLARY_BIT (0x20 <<  0)
#define PNG_CHUNK_PRIVATE_BIT   (0x20 <<  8)
#define PNG_CHUNK_RESERVED_BIT  (0x20 << 16)
// NOTE(simon): We don't care about this flag.
//#define PNG_CHUNK_ANCILLARY_BIT (0x20 << 24)
#define PNG_TYPE(c0, c1, c2, c3) ((c0) << 0 | (c1) << 8 | (c2) << 16 | (c3) << 24)
#define PNG_CHUNK_IHDR_SIZE 13
#define PNG_CHUNK_TYPE_TO_CSTR(type) ((char[5]) { \
        (char) ((type) >>  0),                    \
        (char) ((type) >>  8),                    \
        (char) ((type) >> 16),                    \
        (char) ((type) >> 24),                    \
        0,                                        \
    })
#define PNG_TYPE_INVALID 0

#define PNG_HUFFMAN_MAX_BITS 15
#define PNG_HUFFMAN_FAST_BITS 9
#define PNG_HUFFMAN_FAST_COUNT (1 << PNG_HUFFMAN_FAST_BITS)

global U8 png_magic[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, };

global U32 png_code_length_reorder[] = {
    16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
    11, 4,  12, 3, 13, 2, 14, 1, 15
};

// TODO(simon): Maybe we should store dummy value until index 257. Profile!
// NOTE(simon): Base index is 257
global U32 png_length_extra_bits[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4,
    5, 5, 5, 5, 0,
};

// TODO(simon): Maybe we should store dummy value until index 257. Profile!
// NOTE(simon): Base index is 257
global U32 png_length_base[] = {
    3,   4,   5,   6,   7,  8,  9,  10,
    11,  13,  15,  17,  19, 23, 27, 31,
    35,  43,  51,  59,  67, 83, 99, 115,
    131, 163, 195, 227, 258,
};

global U32 png_distance_extra_bits[] = {
    0,  0,  0,  0,  1,  1, 2,  2,
    3,  3,  4,  4,  5,  5, 6,  6,
    7,  7,  8,  8,  9,  9, 10, 10,
    11, 11, 12, 12, 13, 13,
};

global U32 png_distance_base[] = {
    1,    2,    3,    4,     5,     7,    9,    13,
    17,   25,   33,   49,    65,    97,   129,  193,
    257,  385,  513,  769,   1025,  1537, 2049, 3073,
    4097, 6145, 8193, 12289, 16385, 24577,
};

global U32 png_interlace_null_row_offsets[]     = { 0, };
global U32 png_interlace_null_row_advances[]    = { 1, };
global U32 png_interlace_null_column_offsets[]  = { 0, };
global U32 png_interlace_null_column_advances[] = { 1, };

global U32 png_interlace_adam7_row_offsets[]     = { 0, 0, 4, 0, 2, 0, 1, };
global U32 png_interlace_adam7_row_advances[]    = { 8, 8, 8, 4, 4, 2, 2, };
global U32 png_interlace_adam7_column_offsets[]  = { 0, 4, 0, 2, 0, 1, 0, };
global U32 png_interlace_adam7_column_advances[] = { 8, 8, 4, 4, 2, 2, 1, };

global U32 *png_interlace_row_offsets[]     = { png_interlace_null_row_offsets,     png_interlace_adam7_row_offsets, };
global U32 *png_interlace_row_advances[]    = { png_interlace_null_row_advances,    png_interlace_adam7_row_advances, };
global U32 *png_interlace_column_offsets[]  = { png_interlace_null_column_offsets,  png_interlace_adam7_column_offsets, };
global U32 *png_interlace_column_advances[] = { png_interlace_null_column_advances, png_interlace_adam7_column_advances, };
global U32  png_interlace_pass_count[]      = { 1, 7, };

typedef struct PNG_Chunk PNG_Chunk;
struct PNG_Chunk
{
    U32 type;
    Str8 data;
    U32 crc;
};

typedef enum PNG_ColorType PNG_ColorType;
enum PNG_ColorType
{
    PNG_ColorType_Greyscale      = 0,
    PNG_ColorType_Truecolor      = 2,
    PNG_ColorType_IndexedColor   = 3,
    PNG_ColorType_GreyscaleAlpha = 4,
    PNG_ColorType_TruecolorAlpha = 6,
};

typedef enum PNG_InterlaceMethod PNG_InterlaceMethod;
enum PNG_InterlaceMethod
{
    PNG_InterlaceMethod_None  = 0,
    PNG_InterlaceMethod_Adam7 = 1,
};

typedef struct PNG_Huffman PNG_Huffman;
struct PNG_Huffman
{
    U16 fast[PNG_HUFFMAN_FAST_COUNT];

    U32 *codes;
    U32 *lengths;
    U32 *values;
    U32 count;
};

typedef struct PNG_IDATNode PNG_IDATNode;
struct PNG_IDATNode
{
    PNG_IDATNode *next;
    Str8 data;
};

typedef struct PNG_State PNG_State;
struct PNG_State
{
    U32 width;
    U32 height;
    PNG_IDATNode *first_idat;
    PNG_IDATNode *last_idat;
    PNG_ColorType color_type;
    U8 bit_depth;
    PNG_InterlaceMethod interlace_method;

    U8 *palette;
    U32 palette_size;

    // NOTE(simon): Bit level reading
    U64 bit_buffer;
    U32 bit_count;

    PNG_IDATNode *current_node;
    U64 current_offset;

    PNG_Huffman fixed_literal_huffman;
    PNG_Huffman fixed_distance_huffman;
    U8 *zlib_output;
    U8 *zlib_ptr;
    U8 *zlib_opl;
};

#endif // PNG_H
