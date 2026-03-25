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
		.rotation_setting = SIZE_ROTATION,
		.nbr_of_keeping_files = 5,                // keep only 5 files
		.file_size_in_mb = 10                     // each log file has a maximum size of 10MB
	};

	init_log(&log);

	for(int i = 0; i < 500000; i++) {            // repeat 500,000 times
		for(LogLevel mark = LOG_TRACE; mark <= LOG_FATAL; mark++) {
			write_to_log(mark, LOG_MESSAGE);
		}
	}

	dispose();

	return EXIT_SUCCESS;
}