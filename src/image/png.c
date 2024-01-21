internal U32
png_component_count_from_color_type(PNG_ColorType type)
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
png_make_huffman(Arena *arena, U32 count, U32 *lengths)
{
	PNG_Huffman result = { 0 };
	result.codes   = push_array(arena, U32, count);
	result.lengths = push_array(arena, U32, count);
	result.values  = push_array(arena, U32, count);
	result.count   = count;

	U32 bit_length_count[PNG_HUFFMAN_MAX_BITS + 1] = { 0 };
	for (U32 i = 0; i < count; ++i)
	{
		assert(lengths[i] <= PNG_HUFFMAN_MAX_BITS);
		++bit_length_count[lengths[i]];
	}

	U32 next_code_for_length[PNG_HUFFMAN_MAX_BITS + 1]   = { 0 };
	U32 first_index_for_length[PNG_HUFFMAN_MAX_BITS + 1] = { 0 };

	U32 first_code_for_length = 0;
	bit_length_count[0] = 0;
	for (U32 bits = 1; bits <= PNG_HUFFMAN_MAX_BITS; ++bits)
	{
		first_code_for_length        = (first_code_for_length + bit_length_count[bits - 1]) << 1;
		next_code_for_length[bits]   = first_code_for_length;
		first_index_for_length[bits] = first_index_for_length[bits - 1] + bit_length_count[bits - 1];
	}

	for (U32 i = 0; i < count; ++i)
	{
		U32 length = lengths[i];
		if (length != 0)
		{
			U32 index = first_index_for_length[length]++;
			// NOTE(simon): The codes here have the reverse bit order of what we need.
			U32 code  = next_code_for_length[length]++;
			result.codes[index]   = u32_reverse(code) >> (32 - length);
			result.values[index]  = i;
			result.lengths[index] = length;
		}
	}

	return(result);
}

