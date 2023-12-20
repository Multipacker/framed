internal OperatingSystem
operating_system_from_context(Void)
{
	OperatingSystem result = OperatingSystem_Null;
#if OS_WINDOWS
	result = OperatingSystem_Windows;
#elif OS_LINUX
	result = OperatingSystem_Linux;
#elif OS_MAC
	result = OperatingSystem_Mac;
#endif

	return(result);
}

internal Architecture
architecture_from_context(Void)
{
	Architecture result = Architecture_Null;
#if ARCH_X64
	result = Architecture_X64;
#elif ARCH_X86
	result = Architecture_X86;
#elif ARCH_ARM
	result = Architecture_ARM;
#elif ARCH_ARM64
	result = Architecture_ARM64;
#endif

	return(result);
}

internal BuildMode
build_mode_from_context(Void)
{
	BuildMode result = BuildMode_Null;
#if BUILD_MODE_DEBUG
	result = BuildMode_Debug;
#elif BUILD_MODE_OPTIMIZED
	result = BuildMode_Optimized;
#elif BUILD_MODE_RELEASE
	result = BuildMode_Release;
#endif
	return(result);
}

internal DateTime
build_date_from_context(Void)
{
	DateTime result = { 0 };
	Arena_Temporary scratch = arena_get_scratch(0, 0);

	// NOTE(hampus): Date is in the format "M D Y"
	// Example: "Dec 15 2023"
	Str8 date =  str8_cstr(__DATE__);

	// NOTE(hampus): Time is in the format "HH:MM:SS"
	// Example: "11:53:37"
	Str8 time = str8_cstr(__TIME__);

	{
		Str8List date_list = str8_split_by_codepoints(scratch.arena, date, str8_lit(" "));

		Str8Node *node = date_list.first;
		Str8 month = node->string;

		for (U8 i = 0; i < Month_COUNT; ++i)
		{
			if (str8_equal(month, string_from_month(i)))
			{
				result.month = i;
			}
		}

		date = str8_skip(date, 4);

		U64 len = u8_from_str8(date, &result.day);
		date = str8_skip(date, len+1);
		len = s16_from_str8(date, &result.year);
	}

	{
		U64 len = u8_from_str8(time, &result.hour);
		time = str8_skip(time, len+1);
		len = u8_from_str8(time, &result.minute);
		time = str8_skip(time, len+1);
		u8_from_str8(time, &result.second);
	}
	arena_release_scratch(scratch);
	return(result);
}

internal Str8
string_from_build_mode(BuildMode mode)
{
	Str8 result = str8_lit("NullBuildMode");
	switch (mode)
	{
		case BuildMode_Debug:
		{
			result = str8_lit("Debug");
		} break;

		case BuildMode_Optimized:
		{
			result = str8_lit("Optimized");
		} break;

		case BuildMode_Release:
		{
			result = str8_lit("Release");
		} break;

		invalid_case;
	}
	return(result);
}

internal Str8
string_from_operating_system(OperatingSystem os)
{
	Str8 result = str8_lit("NullOS");
	switch (os)
	{
		case OperatingSystem_Null:
		{
			result = str8_lit("Null");
		} break;

		case OperatingSystem_Windows:
		{
			result = str8_lit("Windows");
		} break;

		case OperatingSystem_Linux:
		{
			result = str8_lit("Linux");
		} break;

		case OperatingSystem_Mac:
		{
			result = str8_lit("Mac");
		} break;

		invalid_case;
	}

	return(result);
}

internal Str8
string_from_architecture(Architecture arc)
{
	Str8 result = str8_lit("NullArchitecture");
	switch (arc)
	{
		case Architecture_Null:
		{
			result = str8_lit("Null");
		} break;

		case Architecture_X64:
		{
			result = str8_lit("x64");
		} break;

		case Architecture_X86:
		{
			result = str8_lit("x86");
		} break;

		case Architecture_ARM:
		{
			result = str8_lit("ARM");
		} break;

		case Architecture_ARM64:
		{
			result = str8_lit("ARM64");
		} break;

		invalid_case;
	}

	return(result);
}

internal Str8
string_from_day_of_the_week(DayOfWeek day)
{
	Str8 result = str8_lit("NullDay");
	switch (day)
	{
		case DayOfWeek_Monday:
		{
			result = str8_lit("Monday");
		} break;

		case DayOfWeek_Tuesday:
		{
			result = str8_lit("Tuesday");
		} break;

		case DayOfWeek_Wednesday:
		{
			result = str8_lit("Wednesday");
		} break;

		case DayOfWeek_Thursday:
		{
			result = str8_lit("Thursday");
		} break;

		case DayOfWeek_Friday:
		{
			result = str8_lit("Friday");
		} break;

		case DayOfWeek_Saturday:
		{
			result = str8_lit("Saturday");
		} break;

		case DayOfWeek_Sunday:
		{
			result = str8_lit("Sunday");
		} break;

		invalid_case;
	}

	return(result);
}

internal Str8
string_from_month(Month month)
{
	Str8 result = str8_lit("NullMonth");

	switch (month)
	{
		case Month_Jan:
		{
			result = str8_lit("Jan");
		} break;

		case Month_Feb:
		{
			result = str8_lit("Feb");
		} break;

		case Month_Mar:
		{
			result = str8_lit("Mar");
		} break;

		case Month_Apr:
		{
			result = str8_lit("Apr");
		} break;

		case Month_May:
		{
			result = str8_lit("May");
		} break;

		case Month_Jun:
		{
			result = str8_lit("Jun");
		} break;

		case Month_Jul:
		{
			result = str8_lit("Jul");
		} break;

		case Month_Aug:
		{
			result = str8_lit("Aug");
		} break;

		case Month_Sep:
		{
			result = str8_lit("Sep");
		} break;

		case Month_Oct:
		{
			result = str8_lit("Oct");
		} break;

		case Month_Nov:
		{
			result = str8_lit("Nov");
		} break;

		case Month_Dec:
		{
			result = str8_lit("Dec");
		} break;

		invalid_case;
	}

	return(result);
}

internal DenseTime
dense_time_from_date_time(DateTime *date_time)
{
	DenseTime result = { 0 };

	result.time += (U32) ((S32) date_time->year + 0x8000);
	result.time *= 12;
	result.time += date_time->month;
	result.time *= 31;
	result.time += date_time->day;
	result.time *= 24;
	result.time += date_time->hour;
	result.time *= 60;
	result.time += date_time->minute;
	result.time *= 61;
	result.time += date_time->second;
	result.time *= 1000;
	result.time += date_time->millisecond;

	return result;
}

internal DateTime
date_time_from_dense_time(DenseTime dense_time)
{
	DateTime result = { 0 };

	result.millisecond = dense_time.time % 1000;
	dense_time.time /= 1000;
	result.second = dense_time.time % 61;
	dense_time.time /= 61;
	result.minute = dense_time.time % 60;
	dense_time.time /= 60;
	result.hour = dense_time.time % 24;
	dense_time.time /= 24;
	result.day = dense_time.time % 31;
	dense_time.time /= 31;
	result.month = dense_time.time % 12;
	dense_time.time /= 12;
	result.year = (S16) ((S32) dense_time.time - 0x8000);

	return result;
}