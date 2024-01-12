// NOTE(simon): Specifications used:
//   * [PNG](https://www.w3.org/TR/png-3/#10Compression)
//   * [ZLIB](https://www.rfc-editor.org/rfc/rfc1950)
//   * [DEFLATE](https://www.rfc-editor.org/rfc/rfc1951)

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

global U8 png_magic[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, };

global U32 png_code_length_reorder[] = {
	16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
	11, 4,  12, 3, 13, 2, 14, 1, 15
};

global U32 png_linear_alphabet[] = {
	0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,
	16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
	32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
	48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
	64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
	80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
	96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
	128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
	144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
	160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
	176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
	192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
	208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
	224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
	256, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271,
	272, 273, 274, 275, 276, 277, 278, 279, 280, 281, 282, 283, 284, 285,
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
	PNG_InterlaceMethod_Adam7 = 2,
};

typedef struct PNG_Huffman PNG_Huffman;
struct PNG_Huffman
{
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

	// NOTE(simon): Bit level reading
	U64 bit_buffer;
	U32 bit_count;

	PNG_IDATNode *current_node;
	U64 current_offset;

	U8 *zlib_output;
	U8 *zlib_ptr;
	U8 *zlib_opl;
};

internal U32
png_stride_from_color_type(PNG_ColorType type)
{
	switch (type)
	{
		case PNG_ColorType_Greyscale:      return 1; break;
		case PNG_ColorType_Truecolor:      return 3; break;
		case PNG_ColorType_IndexedColor:   return 1; break;
		case PNG_ColorType_GreyscaleAlpha: return 2; break;
		case PNG_ColorType_TruecolorAlpha: return 4; break;
		invalid_case;
	}

	return 0;
}

internal U8
png_get_byte(PNG_State *state)
{
	U8 result = 0;

	while (state->current_node && state->current_offset >= state->current_node->data.size)
	{
		state->current_node = state->current_node->next;
		state->current_offset = 0;
	}

	if (state->current_node)
	{
		result = state->current_node->data.data[state->current_offset];
		++state->current_offset;
	}

	return(result);
}

// TODO(simon): This should register the bit position that is outside of the stream.
internal B32
png_is_end(PNG_State *state)
{
	B32 is_end = (state->current_node == 0);
	return(is_end);
}

// NOTE(simon): LSB order
internal Void
png_refill_bits(PNG_State *state, U32 count)
{
	assert(count <= 57);
	while (state->bit_count < count)
	{
		state->bit_buffer |= (U64) png_get_byte(state) << state->bit_count;
		state->bit_count += 8;
	}
}

internal U64
png_peek_bits(PNG_State *state, U32 count)
{
	assert(state->bit_count >= count);
	U64 result = state->bit_buffer & ((1ULL << count) - 1);
	return(result);
}

internal Void
png_consume_bits(PNG_State *state, U32 count)
{
	assert(state->bit_count >= count);
	state->bit_buffer >>= count;
	state->bit_count   -= count;
}

internal U64
png_get_bits_no_refill(PNG_State *state, U32 count)
{
	U64 result = png_peek_bits(state, count);
	png_consume_bits(state, count);
	return(result);
}

internal Void
png_align_to_byte(PNG_State *state)
{
	U32 bits_to_discard = state->bit_count & 0x07;
	png_consume_bits(state, bits_to_discard);
}

internal PNG_Huffman
png_make_huffman(Arena *arena, U32 count, U32 *lengths, U32 *alphabet)
{
	PNG_Huffman result = { 0 };
	result.codes   = push_array(arena, U32, count);
	result.lengths = lengths;
	result.values  = alphabet;
	result.count   = count;

	U32 bit_length_count[PNG_HUFFMAN_MAX_BITS + 1] = { 0 };
	for (U32 i = 0; i < count; ++i)
	{
		assert(lengths[i] <= PNG_HUFFMAN_MAX_BITS);
		++bit_length_count[lengths[i]];
	}

	U32 next_code_for_length[PNG_HUFFMAN_MAX_BITS + 1] = { 0 };

	U32 first_code_for_length = 0;
	bit_length_count[0] = 0;
	for (U32 bits = 1; bits <= PNG_HUFFMAN_MAX_BITS; ++bits)
	{
		first_code_for_length = (first_code_for_length + bit_length_count[bits - 1]) << 1;
		next_code_for_length[bits] = first_code_for_length;
	}

	for (U32 i = 0; i < count; ++i)
	{
		U32 length = lengths[i];
		if (length != 0)
		{
			// NOTE(simon): The codes here have the reverse bit order of what we need.
			U32 code = next_code_for_length[length]++;
			result.codes[i] = u32_reverse(code) >> (32 - length);
		}
	}

	return(result);
}

