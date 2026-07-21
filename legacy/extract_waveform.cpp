#include <rawio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>

int main() {
    std::string midas_path = "/project/rrg-mdiamond/data/CDMS/CUTE/R37/Raw/23231217_135018";
    std::string csv_path = "filtered_events.csv";

    //open csv, get DumpNumber and EventNumber
    std::ifstream infile(csv_path);
    if (!infile.is_open()) {
        std::cerr << "Failed to open CSV file: " << csv_path << std::endl;
        return 1;
    }

    std::vector<std::pair<int, int>> selected_events;
    std::string line;
    std::getline(infile, line); // skip header
    while (std::getline(infile, line)) {
        std::stringstream ss(line);
        std::string token;
        int event_number, dump_number;

        std::getline(ss, token, ',');
        event_number = std::stoi(token);
        std::getline(ss, token, ',');
        dump_number = std::stoi(token);

        selected_events.emplace_back(dump_number, event_number);
    }

    infile.close();
    std::cout << "✅ Loaded " << selected_events.size() << " events from CSV.\n";

    //open midas raw data
    rawio::RawReader reader(midas_path);
    if (!reader.IsOpen()) {
        std::cerr << "Failed to open midas file: " << midas_path << std::endl;
        return 1;
    }

    std::ofstream wfout("matched_waveforms.csv");
    wfout << "DumpNumber,EventNumber,Channel,SampleIndex,SampleValue\n";

    //run raw data
    size_t match_count = 0;
    while (reader.Next()) {
        int dump = reader.DumpNumber();
        int evt = reader.EventNumber();

        for (const auto& pair : selected_events) {
            if (pair.first == dump && pair.second == evt) {
                match_count++;

                int ch = 0;
                auto* wf = reader.Waveform(ch);
                if (wf) {
                    for (size_t i = 0; i < wf->size(); ++i) {
                        wfout << dump << "," << evt << "," << ch << "," << i << "," << wf->at(i) << "\n";
                    }
                }

                break; // matched, skip to next raw event
            }
        }
    }

    wfout.close();
    std::cout << "✅ Extracted waveforms for " << match_count << " matched events.\n";

    return 0;
}
