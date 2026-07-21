#include "midas_file_reader.h"
#include "midas_file_writer.h"
#include "io_exceptions.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unordered_set>
#include <map>
#include <vector>

using namespace CDMSIOLIB;

/************************************************************
 * Helper                                                   *
 ************************************************************/
void usage() {
    std::cerr << "Usage:  AddSalt.exe <output_midas_file>\n";
    exit(1);
}

int main(int argc, char** argv) {
    const bool   debug       = true;   // Toggle verbose output
    const size_t probeLines  = 10;     // Print first N events per dump for sanity‑check

    if (argc != 2) usage();
    const std::string outFile = argv[1];

    /* Path template for the raw MIDAS files. Adapt if your run ID changes. */
    const std::string inPrefix =
        "/project/rrg-mdiamond/data/CDMS/CUTE/R37/Raw/23231222_074513/23231222_074513_";
    const std::string csvFile  = "filtered2_events.csv";

    /************************************************************
     * Read CSV: collect global EventNumbers and group by dump  *
     ************************************************************/
    std::unordered_set<uint32_t> keepEvents;                     // All global event numbers we care about
    std::map<int, std::vector<uint32_t>> eventsByDump;           // dump -> list of global events (for stats)

    {
        std::ifstream fin(csvFile);
        if (!fin) {
            std::cerr << "Cannot open " << csvFile << '\n';
            return 1;
        }
        std::string line;
        std::getline(fin, line);                                 // Skip header
        while (std::getline(fin, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string tok;

            std::getline(ss, tok, ',');                          // EventNumber (global)
            const uint32_t globalNum = static_cast<uint32_t>(std::stod(tok));

            std::getline(ss, tok, ',');                          // DumpNumber
            const int dump = static_cast<int>(std::stod(tok));

            keepEvents.insert(globalNum);
            eventsByDump[dump].push_back(globalNum);
        }
    }

    if (keepEvents.empty()) {
        std::cerr << "No valid events found in CSV.\n";
        return 1;
    }

    if (debug) {
        std::cout << "Parsed " << keepEvents.size() << " unique events spanning "
                  << eventsByDump.size() << " dump files:\n";
        for (const auto& [dumpNo, v] : eventsByDump) {
            std::cout << "  Dump " << dumpNo << ": " << v.size() << " target events\n";
        }
    }

    /************************************************************
     * Prepare output file                                    *
     ************************************************************/
    MidasFileWriter writer;
    writer.OpenFile(outFile);
    if (debug) std::cout << "Output file opened: " << outFile << "\n";

    bool   odbWritten   = false;
    size_t totalWritten = 0;

    /************************************************************
     * Iterate over dumps                                     *
     ************************************************************/
    for (const auto& [dumpNo, globals] : eventsByDump) {
        // Compose the raw MIDAS filename for this dump
        std::ostringstream path;
        path << inPrefix << "F" << std::setw(4) << std::setfill('0') << dumpNo << ".mid.gz";
        const std::string inFile = path.str();

        std::cout << "\n>> Processing Dump " << dumpNo << ": " << inFile << '\n';

        MidasFileReader reader;
        try {
            reader.OpenFile(inFile);
        } catch (...) {
            std::cerr << "ERROR: Cannot open " << inFile << '\n';
            return 1;
        }

        // Copy ODB only once (from the first dump)
        if (!odbWritten) {
            writer.WriteOdb(reader.GetFullODBAsXML());
            odbWritten = true;
            if (debug) std::cout << "ODB copied to output file.\n";
        }

        size_t readCnt = 0, writeCnt = 0;
        try {
            while (true) {
                CDMS_EVENT& ev = reader.GetNextEvent();
                ++readCnt;

                // Mapping: global = local + dump*10000 - 10000
                const uint32_t global = ev.eventNumber + dumpNo * 10000 - 10000;

                if (readCnt <= probeLines && debug) {
                    std::cout << "  raw ev.eventNumber = " << ev.eventNumber
                              << "  ->  global = " << global << '\n';
                }

                if (keepEvents.count(global)) {
                    writer.WriteEvent(ev);
                    ++writeCnt;
                    ++totalWritten;
                    if (debug) std::cout << "    ** wrote global=" << global << '\n';
                }
            }
        } catch (NoMoreEventsException&) {
            // Reached end of this dump file
        }

        reader.CloseFile();
        std::cout << "Dump " << dumpNo << " read " << readCnt
                  << " events, wrote " << writeCnt << " matches.\n";
    }

    writer.CloseFile();
    std::cout << "\n=== Finished === wrote " << totalWritten << " events.\n";

    if (totalWritten != keepEvents.size()) {
        std::cout << "⚠  Mismatch: output events (" << totalWritten
                  << ") != CSV unique events (" << keepEvents.size() << ").\n";
    }

    return 0;
}