internal U32
png_read_huffman(PNG_State *state, PNG_Huffman huffman)
{
	for (U32 i = 0; i < huffman.count; ++i)
	{
		U32 bits = (U32) png_peek_bits(state, huffman.lengths[i]);
		if (huffman.lengths[i] && bits == huffman.codes[i])
		{
			png_consume_bits(state, huffman.lengths[i]);
			U32 result = huffman.values[i];
			return(result);
		}
	}

	log_error("Unable to decode Huffman code");
	return(U32_MAX);
}

internal Void
png_print_huffman(PNG_Huffman huffman)
{
	U32 max_value  = 0;
	U32 max_length = 0;
	for (U32 i = 0; i < huffman.count; ++i)
	{
		max_value  = u32_max(max_value,  huffman.values[i]);
		max_length = u32_max(max_length, huffman.lengths[i]);
	}

	U32 value_length  = (U32) f32_floor(f32_log((F32) max_value))  + 1;
	U32 length_length = (U32) f32_floor(f32_log((F32) max_length)) + 1;

	log_info("Huffman codes(Symbol, Length, Code):");
	for (U32 i = 0; i < huffman.count; ++i)
	{
		if (huffman.lengths[i] == 0)
		{
			log_info(" %*"PRIU32" %*"PRIU32, value_length, huffman.values[i], length_length, huffman.lengths[i]);
		}
		else
		{
			log_info(" %*"PRIU32" %*"PRIU32" %0*b", value_length, huffman.values[i], length_length, huffman.lengths[i], huffman.lengths[i], huffman.codes[i]);
		}
	}
}

internal PNG_Chunk
png_parse_chunk(Str8 *contents)
{
	PNG_Chunk chunk = { 0 };

	if (contents->size >= PNG_CHUNK_MIN_SIZE)
	{
		U32 length = u32_big_to_local_endian(*(U32 *) contents->data);
		*contents = str8_skip(*contents, sizeof(U32));

		chunk.type = *(U32 *) contents->data;
		*contents = str8_skip(*contents, sizeof(U32));

		// TODO(simon): Verify type is valid.

		if (length < (U32) S32_MAX)
		{
			if (contents->size >= length + sizeof(U32))
			{
				chunk.data = str8(contents->data, length);
				*contents = str8_skip(*contents, length);

				chunk.crc = u32_big_to_local_endian(*(U32 *) contents->data);
				*contents = str8_skip(*contents, sizeof(U32));

				// TODO(simon): Verify CRC.
			}
			else
			{
				chunk.type = PNG_TYPE_INVALID;
				log_error("Not enough data for chunk");
			}
		}
		else
		{
			chunk.type = PNG_TYPE_INVALID;
			log_error("Chunk has too much data");
		}
	}
	else
	{
		log_error("Not enough data for chunk");
	}

	return(chunk);
}

