#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "logging.h"

int main(int argc, char **argv) {
	if (argc == 2 && strcmp(argv[1], "-v") == 0) {
		printf("current version: %s\n", CURRENT_VERSION);
	}

	// NOTE: This main file doesn't comes with any log implementations.
	//       This is just a dummy file to allow to build the runable file.
	return EXIT_SUCCESS;
}