internal U32
png_read_huffman(PNG_State *state, PNG_Huffman *huffman)
{
	for (U32 i = 0; i < huffman->count; ++i)
	{
		U32 bits = (U32) png_peek_bits(state, huffman->lengths[i]);
		if (huffman->lengths[i] && bits == huffman->codes[i])
		{
			png_consume_bits(state, huffman->lengths[i]);
			U32 result = huffman->values[i];
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
			case PNG_TYPE('P', 'L', 'T', 'E'):
			{
				if (state->palette)
				{
					log_error("Duplicate 'PLTE' chunk, corrupted PNG");
					return(false);
				}

				if (!seen_ihdr)
				{
					log_error("Expected 'IHDR' chunk but got 'PLTE'");
					return(false);
				}

				if (state->first_idat)
				{
					log_error("'PLTE' chunk must be before the first 'IDAT' chunk, corrupted PNG");
					return(false);
				}

				if (state->color_type == PNG_ColorType_Greyscale || state->color_type == PNG_ColorType_GreyscaleAlpha)
				{
					log_error("'PLTE' chunk should not be present when the color type is greyscale or greyscale + alpha, corrupted PNG");
					return(false);
				}

				if (chunk.data.size == 0)
				{
					log_error("'PLTE' chunk must contain at least one palette entry, corrupted PNG");
					return(false);
				}

				if (chunk.data.size % 3 != 0)
				{
					log_error("'PLTE' chunk size is not a multiple of 3, corrupted PNG");
					return(false);
				}

				if (chunk.data.size / 3 > (1llu << state->bit_depth))
				{
					log_error("'PLTE' chunk has more entries than can be represented with the specified bit depth, corrupted PNG");
					return(false);
				}

				state->palette      = chunk.data.data;
				state->palette_size = (U32) (chunk.data.size / 3);
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
png_zlib_decode_lengths(PNG_State *state, PNG_Huffman *huffman, U32 *result_lengths, U32 count)
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
png_zlib_decode_huffman_block(PNG_State *state, PNG_Huffman *literal_huffman, PNG_Huffman *distance_huffman)
{
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
				log_error("Attempt to reference data that is outside of the ZLIB stream, corrupted PNG");
				return(false);
			}

			if (length > state->zlib_opl - state->zlib_ptr)
			{
				log_error("ZLIB Huffman compressed block contains too much data, corrupted PNG");
				return(false);
			}

			if (length < distance)
			{
				// NOTE(simon): None-overlapping regions.
				memory_copy(state->zlib_ptr, state->zlib_ptr - distance, length);
				state->zlib_ptr += length;
			}
			else if (distance == 1)
			{
				// NOTE(simon): Common if filtering can generate a lot of repeated values.
				memset(state->zlib_ptr, state->zlib_ptr[-1], length);
				state->zlib_ptr += length;
			}
			else
			{
				// NOTE(simon): memory_copy will not work as the two regions overlap.
				U8 *write = state->zlib_ptr;
				U8 *read = state->zlib_ptr - distance;
				for (U32 i = 0; i < length; ++i)
				{
					*write++ = *read++;
				}
				state->zlib_ptr += length;
			}
		}
		else if (literal > 285)
		{
			log_error("ZLIB Huffman compressed block contains unknown literal value, corrupted PNG");
			return(false);
		}
		else
		{
			log_error("ZLIB Huffman compressed block contains too much data, corruped PNG");
			return(false);
		}
	}

	// NOTE(simon): Because there is a check sum at the end of the ZLIB stream,
	// the data is corrupted if we have reached the end by this point.
	if (png_is_end(state))
	{
		//log_error("Not enough data in ZLIB stream, corrupted PNG");
		//return(false);
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
			if (!png_zlib_decode_huffman_block(state, &state->fixed_literal_huffman, &state->fixed_distance_huffman))
			{
				return(false);
			}
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

			PNG_Huffman code_length_huffman = png_make_huffman(scratch.arena, array_count(png_code_length_reorder), code_length_lengths);

			// NOTE(simon): Lengths for literals and distances are compressed together.
			U32 *lengths          = push_array(scratch.arena, U32, hlit + hdist);
			U32 *literal_lengths  = lengths;
			U32 *distance_lengths = lengths + hlit;

			if (!png_zlib_decode_lengths(state, &code_length_huffman, lengths, hlit + hdist))
			{
				release_scratch(scratch);
				return(false);
			}

			PNG_Huffman literal_huffman  = png_make_huffman(scratch.arena, hlit,  literal_lengths);
			PNG_Huffman distance_huffman = png_make_huffman(scratch.arena, hdist, distance_lengths);

			if (!png_zlib_decode_huffman_block(state, &literal_huffman, &distance_huffman))
			{
				release_scratch(scratch);
				return(false);
			}

			release_scratch(scratch);
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

internal B32
png_unfilter(PNG_State *state)
{
	U32 *row_offsets     = png_interlace_row_offsets[state->interlace_method];
	U32 *row_advances    = png_interlace_row_advances[state->interlace_method];
	U32 *column_offsets  = png_interlace_column_offsets[state->interlace_method];
	U32 *column_advances = png_interlace_column_advances[state->interlace_method];
	U32  pass_count      = png_interlace_pass_count[state->interlace_method];

	U32 components = png_component_count_from_color_type(state->color_type);
	U32 bit_stride = u32_round_up_to_power_of_2(state->bit_depth, 8) * components;

	U8 *scanline = state->zlib_output;

	for (U32 pass_index = 0; pass_index < pass_count; ++pass_index)
	{
		if (column_offsets[pass_index] >= state->width)
		{
			continue;
		}

		U32 scanline_length = (state->width - column_offsets[pass_index] + column_advances[pass_index] - 1) / column_advances[pass_index];
		U32 scanline_size   = u32_round_up_to_power_of_2(state->bit_depth * components * scanline_length, 8) / 8;
		U32 byte_stride     = u32_round_up_to_power_of_2(state->bit_depth, 8) * components / 8;

		for (U32 y = row_offsets[pass_index]; y < state->height; y += row_advances[pass_index])
		{
			U8 filter_type = scanline[0];

			if (filter_type == 0 || (filter_type == 2 && y == row_offsets[pass_index]))
			{
				// NOTE(simon): No filter and up filter at y == 0, nothing to do.
			}
			else if (filter_type == 1)
			{
				// NOTE(simon): Sub filter
				for (U32 x = byte_stride; x < scanline_size; ++x)
				{
					U8 a = scanline[1 + x - byte_stride];

					scanline[1 + x] += a;
				}
			}
			else if (filter_type == 2)
			{
				// NOTE(simon): Up filter at y != 0
				U8 *previous_scanline = scanline - scanline_size - 1;
				for (U32 x = 0; x < scanline_size; ++x)
				{
					U8 b = previous_scanline[1 + x];

					scanline[1 + x] += b;
				}
			}
			else if (filter_type == 3 && y == row_offsets[pass_index])
			{
				// NOTE(simon): Average filter at y == 0
				for (U32 x = byte_stride; x < scanline_size; ++x)
				{
					U8 a = scanline[1 + x - byte_stride];
					U8 b = 0;

					scanline[1 + x] += (U8) ((a + b) / 2);
				}
			}
			else if (filter_type == 3 && y != row_offsets[pass_index])
			{
				// NOTE(simon): Average filter
				U8 *previous_scanline = scanline - scanline_size - 1;
				for (U32 x = 0; x < scanline_size; ++x)
				{
					U8 a = (x < byte_stride ? 0 : scanline[1 + x - byte_stride]);
					U8 b = previous_scanline[1 + x];

					scanline[1 + x] += (U8) ((a + b) / 2);
				}
			}
			else if (filter_type == 4 && y == row_offsets[pass_index])
			{
				// NOTE(simon): Paeth filter
				for (U32 x = 0; x < scanline_size; ++x)
				{
					U8 a = (x < byte_stride ? 0 : scanline[1 + x - byte_stride]);
					U8 b = 0;
					U8 c = 0;

					S32 p = (S32) a + (S32) b - (S32) c;
					S32 pa = s32_abs(p - (S32) a);
					S32 pb = s32_abs(p - (S32) b);
					S32 pc = s32_abs(p - (S32) c);

					U8 pr = 0;
					if (pa <= pb && pa <= pc)
					{
						pr = a;
					}
					else if (pb <= pc)
					{
						pr = b;
					}
					else
					{
						pr = c;
					}

					scanline[1 + x] += pr;
				}
			}
			else if (filter_type == 4 && y != row_offsets[pass_index])
			{
				// NOTE(simon): Paeth filter
				U8 *previous_scanline = scanline - scanline_size - 1;
				for (U32 x = 0; x < scanline_size; ++x)
				{
					U8 a = (x < byte_stride ? 0 : scanline[1 + x - byte_stride]);
					U8 b = previous_scanline[1 + x];
					U8 c = (x < byte_stride ? 0 : previous_scanline[1 + x - byte_stride]);

					S32 p = (S32) a + (S32) b - (S32) c;
					S32 pa = s32_abs(p - (S32) a);
					S32 pb = s32_abs(p - (S32) b);
					S32 pc = s32_abs(p - (S32) c);

					U8 pr = 0;
					if (pa <= pb && pa <= pc)
					{
						pr = a;
					}
					else if (pb <= pc)
					{
						pr = b;
					}
					else
					{
						pr = c;
					}

					scanline[1 + x] += pr;
				}
			}
			else
			{
				log_error("Unknown filter (%"PRIU8"), corrupted PNG", filter_type);
				return(false);
			}

			scanline += scanline_size + 1;
		}
	}

	return(true);
}

internal B32
png_deinterlace_and_resample_to_8bit(PNG_State *state, U8 *pixels, U8 *output)
{
	U32 *row_offsets     = png_interlace_row_offsets[state->interlace_method];
	U32 *row_advances    = png_interlace_row_advances[state->interlace_method];
	U32 *column_offsets  = png_interlace_column_offsets[state->interlace_method];
	U32 *column_advances = png_interlace_column_advances[state->interlace_method];
	U32  pass_count      = png_interlace_pass_count[state->interlace_method];

	U32 components = png_component_count_from_color_type(state->color_type);
	if (state->color_type == PNG_ColorType_IndexedColor && state->bit_depth != 8)
	{
		U8 *input = pixels;

		// NOTE(simon): There can only ever be one component per pixel when the bit-depth is below 8.
		for (U32 pass_index = 0; pass_index < pass_count; ++pass_index)
		{
			for (U32 y = row_offsets[pass_index]; y < state->height; y += row_advances[pass_index])
			{
				U32 scanline_length = (state->width - column_offsets[pass_index] + column_advances[pass_index] - 1) / column_advances[pass_index];
				U32 scanline_size   = u32_round_up_to_power_of_2(state->bit_depth * png_component_count_from_color_type(state->color_type) * scanline_length, 8) / 8;
				U64 bit_position    = 0;

				for (U32 x = column_offsets[pass_index]; x < state->width; x += column_advances[pass_index])
				{
					U64 byte_index   = bit_position / 8;
					U64 bit_index    = bit_position % 8;
					U8 byte          = input[byte_index];
					U8 value         = (U8) ((U8) (byte << bit_index) >> (8 - state->bit_depth));

					output[(x + y * state->width) * components] = value;

					bit_position += state->bit_depth;
				}

				input += scanline_size;
			}
		}
	}
	else if (state->bit_depth == 16)
	{
		U16 *input = (U16 *) pixels;
		for (U32 pass_index = 0; pass_index < pass_count; ++pass_index)
		{
			for (U32 y = row_offsets[pass_index]; y < state->height; y += row_advances[pass_index])
			{
				for (U32 x = column_offsets[pass_index]; x < state->width; x += column_advances[pass_index])
				{
					for (U32 i = 0; i < components; ++i)
					{
						U16 value = u16_big_to_local_endian(*input++);
						output[(x + y * state->width) * components + i] = (U8) (((U32) value * (U32) U8_MAX + (U32) U16_MAX / 2) / (U32) U16_MAX);
					}
				}
			}
		}
	}
	else if (state->bit_depth != 8)
	{
		U8 *input = pixels;
		U32 bit_max = (1U << state->bit_depth) - 1;

		// NOTE(simon): There can only ever be one component per pixel when the bit-depth is below 8.
		for (U32 pass_index = 0; pass_index < pass_count; ++pass_index)
		{
			for (U32 y = row_offsets[pass_index]; y < state->height; y += row_advances[pass_index])
			{
				U32 scanline_length = (state->width - column_offsets[pass_index] + column_advances[pass_index] - 1) / column_advances[pass_index];
				U32 scanline_size   = u32_round_up_to_power_of_2(state->bit_depth * png_component_count_from_color_type(state->color_type) * scanline_length, 8) / 8;
				U64 bit_position    = 0;

				for (U32 x = column_offsets[pass_index]; x < state->width; x += column_advances[pass_index])
				{
					U64 byte_index   = bit_position / 8;
					U64 bit_index    = bit_position % 8;
					U8 byte          = input[byte_index];
					U8 value         = (U8) ((U8) (byte << bit_index) >> (8 - state->bit_depth));

					output[(x + y * state->width) * components] = (U8) (((U32) value * (U32) U8_MAX + bit_max / 2) / bit_max);

					bit_position += state->bit_depth;
				}

				input += scanline_size;
			}
		}
	}
	else
	{
		U8 *input = pixels;
		for (U32 pass_index = 0; pass_index < pass_count; ++pass_index)
		{
			if (column_offsets[pass_index] >= state->width)
			{
				continue;
			}

			for (U32 y = row_offsets[pass_index]; y < state->height; y += row_advances[pass_index])
			{
				++input;
				for (U32 x = column_offsets[pass_index]; x < state->width; x += column_advances[pass_index])
				{
					memory_copy(&output[(x + y * state->width) * components], input, components);
					input += components;
				}
			}
		}
	}

	return(true);
}

internal B32
png_expand_to_rgba(PNG_State *state, U8 *pixels)
{
	U32 components = png_component_count_from_color_type(state->color_type);
	U8 *read  = &pixels[(state->width * state->height - 1) * components];
	U8 *write = &pixels[(state->width * state->height - 1) * 4];

	switch (state->color_type)
	{
		case PNG_ColorType_Greyscale:
		{
			for (U64 i = 0; i < (U64) state->width * (U64) state->height; ++i)
			{
				write[3] = 0xFF;
				write[2] = read[0];
				write[1] = read[0];
				write[0] = read[0];

				read  -= components;
				write -= 4;
			}
		} break;
		case PNG_ColorType_Truecolor:
		{
			for (U64 i = 0; i < (U64) state->width * (U64) state->height; ++i)
			{
				write[3] = 0xFF;
				write[2] = read[2];
				write[1] = read[1];
				write[0] = read[0];

				read  -= components;
				write -= 4;
			}
		} break;
		case PNG_ColorType_IndexedColor:
		{
			for (U64 i = 0; i < (U64) state->width * (U64) state->height; ++i)
			{
				// TODO(simon): Try allowing you to always index in to the
				// palette and check at the end of the loop. Profile!
				U32 index = *read;
				if (index >= state->palette_size)
				{
					log_error("Attempt to refernce a palette entry that does not exist, corrupted PNG");
					return(false);
				}

				write[3] = 0xFF;
				write[2] = state->palette[index * 3 + 2];
				write[1] = state->palette[index * 3 + 1];
				write[0] = state->palette[index * 3 + 0];

				read  -= components;
				write -= 4;
			}
		} break;
		case PNG_ColorType_GreyscaleAlpha:
		{
			for (U64 i = 0; i < (U64) state->width * (U64) state->height; ++i)
			{
				write[3] = read[1];
				write[2] = read[0];
				write[1] = read[0];
				write[0] = read[0];

				read  -= components;
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
png_load(Arena *arena, Str8 contents, Image *result)
{
	U64 before = os_now_nanoseconds();
	Arena_Temporary scratch = get_scratch(&arena, 1);

	PNG_State state = { 0 };
	if (!png_parse_chunks(scratch.arena, contents, &state))
	{
		release_scratch(scratch);
		return(false);
	}

	if (state.color_type == PNG_ColorType_IndexedColor && !state.palette)
	{
		log_error("Using indexed color without a 'PLTE' chunk, corrupted PNG");
		release_scratch(scratch);
		return(false);
	}

	state.current_node = state.first_idat;

	U32 scanline_count = 0;
	for (U32 pass_index = 0; pass_index < png_interlace_pass_count[state.interlace_method]; ++pass_index)
	{
		U32 *row_offsets  = png_interlace_row_offsets[state.interlace_method];
		U32 *row_advances = png_interlace_row_advances[state.interlace_method];
		scanline_count += (state.height - row_offsets[pass_index] + row_advances[pass_index] - 1) / row_advances[pass_index];
	}
	U64 inflated_size = 4 * sizeof(U16) * state.width * state.height + scanline_count;
	state.zlib_output = push_array(scratch.arena, U8, inflated_size);
	state.zlib_ptr = state.zlib_output;
	state.zlib_opl = state.zlib_ptr + inflated_size;

	U32 fixed_lengths[288];
	for (U32 i = 0; i <= 143; ++i)
	{
		fixed_lengths[i] = 8;
	}
	for (U32 i = 144; i <= 255; ++i)
	{
		fixed_lengths[i] = 9;
	}
	for (U32 i = 256; i <= 279; ++i)
	{
		fixed_lengths[i] = 7;
	}
	for (U32 i = 280; i <= 287; ++i)
	{
		fixed_lengths[i] = 8;
	}
	state.fixed_literal_huffman = png_make_huffman(scratch.arena, 288, fixed_lengths);

	U32 fixed_distances[32];
	for (U32 i = 0; i <= 31; ++i)
	{
		fixed_distances[i] = 5;
	}
	state.fixed_distance_huffman = png_make_huffman(scratch.arena, 32, fixed_distances);

	if (!png_zlib_inflate(&state))
	{
		release_scratch(scratch);
		return(false);
	}

	if (!png_unfilter(&state))
	{
		release_scratch(scratch);
		return(false);
	}

	Arena_Temporary restore_point = arena_begin_temporary(arena);
	U8 *output = push_array(arena, U8, state.width * state.height * 4);
	if (!png_deinterlace_and_resample_to_8bit(&state, state.zlib_output, output))
	{
		arena_end_temporary(restore_point);
		release_scratch(scratch);
		return(false);
	}

	if (!png_expand_to_rgba(&state, output))
	{
		arena_end_temporary(restore_point);
		release_scratch(scratch);
		return(false);
	}

	U64 after = os_now_nanoseconds();
	log_info("Our: %.2fms to load PNG", (F64) (after - before) / (F64) million(1));

	result->width  = state.width;
	result->height = state.height;
	result->pixels = output;
	result->color_space = Render_ColorSpace_sRGB;

	release_scratch(scratch);
	return(true);
}
