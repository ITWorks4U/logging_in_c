/*
* Customized logging system. This allows you to log an event to stdout or into a file.
* The log levels are in the range of [TRACE..FATAL].
*
* By using stdout, the given log state is colorized.
* By using a file, you can choose between one of three options:
*    NO_ROTATION     := everything is going to write into the used log file
*    DAYLY_ROTATION  := rotate the log file(s), when a new date has been detected
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
* @updated   October 15th, 2025
* @version   1.0.0
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

typedef enum {
	NO_ROTATION,
	DAYLY_ROTATION,
	SIZE_ROTATION
} LogRotation;

/// @brief Logging container. If the member on_console_only is set to true,
/// then no log file is in use here. Every log output will be printed
/// to stdout. Members:
///
/// - file_name            = The file name for logging By default next text will be appended to this file.
///                          If no file name is given or the name length is outside of [0..31] characters,
///                          then "app.log" will be used instead.
///
/// - init_level           = the minimal level for logging; every log level below the limit is going to ignore
///
/// - on_console_only      = optional flag; if set, then no file is in use
///
/// - rotation_setting     = Setup for log rotation. By default no rotation is set.
///   Valid options: [NO_ROTATION, DAYLY_ROTATION, SIZE_ROTATION]
///
/// - file_size_in_mb      = Only in use for SIZE_ROTATION. The amount of MB before the next rotation
///   is going to handle. If a value <1 is set, then the rotation_setting will be set to NO_ROTATION!
///
/// - nbr_of_keeping_files = The number of files to store before the oldest file is going to overwrite.
///   Only in use for DAYLY_ROTATION or SIZE_ROTATION. If the value is <2, then the number is set to 2 by default.
typedef struct {
	char file_name[LENGTH_FILE_NAME];
	LogLevel init_level;
	bool on_console_only;
	LogRotation rotation_setting;
	int file_size_in_mb;
	int nbr_of_keeping_files;
} Logging;

// -----------
// function prototypes
// -----------

/// @brief Initialize a new log by given Logging container.
/// @param log the logging container with known settings
void init_log(Logging *log);

/// @brief Log a message into a file. If console output is set, then the log messages are moved to stdout instead.
///
/// NOTE: Depending on the given LOG_LEVEL every log message, which is below the given level, won't be handled.
///
/// NOTE: If a log output to stdout is set, then no output will be written into a file.
/// @param level current log level
/// @param format the formatted text
void log_message(LogLevel level, const char* format, ...);

// /// @brief Determine the current log file for file rotation.
// /// @param buffer last known log file name
// /// @param size the length of characters for buffer argument
// void determine_log_filename(char* buffer, size_t size);

/// @brief Dispose allocated memory for logging.
void dispose_logging(void);
#endif