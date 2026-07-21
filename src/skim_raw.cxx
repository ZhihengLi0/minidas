// skim_raw: write a "selected raw data" MIDAS file containing only the
// events listed in an eventlist CSV (produced by make_eventlist).
//
// The eventlist CSV must have a header line and columns starting with:
//   EventNumber,DumpNumber,...
// EventNumber is the global (Soudan-style) event number as stored in the
// processed RQ files. Event numbering is delegated to CDMSIOLIB itself:
// after MidasFileReader::SetDumpNumber(dump), the reader assigns each
// event the same eventNumber cdmsbats used when producing the RQ file
// (see midas_file_reader.cxx), so no numbering convention is hardcoded
// here. Matching is done per dump, so a numbering overflow in one dump
// cannot steal events from another dump's list.
//
// Usage:
//   skim_raw.exe -e eventlist.csv -i <raw_prefix> -o out.mid.gz [-q]
// where <raw_prefix> is the raw series path up to the dump index, e.g.
//   /path/to/Raw/23231222_074513/23231222_074513_
// so that dump N is found at <raw_prefix>F000N.mid.gz

#include "midas_file_reader.h"
#include "midas_file_writer.h"
#include "io_exceptions.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <unordered_set>
#include <map>
#include <vector>

using namespace CDMSIOLIB;

namespace {

void usage() {
    std::cerr <<
        "Usage: skim_raw.exe -e <eventlist.csv> -i <raw_prefix> -o <output.mid.gz> [-q]\n"
        "  -e  eventlist CSV (header + EventNumber,DumpNumber,... columns)\n"
        "  -i  raw MIDAS path prefix, dump N is read from <prefix>F%04d.mid.gz\n"
        "  -o  output MIDAS file\n"
        "  -q  quiet (suppress per-event output)\n";
    std::exit(1);
}

} // namespace

int main(int argc, char** argv) {
    std::string csvFile, inPrefix, outFile;
    bool verbose = true;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if      (arg == "-e" && i + 1 < argc) csvFile  = argv[++i];
        else if (arg == "-i" && i + 1 < argc) inPrefix = argv[++i];
        else if (arg == "-o" && i + 1 < argc) outFile  = argv[++i];
        else if (arg == "-q") verbose = false;
        else usage();
    }
    if (csvFile.empty() || inPrefix.empty() || outFile.empty()) usage();

    // Read the eventlist: global event numbers, grouped by dump.
    size_t nSelected = 0;
    std::map<int, std::unordered_set<uint32_t>> eventsByDump;
    {
        std::ifstream fin(csvFile);
        if (!fin) {
            std::cerr << "Cannot open " << csvFile << '\n';
            return 1;
        }
        std::string line;
        std::getline(fin, line); // header
        while (std::getline(fin, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string tok;

            std::getline(ss, tok, ',');
            const uint32_t globalNum = static_cast<uint32_t>(std::stod(tok));
            std::getline(ss, tok, ',');
            const int dump = static_cast<int>(std::stod(tok));

            if (eventsByDump[dump].insert(globalNum).second) ++nSelected;
        }
    }
    if (nSelected == 0) {
        std::cerr << "No valid events found in " << csvFile << '\n';
        return 1;
    }
    std::cout << "Eventlist: " << nSelected << " unique events in "
              << eventsByDump.size() << " dumps\n";

    MidasFileWriter writer;
    writer.OpenFile(outFile);

    bool   odbWritten   = false;
    size_t totalWritten = 0;

    for (const auto& [dumpNo, keep] : eventsByDump) {
        std::ostringstream path;
        path << inPrefix << "F" << std::setw(4) << std::setfill('0')
             << dumpNo << ".mid.gz";
        const std::string inFile = path.str();
        std::cout << ">> Dump " << dumpNo << ": " << inFile
                  << " (" << keep.size() << " target events)\n";

        MidasFileReader reader;
        try {
            reader.OpenFile(inFile);
        } catch (...) {
            std::cerr << "ERROR: cannot open " << inFile << '\n';
            return 1;
        }
        // Have the reader assign the same Soudan-style event numbers
        // cdmsbats assigned when it produced the RQ file for this dump.
        reader.SetDumpNumber(dumpNo);

        // The ODB (detector/DAQ configuration) is copied once from the
        // first dump so the output is a self-contained MIDAS file.
        if (!odbWritten) {
            writer.WriteOdb(reader.GetFullODBAsXML());
            odbWritten = true;
        }

        size_t readCnt = 0, writeCnt = 0;
        try {
            while (true) {
                CDMS_EVENT& ev = reader.GetNextEvent();
                ++readCnt;
                if (verbose && readCnt <= 5)
                    std::cout << "   probe: eventNumber=" << ev.eventNumber << '\n';
                if (keep.count(ev.eventNumber)) {
                    writer.WriteEvent(ev);
                    ++writeCnt;
                    ++totalWritten;
                    if (verbose)
                        std::cout << "   wrote " << ev.eventNumber << '\n';
                }
            }
        } catch (NoMoreEventsException&) {
            // end of dump
        }

        reader.CloseFile();
        std::cout << "   read " << readCnt << ", wrote " << writeCnt << '\n';
    }

    writer.CloseFile();
    std::cout << "Done: wrote " << totalWritten << " events to " << outFile << '\n';

    if (totalWritten != nSelected)
        std::cout << "WARNING: output events (" << totalWritten
                  << ") != eventlist events (" << nSelected << ")\n";

    return 0;
}
