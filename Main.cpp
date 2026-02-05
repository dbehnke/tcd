// tcd - a hybrid transcoder using DVSI hardware and Codec2 software
// Copyright © 2021,2023 Thomas A. Early N7TAE
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <unistd.h>
#include <iostream>
#include <string>
#include <cctype>

#include "Controller.h"
#include "Configure.h"

#ifndef BUILD_VERSION
#define BUILD_VERSION "dev"
#endif

// the global objects
CConfigure  g_Conf;
CController g_Cont;

int main(int argc, char *argv[])
{
	// Simple CLI parsing: allow `--version`, optional `--mode` and the INI path
	std::string iniPath;
	std::string modeStr = "auto";

	for (int i = 1; i < argc; ++i) {
		std::string a(argv[i]);
		if (a == "--version") {
			std::cout << "tcd " << BUILD_VERSION << std::endl;
			return EXIT_SUCCESS;
		}
		if (a.rfind("--mode=", 0) == 0) {
			modeStr = a.substr(7);
			continue;
		}
		if (a == "--mode" && i + 1 < argc) {
			modeStr = argv[++i];
			continue;
		}
		if (a.size() > 0 && a[0] == '-') {
			std::cerr << "Unknown option: " << a << std::endl;
			return EXIT_FAILURE;
		}
		// first non-option arg is treated as ini path
		if (iniPath.empty()) iniPath = a;
	}

	if (iniPath.empty()) {
		std::cerr << "ERROR: Usage: " << argv[0] << " [--mode auto|hardware|software] PATHTOINIFILE" << std::endl;
		return EXIT_FAILURE;
	}

	// Set runtime vocoder mode
	for (auto &c : modeStr) c = tolower(c);
	if (modeStr == "hardware" || modeStr == "hw") {
		CController::g_VocoderMode = CController::EVocoderMode::Hardware;
	} else if (modeStr == "software" || modeStr == "sw") {
		CController::g_VocoderMode = CController::EVocoderMode::Software;
	} else {
		CController::g_VocoderMode = CController::EVocoderMode::Auto;
	}

	if (g_Conf.ReadData(iniPath))
		return EXIT_FAILURE;

	if (g_Cont.Start())
		return EXIT_FAILURE;

	std::cout << "Hybrid Transcoder version " << BUILD_VERSION << " successfully started" << std::endl;

	pause();

	g_Cont.Stop();

	return EXIT_SUCCESS;
}
