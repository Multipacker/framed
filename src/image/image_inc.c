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
#define PNG_TYPE(tag) (((tag)[0]) << 0 | ((tag)[1]) << 8 | ((tag)[2]) << 16 | ((tag)[3]) << 24)
#define PNG_CHUNK_IHDR_SIZE 13
#define PNG_CHUNK_TYPE_TO_CSTR(type) ((char[5]) { \
		(char) ((type) >>  0),                    \
		(char) ((type) >>  8),                    \
		(char) ((type) >> 16),                    \
		(char) ((type) >> 24),                    \
		0,                                        \
	})
#define PNG_TYPE_INVALID 0

global U8 png_magic[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, };

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

		assert(length < S32_MAX);

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
			chunk.type = 0;
			log_error("Not enough data for chunk");
		}
	}
	else
	{
		log_error("Not enough data for chunk");
	}

	return(chunk);
}

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
	assert(0 <= count && count <= 57);
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
			case PNG_TYPE("IHDR"):
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

				if (!(0 < width && width <= S32_MAX))
				{
					log_error("Width is outside of the permitted range");
					return(false);
				}

				if (!(0 < height && height <= S32_MAX))
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
			case PNG_TYPE("IDAT"):
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
			case PNG_TYPE("IEND"):
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
				log_info("ZLIB Uncompressed block length is corrupted, corrupted PNG");
				return(false);
			}

			// TODO(simon): Check for end of input buffer.

			if (length > state->zlib_opl - state->zlib_ptr)
			{
				log_info("ZLIB Uncompressed block contains too much data, corruped PNG");
				return(false);
			}

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
			// NOTE(simon): Dynamic Huffman codes
			log_error("Dynamic Huffman codes are not supported yet!");
			return(false);
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
	for (U32 y = 0; y < state.height; ++y)
	{
		U8 *row = state.zlib_output + y * (1 + state.width * 4);
		U8 filter_type = *row++;
		if (filter_type == 0 || (filter_type == 2 && y == 0))
		{
			// NOTE(simon): No filtering and up filtering at y == 0
			memory_copy(&unfiltered_data[y * state.width * 4], row, state.width * 4);
		}
		else if (filter_type == 1)
		{
			// NOTE(simon): Sub filter
			U64 previous = 0;
			for (U32 x = 0; x < state.width * 4; ++x)
			{
				// TODO(simon): The amount to shift down by depends on bit_depth and color_type.
				U8 new_value = row[x] + (U8) (previous >> 24);
				unfiltered_data[x + y * state.width * 4] = new_value;
				previous = previous << 8 | new_value;
			}
		}
		else if (filter_type == 2)
		{
			// NOTE(simon): Up filter at y != 0
			U8 *previous_scanline = &unfiltered_data[(y - 1) * state.width * 4];
			for (U32 x = 0; x < state.width * 4; ++x)
			{
				U8 new_value = row[x] + *previous_scanline;
				unfiltered_data[x + y * state.width * 4] = new_value;
				++previous_scanline;
			}
		}
		else if (filter_type == 3)
		{
			// NOTE(simon): Average filter
			U8 zero = 0;
			U8 *previous_scanline = (y == 0 ? &zero : &unfiltered_data[(y - 1) * state.width * 4]);
			U64 previous = 0;
			for (U32 x = 0; x < state.width * 4; ++x)
			{
				// TODO(simon): The amount to shift down by depends on bit_depth and color_type.
				U8 previous_value = (U8) (previous >> 24);
				U8 new_value = row[x] + (previous_value + *previous_scanline) / 2;
				unfiltered_data[x + y * state.width * 4] = new_value;
				previous_scanline += (y != 0);
				previous = previous << 8 | new_value;
			}
		}
		else if (filter_type == 4)
		{
			// NOTE(simon): Paeth filter
			U8 zero = 0;
			U8 *previous_scanline = (y == 0 ? &zero : &unfiltered_data[(y - 1) * state.width * 4]);
			U64 previous_scanline_previous = 0;
			U64 previous = 0;
			for (U32 x = 0; x < state.width * 4; ++x)
			{
				// TODO(simon): The amount to shift down by depends on bit_depth and color_type.
				U8 previous_value = (U8) (previous >> 24);
				U8 previous_scanline_previous_value = (U8) (previous_scanline_previous >> 24);
				U8 new_value = row[x] + png_paeth_predictor(previous_value, *previous_scanline, previous_scanline_previous_value);
				unfiltered_data[x + y * state.width * 4] = new_value;

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

	R_Texture texture = render_create_texture_from_bitmap(renderer, unfiltered_data, state.width, state.height, R_ColorSpace_sRGB);
	*texture_result = render_slice_from_texture(texture, rectf32(v2f32(0, 0), v2f32(1, 1)));

	return(true);
}
