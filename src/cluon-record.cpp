/*
 * Copyright (C) 2019  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cluon-complete.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{0};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if (0 == commandlineArguments.count("cid")) {
        std::cerr << argv[0] << " record Envelopes from a given CID." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --cid=<OpenDaVINCI session to record Envelopes>] [--rec=MyFile.rec] [--recsuffix=Suffix] [--remote]" << std::endl;
        std::cerr << "         --cid:       CID of the OD4Session to receive Envelopes for recording" << std::endl;
        std::cerr << "         --rec:       name of the recording file; default: YYYY-MM-DD_HHMMSS.rec" << std::endl;
        std::cerr << "         --recsuffix: additional suffix to add to the .rec file" << std::endl;
        std::cerr << "         --remote:    listen for cluon.data.RecorderCommand to start/stop recording" << std::endl;
        std::cerr << "         --append:    append to existing files instead of overwriting" << std::endl;
        std::cerr << "Example: " << argv[0] << " --cid=111 --recsuffix=-myfile" << std::endl;
        retCode = 1;
    } else {
        auto getYYYYMMDD_HHMMSS = [](){
            cluon::data::TimeStamp now = cluon::time::now();

            long int _seconds = now.seconds();
            const time_t __seconds = static_cast<time_t>(_seconds);
            struct tm *tm = localtime(&__seconds);

            uint32_t year = (1900 + tm->tm_year);
            uint32_t month = (1 + tm->tm_mon);
            uint32_t dayOfMonth = tm->tm_mday;
            uint32_t hours = tm->tm_hour;
            uint32_t minutes = tm->tm_min;
            uint32_t seconds = tm->tm_sec;

            std::stringstream sstr;
            sstr << year << "-" << ( (month < 10) ? "0" : "" ) << month << "-" << ( (dayOfMonth < 10) ? "0" : "" ) << dayOfMonth
                           << "_" << ( (hours < 10) ? "0" : "" ) << hours
                           << ( (minutes < 10) ? "0" : "" ) << minutes
                           << ( (seconds < 10) ? "0" : "" ) << seconds;

            std::string retVal{sstr.str()};
            return retVal;
        };

        const bool REMOTE{commandlineArguments.count("remote") != 0};
        const bool APPEND{commandlineArguments.count("append") != 0};
        const std::string RECSUFFIX{commandlineArguments["recsuffix"]};
        const std::string REC{(commandlineArguments["rec"].size() != 0) ? commandlineArguments["rec"] : ""};
        const std::string NAME_RECFILE{(commandlineArguments["rec"].size() != 0) ? commandlineArguments["rec"] + RECSUFFIX : (getYYYYMMDD_HHMMSS() + RECSUFFIX + ".rec")};

        std::mutex recFileMutex;
        const auto recFileOpenMode = std::ios::out | std::ios::binary | (APPEND ? std::ios::app : std::ios::trunc);
        std::shared_ptr<std::fstream> recFile{nullptr};
        if (!REMOTE) {
            recFile.reset(new std::fstream(NAME_RECFILE.c_str(), recFileOpenMode));
            std::clog << argv[0] << ": Created " << NAME_RECFILE << "." << std::endl;
        }

        // Interface to a running OpenDaVINCI session (ignoring any incoming Envelopes).
        std::string nameOfRecFile;
        cluon::OD4Session od4Session(static_cast<uint16_t>(std::stoi(commandlineArguments["cid"])),
            [REMOTE, argv, REC, RECSUFFIX, getYYYYMMDD_HHMMSS, &recFileMutex, &recFileOpenMode, &recFile, &nameOfRecFile](cluon::data::Envelope &&envelope) noexcept {
            std::lock_guard<std::mutex> lck(recFileMutex);
            if ((cluon::data::RecorderCommand::ID() == envelope.dataType()) && REMOTE) {
                cluon::data::RecorderCommand rc = cluon::extractMessage<cluon::data::RecorderCommand>(std::move(envelope));
                if (1 == rc.command()) {
                    if (recFile && recFile->good()) {
                        recFile->flush();
                        recFile->close();
                        recFile = nullptr;
                        std::clog << argv[0] << ": Closed " << nameOfRecFile << "." << std::endl;
                    }
                    nameOfRecFile = (REC.size() != 0) ? REC + RECSUFFIX : (getYYYYMMDD_HHMMSS() + RECSUFFIX + ".rec");
                    recFile.reset(new std::fstream(nameOfRecFile.c_str(), recFileOpenMode));
                    std::clog << argv[0] << ": Created " << nameOfRecFile << "." << std::endl;
                }
                else if (2 == rc.command()) {
                    if (recFile && recFile->good()) {
                        recFile->flush();
                        recFile->close();
                        std::clog << argv[0] << ": Closed " << nameOfRecFile << "." << std::endl;
                    }
                    recFile = nullptr;
                }
            }

            if ((cluon::data::RecorderCommand::ID() != envelope.dataType()) && recFile && recFile->good()) {
                (*recFile) << cluon::serializeEnvelope(std::move(envelope));
            }
        });

        // Do flushing in main thread.
        if (od4Session.isRunning()) {
            using namespace std::literals::chrono_literals; // NOLINT
            while (od4Session.isRunning()) {
                {
                    std::lock_guard<std::mutex> lck(recFileMutex);
                    if (recFile && recFile->good()) {
                        recFile->flush();
                    }
                }
                std::this_thread::sleep_for(5s);
            }
        }
    }
    return retCode;
}

