//##########################################################################
//
// Struct containing webserver command-line options, plus function for parsing
// the command-line args and populating the struct
//
//##########################################################################

#ifndef PROGRAM_OPTIONS_H
#define PROGRAM_OPTIONS_H

#include <stdbool.h>
#include "webserver_config.h"

// ProgramOptions struct
typedef struct ProgramOptions {
  WebServerConfig config;
  bool            help;
} ProgramOptions;

// Flag to tell program_options_parse not to print error messages, For testing.
// TODO - get rid of this, write error message to ProgramOptions struct
extern bool silence_program_options_parse;

// Get the program options usage string
const char* program_options_usage();

// Pring the program options to stdout
void program_options_print(ProgramOptions* options);

// Parse program options from the command-line inputs
bool program_options_parse(ProgramOptions* options, int argc, char** argv);

#endif //PROGRAM_OPTIONS_H
