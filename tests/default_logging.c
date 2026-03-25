#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "logging.h"

#define LOG_MESSAGE "This is a simple message."

int main(void) {
	// no log session can be done, since the init function(s) hasn't been
	// triggered earlier
	//
	// on stderr an error message, highlighted with red color, is going to display
	write_to_log(LOG_INFO, LOG_MESSAGE);


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

	for(int level = LOG_TRACE; level <= LOG_FATAL; level++) {
		write_to_log(level, LOG_MESSAGE);
	}

	dispose();

	/////
	///// using second init function instead
	/////

	init_log_by_arguments(
		/*file_name: */NULL,
		/*init_level: */ LOG_DEBUG,
		/*rotation: */ NO_ROTATION,
		/*size_in_mb: */ 0,
		/*keep_nbr_files: */0,
		/*on_console: */ true
	);

	for(int i = 0; i < 25; i++) {
		puts("\n------------");

		for(LogLevel level = LOG_TRACE; level <= LOG_FATAL; level++) {
			write_to_log(level, LOG_MESSAGE);
		}
	}

	dispose();

	/////
	///// third way
	/////
	init_log(NULL);                            // in that case the output moves to stdout without any file operation setting

	for(int i = 0; i < 25; i++) {
		puts("\n------------");

		for(LogLevel level = LOG_TRACE; level <= LOG_FATAL; level++) {
			write_to_log(level, LOG_MESSAGE);
		}
	}

	dispose();

	return EXIT_SUCCESS;
}