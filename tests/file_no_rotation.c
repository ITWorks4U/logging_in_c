#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "logging.h"

#define LOG_MESSAGE "This is a simple message."

int main(void) {
	// Create a new log construction.
	// NOTE: Comes without a rotation.
	//       Every used log event will be
	//       written into that one file.
	//       Furthermore only logging in
	//       [WARN..FATAL] will be handled here.
	Logging log = {
		.on_console_only = false,
		.init_level = LOG_WARNING,
		.file_name = "output.log",
		.rotation_setting = NO_ROTATION,

		// are going to ignore
		.file_size_in_mb = 0,
		.nbr_of_keeping_files = 0
	};

	init_log(&log);

	//NOTE: the created log file will contain 300,001 lines
	for(int i = 0; i < 100000; i++) {
		// won't be handled
		write_to_log(LOG_TRACE, LOG_MESSAGE);
		write_to_log(LOG_DEBUG, LOG_MESSAGE);
		write_to_log(LOG_INFO, LOG_MESSAGE);

		// starts to handle
		write_to_log(LOG_WARNING, LOG_MESSAGE);
		write_to_log(LOG_ERROR, LOG_MESSAGE);
		write_to_log(LOG_FATAL, LOG_MESSAGE);
	}

	dispose();

	return EXIT_SUCCESS;
}