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

internal Str8
string_from_operating_system(OperatingSystem os)
{
	Str8 result = str8_lit("NullOS");
	switch (os)
	{
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
	}

	return(result);
}

internal Str8
string_from_architecture(Architecture arc)
{
	Str8 result = str8_lit("NullArchitecture");
	switch (arc)
	{
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
	}

	return(result);
}
