#ifndef MINIDAS_COMMANDS_H
#define MINIDAS_COMMANDS_H

// Subcommand entry points. Each receives the arguments after the
// subcommand name (argv[0] is the first subcommand argument).
int run_eventlist(int argc, char** argv);
int run_skim(int argc, char** argv);

#endif
