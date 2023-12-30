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
};

internal B32
image_parse_png_chunks(Arena *arena, Str8 contents, PNG_State *state)
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

internal Void
image_load(Arena *arena, Str8 contents)
{
	PNG_State state = { 0 };
	if (!image_parse_png_chunks(arena, contents, &state))
	{
		return;
	}
}
