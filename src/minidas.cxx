// minidas: reduce SuperCDMS/CUTE raw data volume by skimming raw MIDAS
// files down to the events surviving a loose preselection on the
// processed RQ files.
//
//   minidas eventlist ...  preselect events from a processed RQ ROOT
//                          file into an eventlist CSV
//   minidas skim ...       write the selected raw data (MIDAS format)
//                          for the events in an eventlist CSV

#include "commands.h"

#include <cstring>
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2 ||
        std::strcmp(argv[1], "-h") == 0 || std::strcmp(argv[1], "--help") == 0) {
        std::cerr <<
            "Usage: minidas <command> [options]\n"
            "Commands:\n"
            "  eventlist   preselect events from a processed RQ file\n"
            "  skim        skim raw MIDAS files per an eventlist\n"
            "Run 'minidas <command>' without options for command usage.\n";
        return argc < 2 ? 1 : 0;
    }

    if (std::strcmp(argv[1], "eventlist") == 0)
        return run_eventlist(argc - 2, argv + 2);
    if (std::strcmp(argv[1], "skim") == 0)
        return run_skim(argc - 2, argv + 2);

    std::cerr << "minidas: unknown command '" << argv[1] << "'\n";
    return 1;
}