internal B32
png_parse_chunks(Arena *arena, Str8 contents, PNG_State *state)
{
	if (!(contents.size >= sizeof(png_magic) && memory_match(contents.data, png_magic, sizeof(png_magic))))
	{
		log_error("Invalid magic number");
		return(false);
	}

	contents = str8_skip(contents, sizeof(png_magic));

	B32 seen_ihdr = false;
	for (;;)
	{
		PNG_Chunk chunk = png_parse_chunk(&contents);
		switch (chunk.type)
		{
			case PNG_TYPE_INVALID:
			{
				// NOTE(simon): Either the chunk data was invalid or we
				// reached the end of the buffer enexpectedly. Both are
				// signs of data corruption.
				return(false);
			} break;
			case PNG_TYPE('I', 'H', 'D', 'R'):
			{
				if (seen_ihdr)
				{
					log_error("Duplicate 'IHDR' chunk, corrupted PNG");
					return(false);
				}

				if (chunk.data.size != PNG_CHUNK_IHDR_SIZE)
				{
					log_error("Not enough data for 'IHDR' chunk, corrupted PNG");
					return(false);
				}

				U32 width             = u32_big_to_local_endian(*(U32 *) &chunk.data.data[0]);
				U32 height            = u32_big_to_local_endian(*(U32 *) &chunk.data.data[4]);
				U8 bit_depth          = chunk.data.data[8];
				U8 color_type         = chunk.data.data[9];
				U8 compression_method = chunk.data.data[10];
				U8 filter_method      = chunk.data.data[11];
				U8 interlace_method   = chunk.data.data[12];

				if (!(0 < width && width <= (U32)S32_MAX))
				{
					log_error("Width is outside of the permitted range");
					return(false);
				}

				if (!(0 < height && height <= (U32)S32_MAX))
				{
					log_error("Height is outside of the permitted range");
					return(false);
				}

				switch (color_type)
				{
					case PNG_ColorType_Greyscale:
					{
						if (!(bit_depth == 1 || bit_depth == 2 || bit_depth == 4 || bit_depth == 8 || bit_depth == 16))
						{
							log_error("Incompatible color type (%"PRIU8") and bit depth (%"PRIU8"), corrupted PNG", color_type, bit_depth);
							return(false);
						}
					} break;
					case PNG_ColorType_IndexedColor:
					{
						if (!(bit_depth == 1 || bit_depth == 2 || bit_depth == 4 || bit_depth == 8))
						{
							log_error("Incompatible color type (%"PRIU8") and bit depth (%"PRIU8"), corrupted PNG", color_type, bit_depth);
							return(false);
						}
					} break;
					case PNG_ColorType_Truecolor:
					case PNG_ColorType_GreyscaleAlpha:
					case PNG_ColorType_TruecolorAlpha:
					{
						if (!(bit_depth == 8 || bit_depth == 16))
						{
							log_error("Incompatible color type (%"PRIU8") and bit depth (%"PRIU8"), corrupted PNG", color_type, bit_depth);
							return(false);
						}
					} break;
					default:
					{
						log_error("Invalid color type (%"PRIU8"), corrupted PNG", color_type);
						return(false);
					} break;
				}

				if (compression_method != 0)
				{
					log_error("Invalid compression method (%"PRIU8"), corrupted PNG", compression_method);
					return(false);
				}

				if (filter_method != 0)
				{
					log_error("Invalid filter method (%"PRIU8"), corrupted PNG", filter_method);
					return(false);
				}

				if (!(interlace_method == PNG_InterlaceMethod_None || interlace_method == PNG_InterlaceMethod_Adam7))
				{
					log_error("Invalid interlace method (%"PRIU8"), corrupted PNG", interlace_method);
					return(false);
				}

				state->width            = width;
				state->height           = height;
				state->color_type       = color_type;
				state->bit_depth        = bit_depth;
				state->interlace_method = interlace_method;

				seen_ihdr = true;
			} break;
			case PNG_TYPE('I', 'D', 'A', 'T'):
			{
				if (!seen_ihdr)
				{
					log_error("Expected 'IHDR' chunk but got 'IDAT'");
					return(false);
				}

				PNG_IDATNode *node = push_struct(arena, PNG_IDATNode);
				node->data = chunk.data;

				queue_push(state->first_idat, state->last_idat, node);
			} break;
			case PNG_TYPE('I', 'E', 'N', 'D'):
			{
				if (!seen_ihdr)
				{
					log_error("Expected 'IHDR' chunk but got 'IEND'");
					return(false);
				}

				if (!state->first_idat)
				{
					log_error("No image data, corrupted PNG");
					return(false);
				}

				if (chunk.data.size != 0)
				{
					log_error("'IEND' chunk has data, corrupted PNG");
					return(false);
				}

				return(true);
			} break;
			default:
			{
				if (!seen_ihdr)
				{
					log_error("Expected 'IHDR' chunk but got 'IEND'");
					return(false);
				}

				if (!(chunk.type & PNG_CHUNK_ANCILLARY_BIT))
				{
					log_error("Unrecognized critical chunk '%s'", PNG_CHUNK_TYPE_TO_CSTR(chunk.type));
					return(false);
				}

				if (chunk.type & PNG_CHUNK_RESERVED_BIT)
				{
					log_warning("Chunk type '%s' is reserved", PNG_CHUNK_TYPE_TO_CSTR(chunk.type));
				}
			} break;
		}
	}
}

