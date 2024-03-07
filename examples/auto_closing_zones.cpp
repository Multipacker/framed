#define FRAMED_IMPLEMENTATION
#include "public/framed.h"

int my_function_to_be_profiled(int n)
{
    framed_function; // note that we don't have to close this explicitly

    int first = 0;
    int second = 1;
    int next = first + second;
    for (int i = 3; i < 1000; ++i)
    {
        first = second;
        second = next;
        next = first + second;
    }

    return next;

}

int main(int argc, char **argv)
{
    framed_init(true, FRAMED_DEFAULT_PORT);
    {
        framed_zone_block("Sum loop"); // note that we don't have to close this explicitly
        int n = 0;
        for (int i = 0; i < 100; ++i)
        {
            n += my_function_to_be_profiled(i);
        }
    }
    framed_flush();
    return 0;
}