::	batch script to create log_writer.exe including the library folder only
::
::	if this script fails, then you must enter the compile commands into the command line instead
::	=> take a look to the readme.md file
@echo off
setlocal

set DESTINATION=log_writer.exe
set LIB_PATH=lib/logging.c

::	some checks before...
if "%1" == "" goto help_function
if not "%2" == "" goto help_function
if "%1" == "build" goto build_app
if "%1" == "clean" goto clean_up

::	for any other single argument
goto help_function

::	--------------
::	functions
::	--------------
:help_function
echo "usage: makefile.bat [build | clean]"
echo build = build the application
echo clean = removes the application
goto :eof

:build_app
gcc.exe -g3 -Wall -Ilib %LIB_PATH% main.c -o %DESTINATION%
echo application built
goto :eof

:clean_up
del %DESTINATION% 2>&1>nul
echo application removed, if existing
goto :eof

endlocal