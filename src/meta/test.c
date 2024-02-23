#define framed_embed(source, embed) embed
static unsigned char test[] = {
#    include \
    framed_embed   ("src/meta/meta.c", "build/meta.h")
};

int
main()
{
}
