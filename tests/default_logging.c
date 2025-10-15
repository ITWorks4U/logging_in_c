#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "logging.h"

#define LOG_MESSAGE "This is a simple message."

int main(void) {
	// Create a new log construction.
	// Since on_console_only flag is set to true, it doesn't matter,
	// if additional file settings, like file name, rotation setting, ... is/are
	// given or not. These arguments are ignored.
	Logging log = {
		.on_console_only = true,
		.init_level = LOG_TRACE,

		// are going to ignore
		.file_name = "output.log",
		.rotation_setting = NO_ROTATION,
		.file_size_in_mb = 0,
		.nbr_of_keeping_files = 0
	};

	init_log(&log);
	log_message(LOG_TRACE, LOG_MESSAGE);
	log_message(LOG_DEBUG, LOG_MESSAGE);
	log_message(LOG_INFO, LOG_MESSAGE);
	log_message(LOG_WARNING, LOG_MESSAGE);
	log_message(LOG_ERROR, LOG_MESSAGE);
	log_message(LOG_FATAL, LOG_MESSAGE);
	dispose_logging();

	return EXIT_SUCCESS;
}