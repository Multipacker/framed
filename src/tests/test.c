#include <stdio.h>
#include <stdlib.h>

#define FRAMED_IMPLEMENTATION
#include "public/framed.h"

int
main(int argc, char **argv)
{
	framed_init(1);
	framed_zone_begin("Test");
	framed_zone_end();
	framed_flush();
	return(0);
}