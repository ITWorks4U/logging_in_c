# Logging in C

-   written in C and compiled with a C compiler only
    -   it's not clear, if a C++ compiler may do the job as well as a C compiler
-   written and tested on a Windows machine (Windows 11, 64bit) and on an UNIX machine (Linux Mint 22)
    -   other systems (Windows / UNIX) shall be able to build and run this project

| details |
| - |
| author: itworks4u |
| created: October 13th, 2025 |
| updated: October 14th, 2025 |
| version: 1.0.0 |

##  description
-   customized logging system
-   this allows you to log an event to stdout or into a file
-   the log levels are in the range of `[TRACE..FATAL]`

##  How to build
-   just import the lib folder with `logging.c`
    -   include the lib folder, too: `-Ilib`
    -   the additional flags `-g3 -Wall` are not required, but useful
-   use: `gcc(.exe) -g3 -Wall your_file.c lib/logging.c -Ilib`

##  using test files
-   in the folder `tests/` four files with each case exists
-   compile with: `gcc(.exe) -g3 -Wall tests/certain_file.c lib/logging.c -Ilib`

##  details
####    How to initialize
-   the logging system comes with a structure, called Logging
    -   initialize a log: `void init_log(Logging *log);`
    -   write a log: `void log_message(LogLevel level, const char* format, ...);`
        -   unlike to use macro functions, this function handles the current message with the given log level
        -   if the given log level has a lower priorty compared to the initialized level, then the certain message won't be handled
    -   clean up (the mess): `void dispose_logging(void);`
        -   by default the used pointers are going to release automatically, but it's a nice option to have

> **NOTE**: If no settings for the structure below is set, then the logging will be handled in a default way:
>>  - logging to stdout only
>>  -   using LOG_INFO by default
>>  -   no file handling is in use 

```
typedef struct {
	char file_name[LENGTH_FILE_NAME];
	LogLevel init_level;
	bool on_console_only;
	LogRotation rotation_setting;
	int file_size_in_mb;
	int nbr_of_keeping_files;
} Logging;
```
| members | description | additional informations |
| - | - | - |
| file_name | The file name for logging. By default next text will be appended to this file. | If no file name is given or the name length is outside of `[0..31]` characters, then "app.log" will be used instead. |
| init_level | The minimal level for logging. | Every log level below the limit is going to ignore. |
| on_console_only | Optional boolean flag. If set, then no file is in use. | No matter, if a file is given. |
| rotation_setting | Setup for log rotation. By default no rotation is set. | see: level for log rotation table |
| file_size_in_mb | Only in use for **SIZE_ROTATION**. The amount of MB before the next rotation is going to handle. | If a value *below 1* is set, then the rotation_setting will be set to **NO_ROTATION**. |
| nbr_of_keeping_files | The number of files to store before the oldest file is going to overwrite. | Only in use for **DAYLY_ROTATION** or **SIZE_ROTATION**. If the value is *below 2*, then the number is set to **2** by default. |

####    log levels
| level | description | color (only for stdout) |
| - | - | - | 
| TRACE | tracing all messages | cyan |
| DEBUG | for debug messages | light blue |
| INFO | information messages | green |
| WARN | warning messages | yellow |
| ERROR | error messages | magenta |
| FATAL | pray to god...| red |

####    log to stdout
-   the given log state is colorized
    -   see: table of log levels

####    log to a file
-   by using a file, you can choose between one of three options: `[NO_ROTATION, DAYLY_ROTATION, SIZE_ROTATION]`

| level | meaning |
| - | - |
| NO_ROTATION | everything is going to write into the used log file |
| DAYLY_ROTATION | rotate the log file(s), when a new day has been detected |
| SIZE_ROTATION | rotate the log file(s), when a certain size limit (in MB) has been exceeded |