internal B32
png_zlib_decode_lengths(PNG_State *state, PNG_Huffman huffman, U32 *result_lengths, U32 count)
{
	U32 repeat_value = 0;
	U32 repeat_count = 0;
	for (U32 i = 0; i < count && !png_is_end(state); ++i)
	{
		if (repeat_count == 0)
		{
			png_refill_bits(state, PNG_HUFFMAN_MAX_BITS + 7);
			U32 operation = png_read_huffman(state, huffman);
			if (operation <= 15)
			{
				// NOTE(simon): Literal
				result_lengths[i] = operation;
			}
			else if (operation == 16 && i != 0)
			{
				repeat_count = (U32) png_get_bits_no_refill(state, 2) + 3;
				repeat_value = result_lengths[i - 1];
			}
			else if (operation == 17)
			{
				repeat_count = (U32) png_get_bits_no_refill(state, 3) + 3;
				repeat_value = 0;

			}
			else if (operation == 18)
			{
				repeat_count = (U32) png_get_bits_no_refill(state, 7) + 11;
				repeat_value = 0;
			}
			else if (operation > 18)
			{
				log_error("Unknown operation, corrupted PNG");
				return(false);
			}
			else
			{
				log_error("Attempt to copy a ZLIB length value when there is none, corrupted PNG");
				return(false);
			}
		}

		if (repeat_count)
		{
			result_lengths[i] = repeat_value;
			--repeat_count;
		}
	}

	// NOTE(simon): Because there is a check sum at the end of the ZLIB stream,
	// the data is corrupted if we have reached the end by this point.
	if (png_is_end(state))
	{
		log_error("Not enough data in ZLIB stream, corrupted PNG");
		return(false);
	}

	if (repeat_count)
	{
		log_error("Too much data, corrupted PNG");
		return(false);
	}

	return(true);
}

