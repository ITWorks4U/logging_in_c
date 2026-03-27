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
* @updated   March 27th, 2026
* @version   1.3.0
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
#include <errno.h>
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
///        is going to write into the file, even a file name by Logging.file_name
///        has been set.
static bool _on_console_only = false;

/// @brief file pointer to use
static FILE *_log_file_pointer = NULL;

/// @brief The log level. Starts with LOG_INFO and will be updated by
///        Logging.init_level. Every log level, which is at least that level
///        is going to handle.
static LogLevel _level_for_logging = LOG_INFO;

/// @brief The log rotation setting. Comes from Logging.rotation_setting.
///        Depending on which rotation setting is set, the file might be updated
///        on a certain condition.
static LogRotation _log_rotation = UNSET_ROTATION;

/// @brief The size of a file in MB. Defaults to 1MB. Only in use with SIZE_ROTATION.
static int _size_for_file_size = 1024*1024;

/// @brief The number of keeping files for file rotation. Only in use, if
///        DAILY_ROTATION or SIZE_ROTATION is set.
static int _nbr_of_keeping_files = 1;

/// @brief internal flag to check, if the initializing sequence has been passed trough
///        to avoid an undefined behavior, when log_to_write() function has been called
///        without init_log() or init_log_by_arguments()
static bool _initializing_done = false;

// -----------
// fixed expressions
// -----------

// only in use, if the file_name exceeds the boundary limit or no
// file name has been added
static const char *_default_log_name = "app.log";

// rotation names
static const char *_rotation_strings[] = {"NO_ROTATION", "DAILY_ROTATION", "SIZE_ROTATION"};

// log types in words
static const char *_level_strings[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

// log levels:                     |    TRACE   |   DEBUG    |  INFO    |  WARNING  |   ERROR   |   FATAL    |
// colors for log levels:          |     cyan   | light blue |  green   |   yellow  |  magenta  |    red     |
static const char *_level_colors[] = {"\x1b[36m", "\x1b[94m", "\x1b[32m", "\x1b[33m", "\x1b[35m", "\x1b[31m"};

// -----------
// internal functions
// -----------

/// @brief Receive the log level as a word. Only a valid range of [0..5]
///        can be used here. Every other value outside of the range will be handled
///        with LOG_INFO only.
/// @param level the level to look for
/// @return the log level name
static const char* _log_level_to_string(LogLevel level) {
	if (level >= LOG_TRACE && level <= LOG_FATAL) {
		return _level_strings[level];
	}

	return _level_strings[2];
}

/// @brief Create a new timestamp for the next time event.
///        The internal managed _timestamp C-string will be updated.
static void _create_new_timestamp(void) {
	time_t now = time(NULL);
	struct tm* t = localtime(&now);
	memset(_timestamp, '\0', LENGTH_TIMESTAMP);
	strftime(_timestamp, sizeof(_timestamp), "%Y-%m-%d %H:%M:%S", t);
}

/// @brief Initiate to rotate the log files. This happens only, if the setting is
///        set to SIZE_ROTATION. For DAILY_ROTATION take a look to _rotate_log_file_daily().
///
///        A rotation to the next file, depending on the given nbr_of_keeping_files is going
///        to do, if required. If the limitation has been reached, then the oldest file is
///        going to overwrite.
static void _rotate_log_files(void) {
	char rotated_name[FILE_NAME_LOG_ROTATION];
	char new_name[FILE_NAME_LOG_ROTATION];

	// remove the oldest rotated file, if it exists
	snprintf(rotated_name, sizeof(rotated_name), "%s.%d", _log_file_to_use, _nbr_of_keeping_files);
	if (access(rotated_name, F_OK) == 0) {
		remove(rotated_name);
	}

	// shift rotated files up: logfile.(n-1) -> logfile.n
	for (int i = _nbr_of_keeping_files - 1; i >= 1; --i) {
		snprintf(rotated_name, sizeof(rotated_name), "%s.%d", _log_file_to_use, i);
		snprintf(new_name, sizeof(new_name), "%s.%d", _log_file_to_use, i + 1);

		// move old_name to new_name
		rename(rotated_name, new_name);
	}

	// rename the current log file <file_name>_<date_format>.log to <file_name>_<date_format>.logn
	// n = [1..nbr_of_keeping_files]
	snprintf(new_name, sizeof(new_name), "%s.1", _log_file_to_use);
	rename(_log_file_to_use, new_name);

	// Now a new log file can be created as _log_file_to_use (e.g., logfile.log)
}

/// @brief Thread safe localtime function access. Depending on which OS this application
///        is running, the real localtime_x function in a certain order, followed by
///        the correct return value is in use.
/// @param timestamp the current timestamp
/// @param out address to struct tm
/// @return 0, if the called sub function didn't failed, otherwise [result < 0 > result]
static int _on_safe_localtime(time_t *timestamp, struct tm *out) {
	#ifdef _WIN32
	return localtime_s(out, timestamp);
	#else
	return localtime_r(timestamp, out) == NULL;
	#endif
}

/// @brief Rotate the log file. Only for DAYLY_ROTATING.
///        For SIZE_ROTATION take a look to _rotate_log_files().
///
///        When a new day has been detected, the log file will be renamed to 
///        <filename.log>_<timestamp>. <filename.log> becomes a new file to work with
static void _rotate_log_file_daily(void) {
	struct stat st;

	if (stat(_log_file_to_use, &st) != 0) {
		// general error with stat structure
		write_to_log(LOG_ERROR, "Unable to use stat for daily rotation: %s", strerror(errno));
		return;
	}

	time_t now = time(NULL);

	struct tm now_tm;
	struct tm file_tm;

	if (_on_safe_localtime(&now, &now_tm) != 0 || _on_safe_localtime(&st.st_mtime, &file_tm) != 0) {
		write_to_log(LOG_ERROR, "Unable to set a localtime for daily rotation: %s", strerror(errno));
		return;
	}

	// compare the date stats
	if (now_tm.tm_year != file_tm.tm_year || now_tm.tm_yday != file_tm.tm_yday) {
		_rotate_log_files();
	}
}

#ifdef _WIN32
// special case(s) for Windows systems only

/// @brief Create an error message depending on known error number. Since this comes from
///        the Windows API, a standard function, like strerror(errno) can't be used here.
///        Furthermore this function shall also be available for threading.
///
///        The error message points to the address of a dynamic C-string. If this doesn't
///        points to NULL after the function call, this which MUST be freed after its usage.
///        This can be realized with LocalFree() function.
/// @param error_number the detected error number
/// @param error_message the address for the error message
void _generate_last_error_message(DWORD error_number, char **error_message) {
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,    // dwflags
		NULL,                                                                                           // lpSource
		error_number,                                                                                   // dwMEssageId
		0,                                                                                              // dwLanguageId
		(LPSTR) error_message,                                                                          // lpBuffer
		0,                                                                                              // nSize
		NULL                                                                                            // Arguments
	);
}
#endif

