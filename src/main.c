#include "base/base_inc.h"
#include "os/os_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"

internal S32
os_main(Str8List arguments)
{
	DateTime build_date = build_date_from_context();
	if (os_file_copy(str8_lit("build_clang.sh"), str8_lit("test"), false))
	{
		printf("success\n");
	}
	else
	{
		printf("fail\n");
	}
    
	return(0);
}