internal B32
png_zlib_inflate(PNG_State *state)
{
	png_refill_bits(state, 16);

	U16 cmf_flg = u16_big_to_local_endian((U16) png_peek_bits(state, 16));

	U8 compression_method = (U8) png_get_bits_no_refill(state, 4);
	U8 compression_info   = (U8) png_get_bits_no_refill(state, 4);

	U8 header_check       = (U8) png_get_bits_no_refill(state, 5);
	U8 preset_dictionary  = (U8) png_get_bits_no_refill(state, 1);
	U8 compression_level  = (U8) png_get_bits_no_refill(state, 2);

	if (png_is_end(state))
	{
		log_error("Unexpected end of ZLIB stream, missing header, corrupted PNG");
		return(false);
	}

	if (cmf_flg % 31 != 0)
	{
		log_error("ZLIB header check failed, corrupted PNG");
		return(false);
	}

	if (compression_method != 8)
	{
		log_error("Unknown compression method (%"PRIU8"), corrupted PNG", compression_method);
		return(false);
	}

	// NOTE(simon): This check is only valid when compression_method == 8.
	if (compression_info > 7)
	{
		log_error("Window size is too big (2^%"PRIU8"), corrupted PNG", compression_info + 8);
		return(false);
	}

	if (preset_dictionary)
	{
		log_error("ZLIB stream contains a preset dictionary, corrupted PNG");
		return(false);
	}

	B32 is_final_block = true;
	do
	{
		png_refill_bits(state, 3);
		is_final_block = (U8) png_get_bits_no_refill(state, 1);
		U8 block_type  = (U8) png_get_bits_no_refill(state, 2);

		if (block_type == 0)
		{
			// NOTE(simon): No compression.
			png_align_to_byte(state);
			png_refill_bits(state, 32);
			U32 length   = (U32) png_get_bits_no_refill(state, 16);
			U32 n_length = (U32) png_get_bits_no_refill(state, 16);

			if ((U16) ~length != (U16) n_length)
			{
				log_error("ZLIB Uncompressed block length is corrupted, corrupted PNG");
				return(false);
			}

			// TODO(simon): Check for end of input buffer.

			if (length > state->zlib_opl - state->zlib_ptr)
			{
				log_error("ZLIB Uncompressed block contains too much data, corruped PNG");
				return(false);
			}

			assert(state->bit_count % 8 == 0);
			while (state->bit_count)
			{
				*state->zlib_ptr++ = (U8) png_get_bits_no_refill(state, 8);
				--length;
			}

			for (U32 i = 0; i < length; ++i)
			{
				*state->zlib_ptr++ = png_get_byte(state);
			}
		}
		else if (block_type == 1)
		{
			// NOTE(simon): Fixed Huffman codes
			log_error("Fixed Huffman codes are not supported yet!");
			return(false);
		}
		else if (block_type == 2)
		{
			Arena_Temporary scratch = get_scratch(0, 0);

			// NOTE(simon): Dynamic Huffman codes
			png_refill_bits(state, 14);
			U32 hlit  = (U32) png_get_bits_no_refill(state, 5) + 257;
			U32 hdist = (U32) png_get_bits_no_refill(state, 5) + 1;
			U32 hclen = (U32) png_get_bits_no_refill(state, 4) + 4;

			// NOTE(simon): We can always refill enough bits.
			png_refill_bits(state, hclen * 3);
			U32 *code_length_lengths = push_array_zero(scratch.arena, U32, array_count(png_code_length_reorder));
			for (U32 i = 0; i < hclen; ++i)
			{
				code_length_lengths[png_code_length_reorder[i]] = (U32) png_get_bits_no_refill(state, 3);
			}

			PNG_Huffman code_length_huffman = png_make_huffman(scratch.arena, array_count(png_code_length_reorder), code_length_lengths, png_linear_alphabet);

			// NOTE(simon): Lengths for literals and distances are compressed together.
			U32 *lengths          = push_array(scratch.arena, U32, hlit + hdist);
			U32 *literal_lengths  = lengths;
			U32 *distance_lengths = lengths + hlit;

			if (!png_zlib_decode_lengths(state, code_length_huffman, lengths, hlit + hdist))
			{
				release_scratch(scratch);
				return(false);
			}

			PNG_Huffman literal_huffman  = png_make_huffman(scratch.arena, hlit,  literal_lengths,  png_linear_alphabet);
			PNG_Huffman distance_huffman = png_make_huffman(scratch.arena, hdist, distance_lengths, png_linear_alphabet);

			while (!png_is_end(state))
			{
				png_refill_bits(state, 2 * PNG_HUFFMAN_MAX_BITS + 5 + 13);
				U32 literal = png_read_huffman(state, literal_huffman);

				if (literal <= 255 && state->zlib_ptr < state->zlib_opl)
				{
					*state->zlib_ptr++ = (U8) literal;
				}
				else if (literal == 256)
				{
					// NOTE(simon): Normal exit
					break;
				}
				else if (literal <= 285)
				{
					U32 length = png_length_base[literal - 257] + (U32) png_get_bits_no_refill(state, png_length_extra_bits[literal - 257]);
					U32 distance_code = png_read_huffman(state, distance_huffman);
					U32 distance = png_distance_base[distance_code] + (U32) png_get_bits_no_refill(state, png_distance_extra_bits[distance_code]);

					// TODO(simon): Verify that no "unused" codes appear in the input stream.

					if (distance > state->zlib_ptr - state->zlib_output)
					{
						release_scratch(scratch);
						log_error("Attempt to reference data that is outside of the ZLIB stream, corrupted PNG");
						return(false);
					}

					if (length > state->zlib_opl - state->zlib_ptr)
					{
						release_scratch(scratch);
						log_error("ZLIB dynamic Huffman compressed block contains too much data, corrupted PNG");
						return(false);
					}

					// TODO(simon): Special case for when the regions don't overlap? Profile!
					// NOTE(simon): memory_copy will not work as the two regions might overlap.
					for (U32 i = 0; i < length; ++i)
					{
						*state->zlib_ptr = *(state->zlib_ptr - distance);
						++state->zlib_ptr;
					}
				}
				else
				{
					release_scratch(scratch);
					log_error("ZLIB dynamic Huffman compressed block contains too much data, corruped PNG");
					return(false);
				}
			}

			release_scratch(scratch);

			// NOTE(simon): Because there is a check sum at the end of the ZLIB stream,
			// the data is corrupted if we have reached the end by this point.
			if (png_is_end(state))
			{
				//log_error("Not enough data in ZLIB stream, corrupted PNG");
				//return(false);
			}
		}
		else
		{
			log_error("ZLIB invalid block type (%"PRIU8"), corrupted PNG", block_type);
			return(false);
		}
	} while (!is_final_block);

	// TODO(simon): Read adler-32 checksum

	return(true);
}

