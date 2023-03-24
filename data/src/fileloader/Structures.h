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
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#pragma once

#include "..\..\..\..\common\rtl\guid.h"

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
	std::optional<T> get(const GUID &signal_ID) const {
		T* val = nullptr;

		const auto iter = mValues.find(signal_ID);
		if (iter != mValues.end()) {
			val = const_cast<double*>(std::get_if<T>(&iter->second));
		}

		return val != nullptr ? *val : std::optional<T>{};
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

	void update(CMeasured_Values_At_Single_Time& other) {
		other.enumerate([this](const GUID sig, const TValue& val) {
			this->push(sig, val);
		});
	}
};



auto Measured_Value_Comparator = [](const CMeasured_Values_At_Single_Time& a, const CMeasured_Values_At_Single_Time& b) { return a.measured_at() < b.measured_at(); };
using CMeasured_Segment = std::set<CMeasured_Values_At_Single_Time, decltype(Measured_Value_Comparator)>;

