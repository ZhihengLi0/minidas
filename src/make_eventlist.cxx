// make_eventlist: apply a loose preselection to a cdmsbats processed RQ
// ROOT file and write the surviving events to an eventlist CSV.
//
// This is a C++ port of the original Filter.ipynb: it reads the parallel
// trees rqDir/eventTree (EventNumber, DumpNumber, EventTime, EventType)
// and rqDir/zip1 (PTOFamps), keeps events with EventType == <type> and
// ampMin < PTOFamps < ampMax, and writes:
//   EventNumber,DumpNumber,EventTime,EventType,PTOFamps
//
// Usage:
//   make_eventlist.exe -i processed.root -o eventlist.csv
//                      [--amp-min 4e-6] [--amp-max 9e-6] [--type 1]
//                      [--zip zip1]

#include <TFile.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

namespace {

void usage() {
    std::cerr <<
        "Usage: make_eventlist.exe -i <processed.root> -o <eventlist.csv>\n"
        "         [--amp-min <v>] [--amp-max <v>] [--type <n>] [--zip <name>]\n"
        "Defaults: --amp-min 4e-6 --amp-max 9e-6 --type 1 --zip zip1\n";
    std::exit(1);
}

} // namespace

int main(int argc, char** argv) {
    std::string inFile, outFile, zipName = "zip1";
    double ampMin = 4e-6, ampMax = 9e-6;
    int eventType = 1;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if      (arg == "-i"        && i + 1 < argc) inFile  = argv[++i];
        else if (arg == "-o"        && i + 1 < argc) outFile = argv[++i];
        else if (arg == "--amp-min" && i + 1 < argc) ampMin  = std::stod(argv[++i]);
        else if (arg == "--amp-max" && i + 1 < argc) ampMax  = std::stod(argv[++i]);
        else if (arg == "--type"    && i + 1 < argc) eventType = std::stoi(argv[++i]);
        else if (arg == "--zip"     && i + 1 < argc) zipName = argv[++i];
        else usage();
    }
    if (inFile.empty() || outFile.empty()) usage();

    TFile* f = TFile::Open(inFile.c_str(), "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "Cannot open " << inFile << '\n';
        return 1;
    }

    TTreeReader evtReader("rqDir/eventTree", f);
    TTreeReaderValue<double> eventNumber(evtReader, "EventNumber");
    TTreeReaderValue<double> dumpNumber (evtReader, "DumpNumber");
    TTreeReaderValue<double> eventTime  (evtReader, "EventTime");
    TTreeReaderValue<double> eventTypeV (evtReader, "EventType");

    TTreeReader zipReader(("rqDir/" + zipName).c_str(), f);
    TTreeReaderValue<double> ptofAmps(zipReader, "PTOFamps");

    std::ofstream fout(outFile);
    if (!fout) {
        std::cerr << "Cannot open " << outFile << " for writing\n";
        return 1;
    }
    fout << "EventNumber,DumpNumber,EventTime,EventType,PTOFamps\n";
    fout.precision(17);

    // eventTree and the zip tree are parallel (one entry per event),
    // so they are iterated in lockstep.
    size_t total = 0, kept = 0;
    while (evtReader.Next() && zipReader.Next()) {
        ++total;
        const double amp = *ptofAmps;
        if (static_cast<int>(*eventTypeV) == eventType &&
            amp > ampMin && amp < ampMax) {
            fout << *eventNumber << ',' << *dumpNumber << ',' << *eventTime
                 << ',' << *eventTypeV << ',' << amp << '\n';
            ++kept;
        }
    }

    fout.close();
    f->Close();

    std::cout << "Kept " << kept << " / " << total << " events -> "
              << outFile << '\n';
    return 0;
}
