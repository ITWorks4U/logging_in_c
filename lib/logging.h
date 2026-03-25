/*
* Customized logging system. This allows you to log an event to stdout or into a file.
* The log levels are in the range of [TRACE..FATAL].
*
* By using stdout, the given log state is colorized.
* By using a file, you can choose between one of three options:
*    NO_ROTATION     := everything is going to write into the used log file
*    DAILY_ROTATION  := rotate the log file(s), when a new date has been detected
*    SIZE_ROTATION   := rotate the log file(s), when a certain size limit (in MB) has been exceeded
*
* This project has been written and tested on a Windows machine (Windows 11, 64bit) and
* on an UNIX machine (Linux Mint 22). The both systems (Windows / UNIX) shall be able
* to build and run this project.
*
* NOTE: All arguments for a log are required to set, even if you don't use all settings.
*       Otherwise an undefined behavior on runtime may appear.
*
* NOTE: This application has been written in C and only a C compiler was in use. It's not clear,
*       if a C++ compiler does the job well.
*
* @author    itworks4u
* @created   October 12th, 2025
* @updated   March 25th, 2026
* @version   1.2.0
*/

#ifndef LOGGING_H
#define LOGGING_H
#include <stdbool.h>

// -----------
// definitions
// -----------
#define LENGTH_TIMESTAMP         20
#define LENGTH_TIMESTAMP_BUFFER  256
#define LENGTH_LOG_MESSAGE       1024
#define LENGTH_FILE_NAME         32
#define SHORT_TIMESTAMP_LENGTH   11
#define FILE_NAME_LOG_ROTATION   512
#define LENGTH_DATE_STAMP        16

// reset the text color to the default value
#define COLOR_RESET              "\x1b[0m"

// -----------
// structures
// -----------

/// @brief Contains the level for logging, where TRACE has the
/// lowest priority.
typedef enum {
	LOG_TRACE,
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR,
	LOG_FATAL
} LogLevel;

/// @brief Rotation types for logging. Works only for logging into a file.
typedef enum {
	NO_ROTATION,
	DAILY_ROTATION,
	SIZE_ROTATION
} LogRotation;

/// @brief Logging container. Offers to write a log event into a given file name.
///
/// If the member on_console_only is set to true,
/// then no log file is in use here and every log output will be printed
/// to stdout. Members:
///
/// - file_name            = The file name for logging. By default next text is going to append to this file.
///                          If no file name is given or the name length is outside of [1..31] characters,
///                          then "app.log" will be used instead.
///
/// - init_level           = The minimal level for logging. Every log level below this limit is going to ignore.
///                          If the level is outside of [LOG_TRACE .. LOG_FATAL], then LOG_INFO is set.
///
/// - on_console_only      = optional flag; if set, then the settings: file_name, rotation_setting, file_size_in_mb, nbr_of_keeping_files are ignored
///
/// - rotation_setting     = Setup for log rotation. By default no rotation is set.
///                          Valid options: [NO_ROTATION, DAILY_ROTATION, SIZE_ROTATION]
///
/// - file_size_in_mb      = Only in use for SIZE_ROTATION. If the curernt file size has been reached the upper limit,
///                          then the next rotation begins. If the value is <1, then the rotation_setting will be set to NO_ROTATION
///
/// - nbr_of_keeping_files = The number of files to keep, before the oldest file is going to overwrite.
///                          Only in use for DAILY_ROTATION or SIZE_ROTATION. If the value is <2, then the number is set to 2 by default.
typedef struct {
	char file_name[LENGTH_FILE_NAME];
	LogLevel init_level;
	LogRotation rotation_setting;
	int file_size_in_mb;
	int nbr_of_keeping_files;
	bool on_console_only;
} Logging;

// -----------
// function prototypes
// -----------

/// @brief Initialize a new log by given Logging container. If the argument is NULL, then a default setting
///        with console output and init_level to LOG_INFO is in use instead.
/// @param log the logging container with known settings
void init_log(Logging *log);

/// @brief Initializing a new logging sequence by using the certain arguments.
/// @param file_name name of the log file; if unset or outside of [1..31] characters, "app.log" will be used instead
/// @param init_level minimal log level; level range: [LOG_TRACE .. LOG_FATAL]
/// @param rotation log rotation setting; works only for a file logging; rotation settings: [NO_ROTATION, DAILY_ROTATION, SIZE_ROTATION]
/// @param size_in_mb file size in MB before rotation begins; only for SIZE_ROTATION setting
/// @param keep_nbr_files number of files to keep; only for DAILY_ROTATION and SIZE_ROTATION
/// @param on_console write the log events to the console only; if set, then the settings: file_name, rotation, size_in_mb, keep_nbr_files are ignored
void init_log_by_arguments(const char *file_name, const LogLevel init_level, const LogRotation rotation, int size_in_mb, int keep_nbr_files, bool on_console);

/// @brief Log a message into a file. If console output is set, then the log messages are moved to stdout instead.
///
/// NOTE: Depending on the given LOG_LEVEL every log message, which is below the given level, won't be handled.
///
/// NOTE: If a log output to stdout is set, then no output will be written into a file.
/// @param level current log level
/// @param format the formatted text
void write_to_log(LogLevel level, const char* format, ...);

// /// @brief Determine the current log file for file rotation.
// /// @param buffer last known log file name
// /// @param size the length of characters for buffer argument
// void determine_log_filename(char* buffer, size_t size);

/// @brief Dispose allocated memory for logging.
void dispose(void);
#endif