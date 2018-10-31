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

#include "../../../common/iface/FilterIface.h"
#include "../../../common/rtl/DeviceLib.h"
#include "../../../common/rtl/UILib.h"
#include "../../../common/rtl/rattime.h"

#include <set>
#include <map>
#include <algorithm>
#include <numeric>

/*
 * Standalone class for calculating error metrics
 */

using TError_Array = std::array<glucose::TError_Markers, static_cast<size_t>(glucose::NError_Type::count)>;	//without this, ICC 18 Update 2 won't compile the project

class CError_Marker_Counter
{
	private:

		struct TValue
		{
			double time;
			double value;
		};

		using TValueVector = std::vector<TValue>;
		using TSignalValueVector = std::map<GUID, TValueVector>;
		using TSegmentSignalMap = std::map<uint64_t, TSignalValueVector>;

		// segment reference signal values stored; primary key = segment id, secondary key = signal guid, value = vector of values
		TSegmentSignalMap mReferenceValues;
		// segment calculated signal values stored; primary key = segment id, secondary key = signal guid, value = vector of values
		TSegmentSignalMap mCalculatedValues;

		std::set<GUID> mPresentCalculatedSignalIds;

		// calculated errors of all known types; primary key = signal guid, value = array of errors by type
		//std::map<GUID, glucose::TError_Container[glucose::Error_Type_Count]> mErrors;
		std::map<GUID, TError_Array> mErrors;

		// map of signals and their models parameter counts (used for i.e. AIC)
		std::map<GUID, size_t> mSignalModelParamCount;

		// mapping of calculated-reference signal (the errors are calculated against reference signals)
		std::map<GUID, GUID> Reference_For_Calculated_Signal;
	public:
		CError_Marker_Counter();

		// adds level to segment
		bool Add_Level(uint64_t segment_id, const GUID& signal_id, double time, double level);
		// recalculates errors for all signals
		bool Recalculate_Errors();
		// recalculates error for given signal
		bool Recalculate_Errors_For(const GUID& signal_id);
		// updates parameters of given signal (model)
		void Reset_Segment(uint64_t segment_id, const GUID& signal_id);
		// resets the whole instance
		void Reset_All();
		
		// retrieves error record
		HRESULT Get_Errors(const GUID &signal_id, const glucose::NError_Type type, glucose::TError_Markers &markers);
};
