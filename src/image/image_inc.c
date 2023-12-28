#define PNG_CHUNK_MIN_SIZE (4 + 4 + 4)
#define PNG_CHUNK_ANCILLARY_BIT (0x20 <<  0)
#define PNG_CHUNK_PRIVATE_BIT   (0x20 <<  8)
#define PNG_CHUNK_RESERVED_BIT  (0x20 << 16)
// NOTE(simon): We don't care about this flag.
//#define PNG_CHUNK_ANCILLARY_BIT (0x20 << 24)
#define PNG_TYPE(tag) (((tag)[0]) << 0 | ((tag)[1]) << 8 | ((tag)[2]) << 16 | ((tag)[3]) << 24)
#define PNG_CHUNK_IHDR_SIZE 13

global U8 png_magic[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, };

typedef struct PNG_Chunk PNG_Chunk;
struct PNG_Chunk
{
	U32 type;
	Str8 contents;
	U32 crc;
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

		assert(length < S32_MAX);

		if (contents->size >= length + sizeof(U32))
		{
			chunk.contents = str8(contents->data, length);
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

internal Void
image_maybe_log_unrecognized_chunk(PNG_Chunk chunk)
{
	if (!(chunk.type & PNG_CHUNK_ANCILLARY_BIT))
	{
		if (chunk.type & PNG_CHUNK_PRIVATE_BIT)
		{
			log_error("Unrecognized critical private chunk '%.4s'", (CStr) &chunk.type);
		}
		else
		{
			log_error("Unrecognized critical public chunk '%.4s'", (CStr) &chunk.type);
		}
	}

	if (chunk.type & PNG_CHUNK_RESERVED_BIT)
	{
		log_error("Chunk type '%.4s' is reserved", (CStr) &chunk.type);
	}
}

internal B32
png_is_valid_IHDR_chunk(PNG_Chunk chunk)
{
	if (chunk.type != PNG_TYPE("IHDR"))
	{
		log_error("Expected 'IHDR' chunk but got '%.4s'", (CStr) &chunk.type);
		return(false);
	}

	B32 success = true;

	if (chunk.contents.size == PNG_CHUNK_IHDR_SIZE)
	{
		U32 width             = u32_big_to_local_endian(*(U32 *) &chunk.contents.data[0]);
		U32 height            = u32_big_to_local_endian(*(U32 *) &chunk.contents.data[4]);
		U8 bit_depth          = chunk.contents.data[8];
		U8 color_type         = chunk.contents.data[9];
		U8 compression_method = chunk.contents.data[10];
		U8 filter_method      = chunk.contents.data[11];
		U8 interlace_method   = chunk.contents.data[12];

		if (!(0 < width && width <= S32_MAX))
		{
			log_error("Width is outside of the permitted range");
			success = false;
		}

		if (!(0 < height && height <= S32_MAX))
		{
			log_error("Height is outside of the permitted range");
			success = false;
		}
	}
	else
	{
		log_error("Not enough data for 'IHDR' chunk", (CStr) &chunk.type);
		success = false;
	}

	return(success);
}

internal Void
image_load(Arena *arena, Str8 contents)
{
	if (contents.size >= sizeof(png_magic) && memory_match(contents.data, png_magic, sizeof(png_magic)))
	{
		contents = str8_skip(contents, sizeof(png_magic));

		PNG_Chunk chunk = png_parse_chunk(&contents);
		if (png_is_valid_IHDR_chunk(chunk))
		{
			for (;;)
			{
				chunk = png_parse_chunk(&contents);

				if (chunk.type == 0 || chunk.type == PNG_TYPE("IDAT") || chunk.type == PNG_TYPE("IEND"))
				{
					break;
				}
				else if (!(chunk.type & PNG_CHUNK_ANCILLARY_BIT))
				{
					// TODO(simon): Break out if we find a critical chunk.
					image_maybe_log_unrecognized_chunk(chunk);
				}
			}

			for (; chunk.type == PNG_TYPE("IDAT"); chunk = png_parse_chunk(&contents))
			{
				// TODO(simon):
			}

			if (chunk.type == PNG_TYPE("IEND"))
			{
			}
			else
			{
				log_error("Expected 'IEND' chunk but got '%.4s'", (CStr) &chunk.type);
			}
		}
	}
	else
	{
		log_error("Invalid magic number");
	}
}