internal U8
png_paeth_predictor(S32 a, S32 b, S32 c)
{
	S32 p = a + b - c;
	S32 pa = s32_abs(p - a);
	S32 pb = s32_abs(p - b);
	S32 pc = s32_abs(p - c);

	U8 result = 0;
	if (pa <= pb && pa <= pc)
	{
		result = (U8) a;
	}
	else if (pb <= pc)
	{
		result = (U8) b;
	}
	else
	{
		result = (U8) c;
	}

	return(result);
}

internal B32
png_expand_to_rgba(PNG_State *state, U8 *pixels)
{
	// TODO(simon): Expand to RGBA
	U32 byte_stride = png_stride_from_color_type(state->color_type);
	U8 *read  = &pixels[(state->width * state->height - 1) * byte_stride];
	U8 *write = &pixels[(state->width * state->height - 1) * 4];

	switch (state->color_type)
	{
		case PNG_ColorType_Greyscale:
		{
			for (U64 i = 0; i < (U64) state->width * (U64) state->height; ++i)
			{
				write[0] = read[0];
				write[1] = read[0];
				write[2] = read[0];
				write[3] = 0xFF;

				read  -= byte_stride;
				write -= 4;
			}
		} break;
		case PNG_ColorType_Truecolor:
		{
			for (U64 i = 0; i < (U64) state->width * (U64) state->height; ++i)
			{
				write[0] = read[0];
				write[1] = read[1];
				write[2] = read[2];
				write[3] = 0xFF;

				read  -= byte_stride;
				write -= 4;
			}
		} break;
		case PNG_ColorType_IndexedColor:
		{
			// TODO(simon): Change to return Void when we support all formats.
			log_error("Indexed color is not yet supported");
			return(false);
		} break;
		case PNG_ColorType_GreyscaleAlpha:
		{
			for (U64 i = 0; i < (U64) state->width * (U64) state->height; ++i)
			{
				write[0] = read[0];
				write[1] = read[0];
				write[2] = read[0];
				write[3] = read[1];

				read  -= byte_stride;
				write -= 4;
			}
		} break;
		case PNG_ColorType_TruecolorAlpha:
		{
			// NOTE(simon): Nothing to do, already in the right format.
		} break;
	}

	return(true);
}

