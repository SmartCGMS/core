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

#include "../../../../common/rtl/guid.h"

#include <cstdint>
#include <climits>
#include <map>
#include <variant>
#include <optional>
#include <functional>
#include <string>
#include <cmath>
#include <set>


//helper signals
constexpr GUID signal_Insulin_Temp_Rate  = { 0x77e9dafe, 0x2fcd, 0x468b, { 0xac, 0xb8, 0xcc, 0x2d, 0x61, 0x6c, 0x25, 0x32 } };// {77E9DAFE-2FCD-468B-ACB8-CC2D616C2532}
constexpr GUID signal_Insulin_Temp_Rate_Endtime = { 0x23661b4b, 0xf087, 0x4bee, { 0xb6, 0xe5, 0x49, 0x8d, 0x31, 0x7b, 0x75, 0x42 } }; // {23661B4B-F087-4BEE-B6E5-498D317B7542}
constexpr GUID signal_Physical_Activity_Duration = { 0xe0661061, 0x8ab8, 0x4950, { 0xad, 0xd6, 0x63, 0x5c, 0x2e, 0x0, 0xd5, 0x13 } }; // {E0661061-8AB8-4950-ADD6-635C2E00D513}
constexpr GUID signal_Sleep_Endtime = { 0xe9f2f52f, 0xc49f, 0x46e7, { 0x80, 0xd5, 0x11, 0x75, 0xa0, 0x74, 0x8b, 0x6d } };  // {E9F2F52F-C49F-46E7-80D5-1175A0748B6D}
constexpr GUID signal_Comment = { 0xe8621762, 0xc149, 0x4023, { 0xb0, 0x73, 0x7, 0xd6, 0xb9, 0x67, 0xcf, 0x1a } };// {E8621762-C149-4023-B073-07D6B967CF1A}


constexpr GUID signal_Date_Only = { 0x1ac13237, 0x7c75, 0x4c23, { 0xa3, 0xcc, 0xbf, 0xbf, 0xeb, 0x3c, 0x2d, 0x2d } }; // {1AC13237-7C75-4C23-A3CC-BFBFEB3C2D2D}
constexpr GUID signal_Time_Only = { 0xc586e321, 0x444b, 0x454a, { 0xa8, 0x60, 0x2d, 0x4c, 0x64, 0x6d, 0xd7, 0xb8 } }; // {C586E321-444B-454A-A860-2D4C646DD7B8}
constexpr GUID signal_Date_Time = { 0x4e66c60b, 0xd8c7, 0x4aec, { 0x80, 0x1d, 0x11, 0x24, 0x9d, 0x74, 0x2e, 0x4a } };  // {4E66C60B-D8C7-4AEC-801D-11249D742E4A}



class CMeasured_Values_At_Single_Time {
public:
	using TValue = std::variant<double, std::string>;
protected:
	double mMeasured_At = std::numeric_limits<double>::quiet_NaN();
	std::map<const GUID, TValue> mValues;
public:
	void set_measured_at(const double measured_at) { mMeasured_At = measured_at; }
	double measured_at() const { return mMeasured_At; }

	template <typename T>
	void push(const GUID &signal_id, T const& val) {
		mValues[signal_id] = val;		
	}	

	template <typename T>
	const std::optional<T> get(const GUID &signal_ID) const {
		T* val = nullptr;

		const auto iter = mValues.find(signal_ID);
		if (iter != mValues.end()) {
			val = const_cast<T*>(std::get_if<T>(&iter->second));
		}

		if (val != nullptr)
			return *val;
		else
			return std::nullopt;

		//return val != nullptr ? (*val) : std::nullopt; // std::optional<T>{};
	}

	bool valid() const {
		return !(std::isnan(mMeasured_At) || mValues.empty());
	}

	bool has_value(const GUID& signal_ID) const {
		return mValues.find(signal_ID) != mValues.end();
	}


	void enumerate(std::function<void(const GUID&, TValue)> func) const {
		for (const auto &val : mValues) {
			func(val.first, TValue{ val.second });
		}
	}

	void update(const CMeasured_Values_At_Single_Time& other) {
		other.enumerate([this](const GUID sig, const TValue& val) {
			this->push(sig, val);
		});
	}
};


bool Measured_Value_Comparator(const CMeasured_Values_At_Single_Time& a, const CMeasured_Values_At_Single_Time& b);

class CMeasured_Levels {
protected:
	using TMeasured_Value_Comparator = std::function<bool(const CMeasured_Values_At_Single_Time&, const CMeasured_Values_At_Single_Time&)>;
	using TSet = std::set<CMeasured_Values_At_Single_Time, TMeasured_Value_Comparator>;
	TSet mLevels{ Measured_Value_Comparator };
public:
	bool update(const CMeasured_Values_At_Single_Time& val);

	TSet::iterator begin();
	TSet::iterator end();
	bool empty() const;
	size_t size() const;
};

/*
* To consider:
* As each log entry is actually a datetime&value pair, there's they can be stored with
* struct TDatetime_Level {double datetime, double level};
*	std::map<GUID, std::vector<TDatetime_Level>> mLevels;
* 
* where extractor would push datetime&level to each vector found by its GUI
* In FileReader, ResolveSegments would use a triplet (a TValueVector replacement)
*	struct {GUID_remap_id, datetime, level} where the GUID_remap_id is an id to a lookup table with GUIDs to conserve memory.
* Then, ResolveSegments would detect ig/bg signals just by checkding the GUID_remap_id.
* 
* However, to properly handle the extractio of paired signals such as basal temp rate and its endtime, we would have to implement
* some scan in the current datetime stamp, which would complicate the code.
* 
* Anyway, we should consider doing some compression like 
* stuct{uint_64t idx, double level}, where two lower bytes of idx would be GUID_remap_id, while the remaining 6 bytes would be a lookup id
* to a table with known timestamps. Then, we could encode the timestamps on 6 bytes instead of 8, but it may actually increase the memory
* unless we would deal with at least 5 levels per single time. And that's not likely going to happen.
* 
* Therefore, the best solution would be to convert the largest data files separately, then concat the logs and use replay logs to sort them.
*/


/*
* When coding, it looks as the best solution to:
* 1. maintain single datetime-level pairs as stuct{uint_64t idx, double level}, where the reamining 6bytes woudl be 8-bytes double with two least bytes
*	 zeroed and encoding delta from the previous time stamp. As it won't encode a day, then the missing two bytes should be just OK.
* 2. encode any other datetime and multiple levels/or single text event would be encoded as the CMeasure Value as single time
*	
* Then, all routines could stand as they are if we provide a nice, CMeasured_Values_At_Single_Time iterators.
*/