# Logging in C

-   written in C and compiled with a C compiler only
    -   it's not clear, if a C++ compiler may do the job as well as a C compiler
-   written and tested on a Windows machine (Windows 11, 64bit) and on an UNIX machine (Linux Mint 22)
    -   other systems (Windows / UNIX) shall be able to build and run this project

| details |
| - |
| author: itworks4u |
| created: October 13th, 2025 |
| updated: March 25th, 2026 |
| version: 1.2.0 |

##  description
-   customized logging system
-   this allows you to log an event to stdout or into a file
-   the log levels are in the range of `[TRACE..FATAL]`

### How to build
> **NOTE**: Don't use a **C++** compiler, because this won't often be able to build. Use a **C** compiler only.

####    by makefile
-   use the `makefile[.bat]` file (depending on your used OS)

####    by hand
-   use: `gcc(.exe) -g3 -Wall your_main_file.c lib/logging.c -Ilib -o your_output_file`
-   just import the lib folder with `logging.c`
    -   include the lib folder, too: `-Ilib`
    -   the additional flags `-g3 -Wall` are not required, but useful

####    using test files
-   in the folder `tests/` four files with a special case exists
-   compile with: `gcc(.exe) -g3 -Wall tests/certain_file.c lib/logging.c -Ilib`

### function overview
```
void init_log(Logging *log);
void init_log_by_arguments(const char *file_name, const LogLevel init_level, const LogRotation rotation, int size_in_mb, int keep_nbr_files, bool on_console);
void write_to_log(LogLevel level, const char* format, ...);
void dispose_logging(void);
```

###  details
|   function    |   description | additional informations |
| - | - | - |
| `init_log();` | initializing a logging session with `Logging` structure settings | if the argument is **NULL**, then the console output and a minimal log level with **LOG_INFO** is set |
| `init_log_by_arguments();` | initializing a logging session with given arguments instead | if `file_name` points to **NULL**, then the default log name **app.log** will be used instead |
| `write_to_log();` | write a new log event to a file, if given, or to stdout | if the given level is lower than the initialized log level, this message will be ignored |
| `dispose();` | clean up (the mess) | by default the internal used pointers are going to release automatically, but this is a nice option to have |

> **NOTE**: If no settings for the structure below is set, then the logging will be handled in a default way:
>>  - logging to stdout only
>>  -   using LOG_INFO by default
>>  -   no file handling is in use 

```
typedef struct {
    char file_name[LENGTH_FILE_NAME];
    LogLevel init_level;
    LogRotation rotation_setting;
    int file_size_in_mb;
    int nbr_of_keeping_files;
    bool on_console_only;
} Logging;
```
| members | description | additional informations |
| - | - | - |
| file_name | The file name for logging. By default next message will be append to this file. | If the length of the argument is outside of `[1..31]` characters, then **"app.log"** will be used instead. |
| init_level | The minimal level for logging. | Every log level below the limit is going to ignore. |
| rotation_setting | Setup for log rotation. By default no rotation is set. | see: level for log rotation table |
| file_size_in_mb | Only in use for **SIZE_ROTATION**. The amount of MB before the next rotation is going to handle. | If a value *below 1* is set, then the rotation_setting will be set to **NO_ROTATION**. |
| nbr_of_keeping_files | The number of files to store before the oldest file is going to overwrite. | Only in use for **DAILY_ROTATION** or **SIZE_ROTATION**. If the value is *below 2*, then the number is set to **2** by default. |
| on_console_only | Optional boolean flag. If set, then no file output and no rotation setting is in use. | No matter, if a file name is given. |

####    log levels
```
typedef enum {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL
} LogLevel;
```

| level | description | color (only for stdout) |
| - | - | - | 
| TRACE | tracing all messages | cyan |
| DEBUG | for debug messages | light blue |
| INFO | information messages | green |
| WARN | warning messages | yellow |
| ERROR | error messages | magenta |
| FATAL | pray to god...| red |

-   you can choose between one of the level options: `[TRACE, DEBUG, INFO, WARN, ERROR, FATAL]`
    -   **NOTE**: *Any other unknown option leads to **INFO**.*

####    log rotation
```
typedef enum {
    NO_ROTATION,
    DAILY_ROTATION,
    SIZE_ROTATION
} LogRotation;
```

-   you can choose between one of three options: `[NO_ROTATION, DAILY_ROTATION, SIZE_ROTATION]`
    -   **NOTE**: *Any other unknown option leads to **NO_ROTATION**.*

| level | meaning |
| - | - |
| NO_ROTATION | everything is going to write into the used log file |
| DAILY_ROTATION | rotate the log file(s), when a new day has been detected |
| SIZE_ROTATION | rotate the log file(s), when a certain size limit (in MB) has been exceeded |