internal B32
image_load(Arena *arena, R_Context *renderer, Str8 contents, R_TextureSlice *texture_result)
{
	PNG_State state = { 0 };
	if (!png_parse_chunks(arena, contents, &state))
	{
		return(false);
	}

	state.current_node = state.first_idat;

	// TODO(simon): Better approximation for the inflated stream.
	U64 inflated_size = 4 * sizeof(U16) * state.width * state.height + state.height;
	state.zlib_output = push_array(arena, U8, inflated_size);
	state.zlib_ptr = state.zlib_output;
	state.zlib_opl = state.zlib_ptr + inflated_size;

	if (!png_zlib_inflate(&state))
	{
		return(false);
	}

	// TODO(simon): Better approximation for the unfiltered data
	U64 unfiltered_size = 4 * state.width * state.height;
	U8 *unfiltered_data = push_array(arena, U8, unfiltered_size);
	U32 bit_stride = u32_round_up_to_power_of_2(state.bit_depth, 8) * png_stride_from_color_type(state.color_type);
	U32 stride = bit_stride / 8 * state.width;
	for (U32 y = 0; y < state.height; ++y)
	{
		U8 *row = state.zlib_output + y * (1 + stride);
		U8 filter_type = *row++;
		if (filter_type == 0 || (filter_type == 2 && y == 0))
		{
			// NOTE(simon): No filtering and up filtering at y == 0
			memory_copy(&unfiltered_data[y * stride], row, stride);
		}
		else if (filter_type == 1)
		{
			// NOTE(simon): Sub filter
			U64 previous = 0;
			for (U32 x = 0; x < stride; ++x)
			{
				U8 new_value = row[x] + (U8) (previous >> (bit_stride - 8));
				unfiltered_data[x + y * stride] = new_value;
				previous = previous << 8 | new_value;
			}
		}
		else if (filter_type == 2)
		{
			// NOTE(simon): Up filter at y != 0
			U8 *previous_scanline = &unfiltered_data[(y - 1) * stride];
			for (U32 x = 0; x < stride; ++x)
			{
				U8 new_value = row[x] + *previous_scanline;
				unfiltered_data[x + y * stride] = new_value;
				++previous_scanline;
			}
		}
		else if (filter_type == 3)
		{
			// NOTE(simon): Average filter
			U8 zero = 0;
			U8 *previous_scanline = (y == 0 ? &zero : &unfiltered_data[(y - 1) * stride]);
			U64 previous = 0;
			for (U32 x = 0; x < stride; ++x)
			{
				U8 previous_value = (U8) (previous >> (bit_stride - 8));
				U8 new_value = (U8) (row[x] + (previous_value + *previous_scanline) / 2);
				unfiltered_data[x + y * stride] = new_value;
				previous_scanline += (y != 0);
				previous = previous << 8 | new_value;
			}
		}
		else if (filter_type == 4)
		{
			// NOTE(simon): Paeth filter
			U8 zero = 0;
			U8 *previous_scanline = (y == 0 ? &zero : &unfiltered_data[(y - 1) * stride]);
			U64 previous_scanline_previous = 0;
			U64 previous = 0;
			for (U32 x = 0; x < stride; ++x)
			{
				U8 previous_value = (U8) (previous >> (bit_stride - 8));
				U8 previous_scanline_previous_value = (U8) (previous_scanline_previous >> (bit_stride - 8));
				U8 new_value = row[x] + png_paeth_predictor(previous_value, *previous_scanline, previous_scanline_previous_value);
				unfiltered_data[x + y * stride] = new_value;

				previous_scanline_previous = previous_scanline_previous << 8 | *previous_scanline;
				previous_scanline += (y != 0);
				previous = previous << 8 | new_value;
			}
		}
		else
		{
			log_error("Unknown filter (%"PRIU8"), corrupted PNG", filter_type);
			return(false);
		}
	}

	if (state.color_type == PNG_ColorType_IndexedColor && state.bit_depth != 8)
	{
		// TODO(simon): Expand the indicies to 8-bit. Needs to be done starting from the end.
		log_error("Bit depth of %"PRIU8" is not supported yet", state.bit_depth);
		return(false);
	}
	else if (state.bit_depth != 8)
	{
		// TODO(simon): Resample data to be 8-bit. Need to be done starting from the end.
		log_error("Bit depth of %"PRIU8" is not supported yet", state.bit_depth);
		return(false);
	}

	if (!png_expand_to_rgba(&state, unfiltered_data))
	{
		return(false);
	}

	R_Texture texture = render_create_texture_from_bitmap(renderer, unfiltered_data, state.width, state.height, R_ColorSpace_sRGB);
	*texture_result = render_slice_from_texture(texture, rectf32(v2f32(0, 0), v2f32(1, 1)));

	return(true);
}
