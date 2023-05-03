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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#pragma once

#include "../../../../common/utils/drawing/SVGRenderer.h"
#include "../../../../common/rtl/rattime.h"
#include "../../../../common/iface/FilterIface.h"
#include "../../../../common/utils/winapi_mapping.h"

#include <string>
#include <map>
#include <set>
#include <vector>
#include <iomanip>

#include <ctime>
#include <clocale>

// define localtime function depending on compiler to avoid unnecessary warnings
#ifdef _MSC_VER
#define _localtime(pTm, tval) localtime_s(pTm, &tval)
#else
#define _localtime(pTm, tval) pTm = localtime(&tval)
#endif

// pair of doubles in 2D plot
struct TPlot_Value {
	double device_time;
	double value;
};

// stored signal structure
struct TPlot_Signal {
	GUID signal_id;
	std::vector<TPlot_Value> mPlots_Values;
};

// stored segment structure
struct TPlot_Segment {
	uint64_t segment_id;
	std::map<GUID, TPlot_Signal> mPlots_Signals;
};

// enumerator of known drawing errors
enum class NDrawing_Error
{
	Ok = 0,
	Unspecified_Error,
	Invalid_Options,
	Not_Enough_Values,

	// TODO: more error codes
};

// "local" version of drawing options to allow non-interop processing (C++)
struct TDraw_Options_Local {

	std::vector<uint64_t> segment_ids;

	// purposedly an "ordered" set; the ordering matches the ordering in reference_signal_ids set
	std::set<GUID> signal_ids;

	std::set<GUID> reference_signal_ids;

	int width = 0;

	int height = 0;
};

// utility namespace - this will be a subject of future restructuring (once there are more utility functions)
namespace utility
{
	// formats double precision number to a given number of decimal places
	inline std::string Format_Decimal(double number, int precision)
	{
		std::stringstream str;
		str << std::fixed << std::setprecision(precision) << number;
		return str.str();
	}
}

/*
 * Data source of segments
 */
class IDrawing_Data_Source
{
	public:
		// retrieves a segment reference for a given ID; may throw an exception if no such segment is found
		virtual const TPlot_Segment& Get_Segment(const size_t id) const = 0;
};

/*
 * Base for all drawing view handlers (generators)
 */
class CDrawing_View_Base
{
	public:
		// draws signals and segments from given data source to target string (SVG); uses the local opts structure to parametrize the outputs
		virtual NDrawing_Error Draw(std::string& target, const TDraw_Options_Local& opts, const IDrawing_Data_Source& source) = 0;
		virtual ~CDrawing_View_Base() = default;
};
