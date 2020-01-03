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
 * Univerzitni 8
 * 301 00, Pilsen
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
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#pragma once

#include <cstdint>
#include <Windows.h>

#pragma pack(push, 1)

// packet structure of DMMS --> SmartCGMS
struct TDMMS_To_SmartCGMS
{
	double glucose_ig;
	double glucose_bg;
	double insulin_injection;
	double simulation_time;
	struct
	{
		double grams;
		double time;
	} announced_meal;
	struct
	{
		double intensity;
		double time;
		double duration;
	} announced_excercise;
};

// packet structure of SmartCGMS --> DMMS
struct TSmartCGMS_To_DMMS
{
	double basal_rate;
	double bolus_rate;
	double carbs_rate;
	double device_time;
};

#pragma pack(pop)


struct TDMMS_IPC{
    HANDLE file_DataToSmartCGMS;
    void* filebuf_DataToSmartCGMS;
    HANDLE file_DataFromSmartCGMS;
    void* filebuf_DataFromSmartCGMS;
    HANDLE event_DataToSmartCGMS;
    HANDLE event_DataFromSmartCGMS;
};

TDMMS_IPC Establish_DMMS_IPC(const DWORD process_id);
void Clear_DMMS_IPC(TDMMS_IPC& dmms_ipc);
void Release_DMMS_IPC(TDMMS_IPC &dmms_ipc);