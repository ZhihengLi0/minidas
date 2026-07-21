// make_eventlist: apply a loose preselection to a cdmsbats processed RQ
// ROOT file and write the surviving events to an eventlist CSV.
//
// This is a C++ port of the original Filter.ipynb: it reads the parallel
// trees rqDir/eventTree (EventNumber, EventTime, EventType, and
// DumpNumber if present) and rqDir/<zip> (PTOFamps), keeps events with
// EventType == <type> and ampMin < PTOFamps < ampMax, and writes:
//   EventNumber,DumpNumber,EventTime,EventType,PTOFamps
//
// Per-dump RQ files have no DumpNumber branch; in that case the dump is
// derived from the Soudan-style numbering EventNumber = 10000*dump + idx
// that cdmsbats/CDMSIOLIB assigns (see midas_file_reader.cxx).
//
// Usage:
//   make_eventlist.exe -i processed.root -o eventlist.csv
//                      [--amp-min 4e-6] [--amp-max 9e-6] [--type 1]
//                      [--zip zip1]

#include <TFile.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>

#include <iostream>
#include <fstream>
#include <memory>
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

    TTree* evtTree = f->Get<TTree>("rqDir/eventTree");
    TTree* zipTree = f->Get<TTree>(("rqDir/" + zipName).c_str());
    if (!evtTree) {
        std::cerr << "No rqDir/eventTree in " << inFile << '\n';
        return 1;
    }
    if (!zipTree) {
        std::cerr << "No rqDir/" << zipName << " in " << inFile << '\n';
        return 1;
    }
    for (const char* b : {"EventNumber", "EventTime", "EventType"}) {
        if (!evtTree->GetBranch(b)) {
            std::cerr << "eventTree has no branch " << b << '\n';
            return 1;
        }
    }
    if (!zipTree->GetBranch("PTOFamps")) {
        std::cerr << zipName << " has no branch PTOFamps\n";
        return 1;
    }
    if (evtTree->GetEntries() != zipTree->GetEntries()) {
        std::cerr << "eventTree (" << evtTree->GetEntries() << " entries) and "
                  << zipName << " (" << zipTree->GetEntries()
                  << " entries) are not parallel trees\n";
        return 1;
    }
    const bool hasDumpBranch = evtTree->GetBranch("DumpNumber") != nullptr;
    if (!hasDumpBranch)
        std::cout << "No DumpNumber branch; deriving dump as EventNumber/10000\n";

    TTreeReader evtReader(evtTree);
    TTreeReaderValue<double> eventNumber(evtReader, "EventNumber");
    TTreeReaderValue<double> eventTime  (evtReader, "EventTime");
    TTreeReaderValue<double> eventTypeV (evtReader, "EventType");
    std::unique_ptr<TTreeReaderValue<double>> dumpNumber;
    if (hasDumpBranch)
        dumpNumber = std::make_unique<TTreeReaderValue<double>>(evtReader, "DumpNumber");

    TTreeReader zipReader(zipTree);
    TTreeReaderValue<double> ptofAmps(zipReader, "PTOFamps");

    std::ofstream fout(outFile);
    if (!fout) {
        std::cerr << "Cannot open " << outFile << " for writing\n";
        return 1;
    }
    fout << "EventNumber,DumpNumber,EventTime,EventType,PTOFamps\n";
    fout.precision(17);

    size_t total = 0, kept = 0;
    while (evtReader.Next() && zipReader.Next()) {
        ++total;
        const int dump = hasDumpBranch
            ? static_cast<int>(**dumpNumber)
            : static_cast<int>(*eventNumber) / 10000;
        const double amp = *ptofAmps;
        if (static_cast<int>(*eventTypeV) == eventType &&
            amp > ampMin && amp < ampMax) {
            fout << *eventNumber << ',' << dump << ',' << *eventTime
                 << ',' << *eventTypeV << ',' << amp << '\n';
            ++kept;
        }
    }

    const auto expected = static_cast<size_t>(evtTree->GetEntries());
    fout.close();
    f->Close();

    if (total != expected) {
        std::cerr << "ERROR: processed " << total << " of " << expected
                  << " entries (read failure?)\n";
        return 1;
    }

    std::cout << "Kept " << kept << " / " << total << " events -> "
              << outFile << '\n';
    return 0;
}
