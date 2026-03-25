#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "logging.h"

#define LOG_MESSAGE "This is a simple message."

int main(void) {
	// Create a new log construction.
	// NOTE: For a rotation, like DAILY_ROTATION or SIZE_ROTATION
	//       the nbr_of_keeping_files must be set with at least 2.
	//       Every value <2 will be set to 2 by default.
	Logging log = {
		.on_console_only = false,
		.init_level = LOG_INFO,
		.file_name = "output.log",
		.rotation_setting = DAILY_ROTATION,
		.nbr_of_keeping_files = 2,

		// is going to ignore
		.file_size_in_mb = 0
	};

	init_log(&log);

	for(int i = 0; i < 10; i++) {
		for(LogLevel mark = LOG_TRACE; mark <= LOG_FATAL; mark++) {
			write_to_log(mark, LOG_MESSAGE);
		}
	}

	dispose();

	/////
	///// second way
	/////

	init_log_by_arguments(
		/*file_name: */ "",                         // defaults to "app.log" + warning message on stderr
		/*init_level: */ LOG_DEBUG,
		/*rotation: */ DAILY_ROTATION,
		/*size_in_mb: */ 0,
		/*keep_nbr_files: */0,                     // defaults to 2 + warning message on stderr
		/*on_console: */ false
	);

	for(int i = 0; i < 10; i++) {
		for(LogLevel mark = LOG_TRACE; mark <= LOG_FATAL; mark++) {
			write_to_log(mark, LOG_MESSAGE);
		}
	}

	dispose();

	return EXIT_SUCCESS;
}