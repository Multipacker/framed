#include <stdio.h>

#define FRAMED_IMPLEMENTATION
#include "public/framed.h"

#define false 0
#define true 1

int main(int argc, char **argv)
{
	framed_init(true);
	framed_zone_begin("Test");
	framed_zone_end();
	framed_flush();
	while (true)
	{

	}
	return 0;
}