// minidas inspect: print per-event metadata of a MIDAS file, including
// the provenance fields written by `minidas skim` (original event number
// in SIMRecoilEnergy, original dump number in SIMSeriesNumber).

#include "commands.h"

#include "midas_file_reader.h"
#include "io_exceptions.h"

#include <iostream>
#include <string>
#include <cstdlib>

using namespace CDMSIOLIB;

namespace {

int usage() {
    std::cerr <<
        "Usage: minidas inspect -i <file.mid.gz> [-n <max_events>]\n"
        "Prints one line per event:\n"
        "  index triggerID triggerType origEventNumber(SIMRecoilEnergy) origDump(SIMSeriesNumber)\n";
    return 1;
}

} // namespace

int run_inspect(int argc, char** argv) {
    std::string inFile;
    long maxEvents = -1;

    for (int i = 0; i < argc; ++i) {
        const std::string arg = argv[i];
        if      (arg == "-i" && i + 1 < argc) inFile    = argv[++i];
        else if (arg == "-n" && i + 1 < argc) maxEvents = std::stol(argv[++i]);
        else return usage();
    }
    if (inFile.empty()) return usage();

    MidasFileReader reader;
    try {
        reader.OpenFile(inFile);
    } catch (...) {
        std::cerr << "ERROR: cannot open " << inFile << '\n';
        return 1;
    }

    std::cout << "index,triggerID,triggerType,origEventNumber,origDumpNumber\n";
    long count = 0;
    try {
        while (maxEvents < 0 || count < maxEvents) {
            CDMS_EVENT& ev = reader.GetNextEvent();
            std::cout << count << ',' << ev.triggerID << ',' << ev.triggerType
                      << ',' << static_cast<long long>(ev.SIMRecoilEnergy)
                      << ',' << ev.SIMSeriesNumber << '\n';
            ++count;
        }
    } catch (NoMoreEventsException&) {
        // end of file
    }

    reader.CloseFile();
    std::cerr << "Total events read: " << count << '\n';
    return 0;
}
