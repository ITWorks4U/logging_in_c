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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>

#ifdef _WIN32
// only for Windows
#include <Windows.h>

// for access function
#include <io.h>
#define F_OK 0
#define access _access
#else
// for (any) UNIX system
#include <unistd.h>
#endif

#include "logging.h"

// -----------
// internal settings
// -----------

/// @brief internal managed log file to use
static char _log_file_to_use[LENGTH_FILE_NAME];

/// @brief current timestamp for log event x
static char _timestamp[LENGTH_TIMESTAMP];

/// @brief Contains the previous log file name. More in use for dayly rotation.
static char _base_log_file[LENGTH_FILE_NAME];

/// @brief If set, comes from Logging.on_console_only, then no output
/// is going to write into the file, even a file name by Logging.file_name
/// has been set.
static bool _on_console_only = false;

/// @brief file pointer to use
static FILE *_log_file_pointer = NULL;

/// @brief The log level. Starts with LOG_INFO and will be updated by
/// Logging.init_level. Every log level, which is at least that level
/// is going to handle.
static LogLevel _level_for_logging = LOG_INFO;

/// @brief The log rotation setting. Comes from Logging.rotation_setting.
/// Depending on which rotation setting is set, the file might be updated
/// on a certain condition.
static LogRotation _log_rotation = NO_ROTATION;

/// @brief The size of a file in MB. Defaults to 1MB. Only in use with SIZE_ROTATION.
static int _size_for_file_size = 1024*1024;

/// @brief The number of keeping files for file rotation. Only in use, if
/// DAYLY_ROTATION or SIZE_ROTATION is set.
static int _nbr_of_keeping_files = 1;

// -----------
// constant expressions
// -----------

// only in use, if the file_name exceeds the boundary limit or no
// file name has been added
static const char *_default_log_name = "app.log";

// rotation names
static const char *_rotation_strings[] = {"NO_ROTATION", "DAYLY_ROTATION", "SIZE_ROTATION"};