/// @brief Check, if a file needs a rotation. Only in use, if DAILY_ROTATION or
///        SIZE_ROTATION is set. In both cases the Logging.nbr_of_keeping_files is in use.
///
///        SIZE_ROTATION: If the certain Logging.file_size_in_mb has exceeds the limit,
///        mark for a rotation.
///
///        DAILY_ROTATION: If a new day starts, mark for a rotation.
/// @return true, if a rotation is required, otherwise false
static bool _check_for_new_rotation(void) {
	bool rotation_is_required = false;
	int size_in_mb = 0;

	#ifdef _WIN32
	// only for Windows
	WIN32_FILE_ATTRIBUTE_DATA fileInfo;
	char *error_message = NULL;
	DWORD last_error = 0;

	if (!GetFileAttributesEx(_log_file_to_use, GetFileExInfoStandard, &fileInfo)) {
		last_error = GetLastError();
		_generate_last_error_message(last_error, &error_message);

		if (error_message != NULL) {
			write_to_log(LOG_ERROR, "Could not get file attributes: %s", error_message);
			LocalFree(error_message);
		} else {
			write_to_log(LOG_ERROR, "Could not get file attributes. (error id: %lu)", last_error);
		}

		return false;
	}

	// file size in bytes
	LARGE_INTEGER size;
	size.HighPart = fileInfo.nFileSizeHigh;
	size.LowPart = fileInfo.nFileSizeLow;
	size_in_mb = (int)size.QuadPart;

	WIN32_FIND_DATA file_info_2;
	HANDLE hFind = FindFirstFile(_log_file_to_use, &file_info_2);

	if (hFind == INVALID_HANDLE_VALUE) {
		last_error = GetLastError();
		_generate_last_error_message(last_error, &error_message);

		if (error_message != NULL) {
			write_to_log(LOG_ERROR, "Error with hFind: %s", error_message);
			LocalFree(error_message);
		} else {
			write_to_log(LOG_ERROR, "Error with hFind. (error id: %lu)", last_error);
		}

		return false;
	}

	// get current local date
	SYSTEMTIME now;
	GetLocalTime(&now);

	// get file creation time in local timezone
	SYSTEMTIME stUTC, stLocal;
	FileTimeToSystemTime(&fileInfo.ftCreationTime, &stUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

	// special case for daily rotation (Windows)
	if (_log_rotation == DAILY_ROTATION) {
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
	if (stat(_log_file_to_use, &st) != 0) {
		write_to_log(LOG_ERROR, "Unable to use stat for current file: %s", strerror(errno));
		return false;
	}

	size_in_mb = (int)st.st_size;

	// NOTE: the POSIX creation time might not be available on all UNIX systems
	#if defined(__APPLE__) || defined(_MAC)
		time_t creation_time = st.st_birthtime;
	#elif defined(_BSD_SOURCE) || defined(__FreeBSD__)
		time_t creation_time = st.st_ctimespec.tv_sec;
	#else
		write_to_log(LOG_TRACE, "The real creation time isn't available on this platform. A fallback mechanism is in use instead.");
		time_t creation_time = st.st_mtime;
	#endif

	time_t now = time(NULL);
	struct tm file_tm, now_tm;

	localtime_r(&creation_time, &file_tm);
	localtime_r(&now, &now_tm);

	// special case for dayly rotation (UNIX system)
	if (_log_rotation == DAILY_ROTATION) {
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

	return rotation_is_required;
}

/// @brief Final log initializer. The settings are come from init_log_by_arguments() or init_log() function(s).
static void _internal_log_initializer(const char *file_name, const LogLevel init_level, const LogRotation rotation, int size_in_mb, int keep_nbr_files, bool on_console) {
	_level_for_logging = init_level;
	int level_warning = 3;

	if (!(_level_for_logging >= LOG_TRACE && _level_for_logging <= LOG_FATAL)) {                                                   // check for an invalid log level setting
		fprintf(
			stderr,
			"%sWarning: invalid log level setting detected. Set the level to %s default.%s\n",
			_level_colors[level_warning], _level_strings[2], COLOR_RESET
		);

		_level_for_logging = LOG_INFO;
	}

	if (on_console) {
		_on_console_only = true;
		_initializing_done = true;
		return;
	}

	// file handling options are selected
	_nbr_of_keeping_files = (keep_nbr_files - 1);                                                                                  // nbr of files to keep
	_size_for_file_size *= size_in_mb;                                                                                             // 1024*1024*size_in_mb

	switch(rotation) {
		case NO_ROTATION:    // = 0
			_log_rotation = NO_ROTATION;
			break;
		case DAILY_ROTATION: // = 1
			_log_rotation = DAILY_ROTATION;
			break;
		case SIZE_ROTATION:  // = 2
			_log_rotation = SIZE_ROTATION;
			break;
		default:
			// rotation is outside of [NO_ROTATION..SIZE_ROTATION] or UNSET_ROTATION
			fprintf(
				stderr, "%sWarning: Rotation setting is not defined. Using rotation type \"%s\" instead.\n%s",
				_level_colors[level_warning], _rotation_strings[0], COLOR_RESET
			);

			_log_rotation = NO_ROTATION;
			break;
	}

	// null terminating file names
	memset(_log_file_to_use, '\0', sizeof(_log_file_to_use));
	memset(_base_log_file, '\0', sizeof(_base_log_file));

	if (file_name == NULL) {                                                                                                       // check, if the file name points to NULL
		fprintf(
			stderr, "%sWarning: Invalid file name detected. Using \"%s\" as file name instead.\n%s",
			_level_colors[level_warning], _default_log_name, COLOR_RESET
		);

		strcpy(_log_file_to_use, _default_log_name);
	} else {
		size_t name_length = strlen(file_name);

		if (name_length > 0 && name_length < LENGTH_FILE_NAME) {                                                                   // check for valid file size length [1..31]
			strcpy(_log_file_to_use, file_name);
		} else {
			fprintf(
				stderr,
				"%sWarning: Invalid length (%d) for file name detected. Using a default file name \"%s\" instead.%s\n",
				_level_colors[level_warning], (int) name_length, _default_log_name, COLOR_RESET
			);
			strcpy(_log_file_to_use, _default_log_name);
		}
	}

	if (_log_rotation != NO_ROTATION) {                                                                                            // special handling for DAILY_ROTATION and SIZE_ROTATION
		if (_size_for_file_size < 1 && _log_rotation == SIZE_ROTATION) {                                                           // check for invalid SIZE_ROTATION setting
			fprintf(
				stderr,
				"%sWarning: Invalid size in MB for for option %s detected. Switching to option \"%s\" instead.%s\n",
				_level_colors[level_warning], _rotation_strings[2], _rotation_strings[0], COLOR_RESET
			);

			_log_rotation = NO_ROTATION;
		}

		if (_nbr_of_keeping_files < 2) {                                                                                           // check for invalid keep_nbr_files setting
			fprintf(
				stderr,
				"%sWarning: invalid number of keeping files detected: %d. Using 2 files to keep up by default.%s\n",
				_level_colors[level_warning], keep_nbr_files, COLOR_RESET
			);
			_nbr_of_keeping_files = 2;
		}
	}

	// in use for dayly rotation
	strcpy(_base_log_file, _log_file_to_use);
	_initializing_done = true;
}

// -----------
// public functions
// -----------

void init_log_by_arguments(const char *file_name, const LogLevel init_level, const LogRotation rotation, int size_in_mb, int keep_nbr_files, bool on_console) {
	_internal_log_initializer(file_name, init_level, rotation, size_in_mb, keep_nbr_files, on_console);
}

void init_log(Logging *log) {
	if (log == NULL) {
		_internal_log_initializer("", LOG_INFO, NO_ROTATION, 0, 0, true);                                                          // redirect the log output to stdout instead
	} else {
		_internal_log_initializer(log->file_name, log->init_level, log->rotation_setting, log->file_size_in_mb, log->nbr_of_keeping_files, log->on_console_only);
	}
}

void write_to_log(LogLevel level, const char* format, ...) {
	if (level < _level_for_logging) {
		// every level, which has a lower value compared to the initial level
		// won't be handled
		return;
	}

	if (!_initializing_done) {
		fprintf(
			stderr, "%sERROR: No log handling is going to do since no init function before has been called.\n%s",
			_level_colors[5], COLOR_RESET
		);
		return;
	}

	char log_line[LENGTH_LOG_MESSAGE];
	memset(log_line, '\0', sizeof(log_line));

	va_list args;
	va_start(args, format);
	vsnprintf(log_line, sizeof(log_line), format, args);
	va_end(args);

	_create_new_timestamp();

	if (_on_console_only) {
		fprintf(
			stdout, "[%s] %s[%s]%s ",
			_timestamp, _level_colors[level], _log_level_to_string(level), COLOR_RESET
		);

		va_list args_2;
		va_start(args_2, format);
		vfprintf(stdout, format, args_2);
		va_end(args_2);

		fprintf(stdout, "\n");
		return;
	}

	_log_file_pointer = fopen(_log_file_to_use, "a");
	bool on_valid_file_pointer = _log_file_pointer != NULL;

	if (!on_valid_file_pointer) {
		fprintf(stderr, "%sERROR: unable to write the log file...%s: %s\n", _level_colors[4], COLOR_RESET, strerror(errno));
		return;
	}

	if (_log_rotation != NO_ROTATION) {
		// close this file stream and check, depending on which rotation is set,
		// if a file rotation is required
		dispose();

		if (_check_for_new_rotation()) {
			(_log_rotation == DAILY_ROTATION) ? _rotate_log_file_daily() : _rotate_log_files();
		}

		_log_file_pointer = fopen(_log_file_to_use, "a");
		on_valid_file_pointer = _log_file_pointer != NULL;
	}

	// handle only logging events, when a file pointer exists
	if (on_valid_file_pointer) {
		fprintf(_log_file_pointer, "[%s] [%s] %s\n", _timestamp, _log_level_to_string(level), log_line);
	}

	dispose();
}

void dispose(void) {
	if (_log_file_pointer != NULL) {
		fclose(_log_file_pointer);
		_log_file_pointer = NULL;
	}
}