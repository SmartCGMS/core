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

#include "random_playback.h"

#include <scgms/rtl/FilesystemLib.h>

#include <fstream>

#include <random>
#include <limits>
#include <bit>

//#include <intrin.h>

#undef max

void Try_Add_Random_Numbers(const std::string& path, std::vector<CRandom_Playback::result_type> &numbers) {
	std::ifstream random_block(path, std::ios::binary);
	if (random_block.is_open()) {

		random_block.seekg(0, random_block.end);
		const size_t block_size = random_block.tellg() / sizeof(CRandom_Playback::result_type);
		random_block.seekg(0, random_block.beg);

		const size_t current_numbers_size = numbers.size();
		numbers.resize(current_numbers_size + block_size);

		random_block.read(reinterpret_cast<char*>(numbers.data() + current_numbers_size), block_size* sizeof(CRandom_Playback::result_type));
	}
}

std::vector<CRandom_Playback::result_type> Load_All_The_Numbers()  {
	std::vector<CRandom_Playback::result_type> result;

	const auto random_file_list_path = Get_Dll_Dir() / "random_files.list";
	if (Is_Regular_File_Or_Symlink(random_file_list_path)) {
		std::ifstream random_file_list{ random_file_list_path.string() };

		if (random_file_list.is_open()) {
			std::string file_path;

			while (std::getline(random_file_list, file_path)) {
				if (Is_Regular_File_Or_Symlink(file_path)) {
					Try_Add_Random_Numbers(file_path, result);
				}
			}
		}
	}
	
	if (result.empty()) {		//avoid repetitive testing if at least one random number was loaded 
		result.push_back(0);	//and if not, return the default value that's apparently invalid								
	}

	return result;
}


CRandom_Playback::CRandom_Playback() : mNumbers(Load_All_The_Numbers()) {	//move would prevent copy ellision
	std::random_device rd;
	std::uniform_int_distribution<size_t> dist(0.0, mNumbers.size());
	mRecent_Number_Index = dist(rd); 	
	mCurrent_Number_Index = dist(rd); 	//choose pseudo random starting index
}


CRandom_Playback::result_type CRandom_Playback::operator()() {
	const auto number = mNumbers[mCurrent_Number_Index];
	
	//you know, it would be just nice to switch to C++20 and use the std::popcount
	//but, we used Eigen, which does not compile under C++20...
	//so, we either need to replace Eigen, or wait if they adapt. so far, we wait..
	//const auto jump_distance = _mm_popcnt_u64(mRecent_Number_Index ^ mCurrent_Number_Index) % 7;	//7 just feels right
	const auto jump_distance = 1+(mRecent_Number_Index + mCurrent_Number_Index) % 7;	//7 just feels right
			//popcnt problems all the way, let's do it this way
	//const auto jump_distance = std::popcount(mRecent_Number_Index ^ mCurrent_Number_Index) % 7;	//7 just feels right
		//we combine two recent indexes to avoid a trivial repeating of the produced series
	
	mRecent_Number_Index = mCurrent_Number_Index;
	mCurrent_Number_Index += jump_distance;
	if (mCurrent_Number_Index >= mNumbers.size()) {
		mCurrent_Number_Index = 0;
		
		std::random_device rd;
		std::uniform_int_distribution<size_t> dist(0.0, mNumbers.size());
		mRecent_Number_Index = dist(rd);
	}	

	return number;
}

