#	makefile for UNIX/Linux/macOS only,
#	create file log_writer.run by including the library folder only
#
#	If you want to create a library for Windows, use the batch file instead.

compiler = gcc
c_flags = -g3 -Wall -Ilib
path_lib = lib/logging.c
destination = log_writer.run

build:
	@$(compiler) $(c_flags) $(path_lib) main.c -o $(destination)
	$(info application built)

clean:
	@rm -f $(destination)
	$(info application removed, if existing)