// log types in words
static const char *_level_strings[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

/*
* Colors for log levels:
* TRACE = cyan
* DEBUG = light blue
* INFO  = green
* WARN  = yellow
* ERROR = magenta
* FATAL = red
*/
static const char *_level_colors[] = {"\x1b[36m", "\x1b[94m", "\x1b[32m", "\x1b[33m", "\x1b[35m", "\x1b[31m"};

// -----------
// internal functions
// -----------

/// @brief Receive the log level as a word. Only a valid range of [0..5]
/// can be used here. Every other value outside of the range will be handled
/// with LOG_INFO only.
/// @param level the level to look for
/// @return the log level name
static const char* _log_level_to_string(LogLevel level) {
	if (level >= LOG_TRACE && level <= LOG_FATAL) {
		return _level_strings[level];
	}

	return _level_strings[2];
}

/// @brief Create a new timestamp for the next time event.
/// The internal managed _timestamp C-string will be updated.
static void _create_new_timestamp(void) {
	time_t now = time(NULL);
	struct tm* t = localtime(&now);
	memset(_timestamp, '\0', LENGTH_TIMESTAMP);
	strftime(_timestamp, sizeof(_timestamp), "%Y-%m-%d %H:%M:%S", t);
}

/// @brief Rotate the log file. Only for DAYLY_ROTATING.
/// For SIZE_ROTATION take a look to _rotate_log_files().
///
/// When a new day has been detected, the log file will be renamed to 
/// <filename.log>_<timestamp>. <filename.log> becomes a new file to work with
static void _rotate_log_file_dayly(void) {
	char rotated_name[LENGTH_FILE_NAME + LENGTH_FILE_NAME];  // new rotation name of <filename.log>_<timestamp>
	char date_stamp[LENGTH_DATE_STAMP];                      // format: YYYY_MM_DD

	memset(rotated_name, '\0', LENGTH_FILE_NAME);
	memset(date_stamp, '\0', LENGTH_DATE_STAMP);

	// get current date
	time_t now = time(NULL);
	struct tm *tm_info = localtime(&now);
	strftime(date_stamp, sizeof(date_stamp), "%Y_%m_%d", tm_info);

	// build new rotated filename: <file_name>.log_YYYY_MM_DD
	snprintf(rotated_name, sizeof(rotated_name), "%s_%s", _base_log_file, date_stamp);

	// check, if the rotated name may already exist
	// if true, then a rename is not required here
	if (access(rotated_name, F_OK) != 0) {
		// rename current log file to rotated name
		if (rename(_log_file_to_use, rotated_name) < 0) {
			perror("Failed to rotate log file (dayly)");
			return;
		}

		// reset _log_file_to_use and rename it with
		// the given name from initialization
		memset(_log_file_to_use, '\0', LENGTH_FILE_NAME);
		strcpy(_log_file_to_use, _base_log_file);
	}
}

/// @brief Initiate to rotate the log files. This happens only, if the setting is
/// set to SIZE_ROTATION. For DAYLY_ROTATION take a look to _rotate_log_file_dayly().
///
/// A rotation to the next file, depending on the given nbr_of_keeping_files is going
/// to do, if required. If the limitation has been reached, then the oldest file is
/// going to overwrite.
static void _rotate_log_files(void) {
	char old_name[FILE_NAME_LOG_ROTATION];
	char new_name[FILE_NAME_LOG_ROTATION];

	// remove the oldest rotated file, if it exists
	snprintf(old_name, FILE_NAME_LOG_ROTATION, "%s.%d", _log_file_to_use, _nbr_of_keeping_files);
	remove(old_name);

	// shift rotated files up: logfile.(n-1) -> logfile.n
	for (int i = _nbr_of_keeping_files - 1; i >= 1; --i) {
		snprintf(old_name, FILE_NAME_LOG_ROTATION, "%s.%d", _log_file_to_use, i);
		snprintf(new_name, FILE_NAME_LOG_ROTATION, "%s.%d", _log_file_to_use, i + 1);

		// move old_name to new_name
		rename(old_name, new_name);
	}

	// rename the current log file <file_name>_<date_format>.log to <file_name>_<date_format>.logn
	// n = [1..nbr_of_keeping_files]
	snprintf(new_name, FILE_NAME_LOG_ROTATION, "%s.1", _log_file_to_use);
	rename(_log_file_to_use, new_name);

	// Now a new log file can be created as _log_file_to_use (e.g., logfile.log)
}

/// @brief Check, if a file needs a rotation. Only in use, if DAYLY_ROTATION or
/// SIZE_ROTATION is set. In both cases the Logging.nbr_of_keeping_files is in use.
///
/// SIZE_ROTATION: If the certain Logging.file_size_in_mb has exceeds the limit,
/// mark for a rotation.
///
/// DAYLY_ROTATION: If a new day starts, mark for a rotation.
/// @param the current file name to use
/// @return true, if a rotation is required, otherwise false
static bool check_for_new_rotation(const char *filename) {
	bool rotation_is_required = false;
	int size_in_mb = 0;

	#if _WIN32
	// only for Windows
	WIN32_FILE_ATTRIBUTE_DATA fileInfo;

	if (!GetFileAttributesEx(filename, GetFileExInfoStandard, &fileInfo)) {
		fprintf(stderr, "Could not get file attributes.\n");
		return false;
	}

	// file size in bytes
	LARGE_INTEGER size;
	size.HighPart = fileInfo.nFileSizeHigh;
	size.LowPart = fileInfo.nFileSizeLow;
	size_in_mb = (int)size.QuadPart;

	WIN32_FIND_DATA file_info_2;
	HANDLE hFind = FindFirstFile(filename, &file_info_2);

	if (hFind == INVALID_HANDLE_VALUE) {
		perror("hfind");
		return false;
	}

	// get current local date
	SYSTEMTIME now;
	GetLocalTime(&now);

	// get file creation time in local timezone
	SYSTEMTIME stUTC, stLocal;
	FileTimeToSystemTime(&fileInfo.ftCreationTime, &stUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

	// special case for dayly rotation (Windows)
	if (_log_rotation == DAYLY_ROTATION) {
		rotation_is_required = !(
			now.wYear == stLocal.wYear &&
			now.wMonth == stLocal.wMonth &&
			now.wDay == stLocal.wDay
		);
	}
	
	FindClose(hFind);

	#else
	// for UNIX systems only
	struct stat st;
	if (stat(filename, &st) != 0) {
		perror("stat failed");
		return false;
	}

	size_in_mb = (int)st.st_size;

	// NOTE: the POSIX creation time might not be available on all UNIX systems
	#if defined(__APPLE__) || defined(_MAC)
		time_t creation_time = st.st_birthtime;
	#elif defined(_BSD_SOURCE) || defined(__FreeBSD__)
		time_t creation_time = st.st_ctimespec.tv_sec;
	#else
		// fallback: use last status change time or last modified time
		time_t creation_time = st.st_mtime;
		// fprintf(stderr, "the real creation time is not available on this platform\n");
	#endif

	time_t now = time(NULL);
	struct tm file_tm, now_tm;

	localtime_r(&creation_time, &file_tm);
	localtime_r(&now, &now_tm);

	// special case for dayly rotation (UNIX system)
	if (_log_rotation == DAYLY_ROTATION) {
		rotation_is_required = !(
			file_tm.tm_year == now_tm.tm_year &&
			file_tm.tm_mon == now_tm.tm_mon &&
			file_tm.tm_mday == now_tm.tm_mday
		);
	}
	#endif

	// SIZE_ROTATION (available for Windows / UNIX)
	if (_log_rotation == SIZE_ROTATION) {
		if (size_in_mb >= _size_for_file_size) {
			rotation_is_required = true;
		}
	}

	// printf("rotation is required: %s\n", rotation_is_required ? "yes" : "no");
	return rotation_is_required;
}

// -----------
// public functions
// -----------

void init_log(Logging *log) {
	_level_for_logging = log->init_level;

	// check for an invalid log level
	if (!(_level_for_logging >= LOG_TRACE && _level_for_logging <= LOG_FATAL)) {
		fprintf(
			stderr,
			"Warning: invalid log level setting detected. Set the level to %s default.\n",
			_level_strings[2]
		);
		_level_for_logging = LOG_INFO;
	}

	if (!log->on_console_only) {
		// file handling only

		_log_rotation = log->rotation_setting;

		// if SIZE_ROTATION is given, the size to store must be at least 1 (MB),
		// otherwise NO_ROTATION is set
		if (log->file_size_in_mb < 1 && _log_rotation == SIZE_ROTATION) {
			fprintf(
				stderr,
				"Warning: invalid setting for option %s detected. Switching to option %s instead.\n",
				_rotation_strings[2],
				_rotation_strings[0]
			);
			_log_rotation = NO_ROTATION;
		} else {
			// 1024*1024 (bytes) * log->file_size_in_mb in MB
			_size_for_file_size *= log->file_size_in_mb;
		}

		_nbr_of_keeping_files = log->nbr_of_keeping_files;

		// if an invalid number (<2) is set, set this value to 2
		// only for DAYLY_ROTATION and SIZE_ROTATION
		if (_nbr_of_keeping_files < 2 && _log_rotation != NO_ROTATION) {
			fprintf(
				stderr,
				"Warning: invalid number of keeping files detected: %d. Using 2 files to keep up by default.\n",
				_nbr_of_keeping_files
			);
			_nbr_of_keeping_files = 2;
		}

		size_t name_length = strlen(log->file_name);
		bool on_valid_name_length = name_length > 0 && name_length < LENGTH_FILE_NAME;

		if (!on_valid_name_length) {
			fprintf(
				stderr,
				"Warning: Invalid length (%d) for file name detected. Using a default file name \"%s\" instead.\n",
				(int) name_length, _default_log_name
			);
			strcpy(_log_file_to_use, _default_log_name);
		} else {
			strcpy(_log_file_to_use, log->file_name);
		}

		// in use for dayly rotation
		strcpy(_base_log_file, _log_file_to_use);
	} else {
		// only for handling on stdout
		_on_console_only = true;
	}
}

void log_message(LogLevel level, const char* format, ...) {
	if (level < _level_for_logging) {
		// every level, which has a lower value compared to the initial level
		// won't be handled
		return;
	}

	char log_line[LENGTH_LOG_MESSAGE];
	memset(log_line, '\0', LENGTH_LOG_MESSAGE);

	va_list args;
	va_start(args, format);
	vsnprintf(log_line, sizeof(log_line), format, args);
	va_end(args);

	_create_new_timestamp();

	if (!_on_console_only) {
		_log_file_pointer = fopen(_log_file_to_use, "a");
		bool on_valid_file_pointer = _log_file_pointer != NULL;

		if (!on_valid_file_pointer) {
			perror("Unable to write a log file.");
		} else {
			if (_log_rotation != NO_ROTATION) {
				// close this file stream and check, depending on which rotation is set,
				// if a file rotation is required
				dispose_logging();

				if (check_for_new_rotation(_log_file_to_use)) {
					if (_log_rotation == DAYLY_ROTATION) {
						// only for DAYLY_ROTATION
						_rotate_log_file_dayly();
					} else {
						// only for SIZE_ROTATION
						_rotate_log_files();
					}
				}

				_log_file_pointer = fopen(_log_file_to_use, "a");
				on_valid_file_pointer = _log_file_pointer != NULL;
			}

			// handle only logging events, when a file pointer exists
			if (on_valid_file_pointer) {
				fprintf(_log_file_pointer, "[%s] [%s] %s\n", _timestamp, _log_level_to_string(level), log_line);
			}

			dispose_logging();
		}
	} else {
		fprintf(stdout, "[%s] %s[%s]%s ", _timestamp, _level_colors[level], _log_level_to_string(level), COLOR_RESET);

		va_list args_2;
		va_start(args_2, format);
		vfprintf(stdout, format, args_2);
		va_end(args_2);

		fprintf(stdout, "\n");
	}
}

void dispose_logging(void) {
	if (_log_file_pointer != NULL) {
		fclose(_log_file_pointer);
		_log_file_pointer = NULL;
	}
}