# Framed

The Framed profiler is a hobby project that measures various counters across
sections of your code and displays them visually. It consists of two parts: a
small STB-style single header file library for capturing counters, and a
graphical visualizer for displaying the counters.

Gathering of counters happens by manually adding a minimal amount of markup to
your code. This takes care of collecting counters and sending them over to the
visualizer.

Currently the only supported counter is [RDTSC], but more will be added later.


## Installation

Clone the repository to wherever you like. Then read the section below that
corresponds with you operating system.

### Windows

Open a developer terminal by searching for `x64 Native Tools Command Prompt for
VS <year>` in the start menu. Navigate to the root of the repository and run
the following commands:

```
script\build_freetype_msvc
script\build_msvc release
```

There will now be a `build` folder in the root of the project, and it will
contain `framed.exe`.

### Linux

You can either build with Clang or GCC. The project defaults to using the mold
linker in both cases. The following dependencies need to be installed before
building:

* SDL2

Open a terminal and navigate to the root of the project. Run the following
commands to compile using Clang. Equivalent scripts exist for
compiling with GCC.

```
script/build_freetype_clang.sh 
script/build_clang.sh release
```

There will now be a `build` folder in the root of the project, and it will
contain `framed`.

### MacOS

Framed doesn't currently support running on MacOS. As neither of us have a
Mac, it is unlikely that we will add support for it.


## Usage

To profile your application, add the following two lines to one of your source
files.

```c
#define FRAMED_IMPLEMENTATION
#include "framed.h"
```

This will generate the implementation of the library. To then use the markup in
any other files, you now only have to include `framed.h` in it.

Before any actual profiling can be done, you need to initialize the library.
This has to be done before using any other part of the API. Initialization
takes care of connecting to the visualizer and potentially waiting for the
connection to be established before giving back control to your program. This
is done in the following way:

```c
Framed_B32 wait_for_connection = true;
framed_init(wait_for_connection);
```

When your program is done profiling, it might not have sent all of the data to
the visualizer yet. To ensure this is done, you can call `framed_flush()`
before your program exits.

The API for collecting counters consists of two functions at its core:
`framed_zone_begin(NAME)` to start a new zone named `NAME`, and
`framed_zone_end()` to end it. Note that you have to call `framed_zone_end()`
at all exit points of your function, or else the event will not be recorded.

Here is a complete example. There are more complex ones availible in
`src/tests`.

> [!IMPORTANT]
> This has not yet been tested because the project is not fully functional yet.
> Test it before opening up the project.

```c
#include <stdio.h>

#define FRAMED_IMPLEMENTATION
#include "framed.h"

int
factorial(int n)
{
    framed_zone_begin("factorial");

    int result = 1;
    if (n > 1)
    {
        result = n * factorial(n - 1);
    }

    framed_zone_end();
    return(result);
}

int
main(int argc, char *argv[])
{
    framed_init(true);
    framed_zone_begin("main");

    int x = factorial(10);
    printf("10! = %d\n", x);

    framed_zone_end();
    framed_flush();
    return(0);
}
```


## TODO

- [ ] Both: Cleanup code and fix todos.
- [ ] Both: Try to minimize the number of arenas.
- [ ] Both: Asset streaming.
- [ ] Hampus: Use a default font if the font cache is full (null-font).
- [ ] Simon: Window handling on a separate thread.


## License

Framed is released under the [MIT License].


[MIT License]: https://mit-license.org/
[RDTSC]: https://en.wikipedia.org/wiki/Time_Stamp_Counter 
