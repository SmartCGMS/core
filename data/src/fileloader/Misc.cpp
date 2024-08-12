/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Copyright (c) since 2018 University of West Bohemia.
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
 * 
 * 
 * Purpose of this software:
 * This software is intended to demonstrate work of the diabetes.zcu.cz research
 * group to other scientists, to complement our published papers. It is strictly
 * prohibited to use this software for diagnosis or treatment of any medical condition,
 * without obtaining all required approvals from respective regulatory bodies.
 *
 * Especially, a diabetic patient is warned that unauthorized use of this software
 * may result into severe injure, including death.
 *
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "Misc.h"

#include <sstream>
#include <cstring>

void Discard_BOM_If_Present(std::istream& is) {
	uint8_t bom[4];
	std::memset(bom, 0, 4);

	is.read((char*)bom, 4);

	// EF BB BF / BF BB EF = UTF-8 big/little endian
	if ((bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) ||
		(bom[0] == 0xBF && bom[1] == 0xBB && bom[2] == 0xEF)) {
		is.seekg(-1, std::ios::cur);
		return;
	}
	// FE FF / FF FE = UTF-16 big/little endian
	else if ((bom[0] == 0xFE && bom[1] == 0xFF) ||
		(bom[0] == 0xFF && bom[1] == 0xFE)) {
		is.seekg(-2, std::ios::cur);
		return;
	}
	// 00 00 FE FF / FF FE 00 00 = UTF-32 big/little endian
	else if ((bom[0] == 0x00 && bom[1] == 0x00 && bom[2] == 0xFE && bom[3] == 0xFF) ||
		(bom[0] == 0xFF && bom[1] == 0xFE && bom[2] == 0x00 && bom[3] == 0x00)) {
		return;
	}

	// no BOM in input file, reset istream to original state and return
	is.seekg(-4, std::ios::cur);
}
