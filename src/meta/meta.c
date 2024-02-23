#include "base/base_inc.h"
#include "os/os_inc.h"
#include "log/log_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "log/log_inc.c"

internal Void
meta_find_files(Arena *arena, Str8 folder, Str8List *result_files)
{
    arena_scratch(0, 0)
    {
        Str8 name = { 0 };
        OS_FileIterator iterator = { 0 };
        os_file_iterator_init(&iterator, folder);
        while (os_file_iterator_next(scratch, &iterator, &name))
        {
            Str8 complete_path = str8_pushf(arena, "%"PRISTR8"%c%"PRISTR8, str8_expand(folder), PATH_SEPARATOR, str8_expand(name));
            OS_FileProperties properties = os_file_properties(complete_path);

            if (properties.is_folder)
            {
                meta_find_files(arena, complete_path, result_files);
            }
            else
            {
                str8_list_push(arena, result_files, complete_path);
            }
        }
        os_file_iterator_end(&iterator);
    }
}

internal Str8List
meta_find_pre_processor_lines(Arena *arena, Str8List files)
{
    Str8List result = { 0 };

    for (Str8Node *file = files.first; file; file = file->next)
    {
        arena_scratch(0, 0)
        {
            Str8 contents = { 0 };
            if (os_file_read(scratch, file->string, &contents))
            {
                U8 *opl = contents.data + contents.size;
                B32 is_line_start = true;
                for (U8 *ptr = contents.data; ptr < opl; ++ptr)
                {
                    if (is_line_start && *ptr == '#')
                    {
                        U8 *start = ptr + 1;
                        while (ptr < opl && !(*(ptr - 1) != '\\' && (*ptr == '\n' || *ptr == '\r')))
                        {
                            ++ptr;
                        }

                        Str8 line = str8_copy(arena, str8_range(start, ptr));
                        str8_list_push(arena, &result, line);
                        is_line_start = true;
                    }
                    else if (*ptr == '\n' || *ptr == '\r')
                    {
                        is_line_start = true;
                    }
                    else if (*ptr != ' ' && *ptr != '\t')
                    {
                        is_line_start = false;
                    }
                }
            }
            else
            {
                log_error("Could not open file '%"PRISTR8"'", str8_expand(file->string));
            }
        }
    }

    return(result);
}

internal Str8
meta_skip_whitespace(Str8 string)
{
    U8 *ptr = string.data;
    U8 *opl = string.data + string.size;

    while (ptr < opl)
    {
        if (*ptr == ' ' || *ptr == '\t')
        {
            ++ptr;
        }
        else if (ptr + 2 < opl && ptr[0] == '\\' && ptr[1] == '\r' && ptr[2] == '\n')
        {
            ptr += 3;
        }
        else if (ptr + 1 < opl && ptr[0] == '\\' && ptr[1] == '\r')
        {
            ptr += 2;
        }
        else if (ptr + 1 < opl && ptr[0] == '\\' && ptr[1] == '\n')
        {
            ptr += 2;
        }
        else
        {
            break;
        }
    }

    Str8 result = str8_range(ptr, opl);
    return(result);
}

internal Str8
meta_get_string(Arena *arena, Str8 string)
{
    Str8 result = { 0 };

    U8 *ptr = string.data;
    U8 *opl = string.data + string.size;

    if (*ptr == '"')
    {
        do
        {
            ++ptr;
        } while(ptr < opl && *ptr != '"');

        if (*ptr == '"')
        {
            result = str8_range(string.data, ptr + 1);
        }
        else
        {
            // TODO(simon): Report location information.
            log_error("Unclosed string constant.");
        }
    }
    else
    {
        // TODO(simon): Report location information.
        log_error("Expected string constant.");
    }

    return(result);
}

typedef struct EmbedNode EmbedNode;
struct EmbedNode
{
    EmbedNode *next;
    EmbedNode *prev;

    Str8 source;
    Str8 embed;
};

typedef struct EmbedList EmbedList;
struct EmbedList
{
    EmbedNode *first;
    EmbedNode *last;
};

internal EmbedList
meta_find_embeds(Arena *arena, Str8List pre_processor_lines)
{
    EmbedList result = { 0 };
    for (Str8Node *line_node = pre_processor_lines.first; line_node; line_node = line_node->next)
    {
        Str8 line = line_node->string;
        line = meta_skip_whitespace(line);

        Str8 include_str = str8_lit("include");
        Str8 include = str8_prefix(line, include_str.size);
        line = str8_skip(line, include_str.size);
        line = meta_skip_whitespace(line);

        Str8 framed_embed_str = str8_lit("framed_embed");
        Str8 framed_embed = str8_prefix(line, framed_embed_str.size);
        line = str8_skip(line, framed_embed_str.size);
        line = meta_skip_whitespace(line);

        if (!(str8_equal(include, include_str) && str8_equal(framed_embed, framed_embed_str)))
        {
            continue;
        }

        Str8 left_paren_str = str8_lit("(");
        Str8 left_paren = str8_prefix(line, left_paren_str.size);
        line = str8_skip(line, left_paren_str.size);
        line = meta_skip_whitespace(line);

        Str8 source = meta_get_string(arena, line);
        line = str8_skip(line, source.size);
        line = meta_skip_whitespace(line);

        Str8 comma_str = str8_lit(",");
        Str8 comma = str8_prefix(line, comma_str.size);
        line = str8_skip(line, comma_str.size);
        line = meta_skip_whitespace(line);

        Str8 embed = meta_get_string(arena, line);
        line = str8_skip(line, embed.size);
        line = meta_skip_whitespace(line);

        Str8 right_paren_str = str8_lit(")");
        Str8 right_paren = str8_prefix(line, right_paren_str.size);
        line = str8_skip(line, right_paren_str.size);
        line = meta_skip_whitespace(line);

        if (!(str8_equal(left_paren, left_paren_str) && str8_equal(comma, comma_str) && str8_equal(right_paren, right_paren_str)))
        {
            // TODO(simon): Report location information.
            log_error("Invalid syntax for 'framed_embed'. Expected '#include framed_embed(SOURCE_FILE, EMBED_FILE)'.");
            continue;
        }

        if (source.size == 0)
        {
            // TODO(simon): Report location information.
            log_error("Missing source file path.");
            continue;
        }

        if (embed.size == 0)
        {
            // TODO(simon): Report location information.
            log_error("Missing embed file path.");
            continue;
        }

        // TODO(simon): Convert to using the platform specific path separator.
        EmbedNode *node = push_struct_zero(arena, EmbedNode);
        node->source = str8_skip(str8_chop(source, 1), 1);
        node->embed  = str8_skip(str8_chop(embed,  1), 1);

        dll_push_back(result.first, result.last, node);
    }

    return(result);
}

internal S32
os_main(Str8List arguments)
{
    arena_scratch(0, 0)
    {
        Str8 binary_path = os_push_system_path(scratch, OS_SystemPath_Binary);
        Str8 log_file = str8_pushf(scratch, "%"PRISTR8"%cmeta_log.txt", str8_expand(binary_path), PATH_SEPARATOR);
        log_init(log_file);
    }

    Arena *arena = arena_create("MetaPerm");
    Str8List files = { 0 };
    meta_find_files(arena, str8_lit("src"), &files);

    Str8List pre_processor_lines = meta_find_pre_processor_lines(arena, files);

    EmbedList embeds = meta_find_embeds(arena, pre_processor_lines);

    for (EmbedNode *node = embeds.first; node; node = node->next)
    {
        printf("%"PRISTR8", %"PRISTR8"\n", str8_expand(node->source), str8_expand(node->embed));
    }

    return(0);
}
