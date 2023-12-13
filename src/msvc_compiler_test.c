#include "base/base_inc.h"

#include "base/base_inc.c"

internal S32 test(S32 x, S32 y, S32 z) {}

internal int main(int argument_count, char *arguments[])
{
	test(5, 2, 2);
	return (0);
}