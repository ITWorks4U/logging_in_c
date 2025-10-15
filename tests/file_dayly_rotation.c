#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "logging.h"

#define LOG_MESSAGE "This is a simple message."

int main(void) {
	// Create a new log construction.
	// NOTE: For a rotation, like DAYLY_ROTATION or SIZE_ROTATION
	//       the nbr_of_keeping_files must be set with at least 2.
	//       Every value <2 will be set to 2 by default.
	Logging log = {
		.on_console_only = false,
		.init_level = LOG_TRACE,
		.file_name = "output.log",
		.rotation_setting = DAYLY_ROTATION,
		.nbr_of_keeping_files = 2,

		// is going to ignore
		.file_size_in_mb = 0
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