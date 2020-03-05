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

#include "Measured_Signal.h"

#undef min

#include <algorithm>
#include <numeric>
#include <assert.h>

CMeasured_Signal::CMeasured_Signal(): mApprox(nullptr) {
	// TODO: proper approximator configuration
	//		 now we just pick the first one, which is obviously wrong

	const auto approx_descriptors = scgms::get_approx_descriptors();

	// TODO: passing approximation parameters through architecture to Approximate method
	//		 for now, we just send nullptr so the approximation method uses default parameters

	if (!approx_descriptors.empty()) {

		scgms::ISignal* self_signal = static_cast<scgms::ISignal*>(this);
		scgms::SApprox_Parameters_Vector params;

		mApprox = scgms::Create_Approximator(approx_descriptors[0].id, self_signal, params);
	}
}

HRESULT IfaceCalling CMeasured_Signal::Get_Discrete_Levels(double* const times, double* const levels, const size_t count, size_t *filled) const {

	*filled = std::min(count, mTimes.size());

	if (*filled) {
		const size_t bytes_to_copy = (*filled) * sizeof(double);
		memcpy(times, mTimes.data(), bytes_to_copy);
		memcpy(levels, mLevels.data(), bytes_to_copy);
	}

	return S_OK;
}

HRESULT IfaceCalling CMeasured_Signal::Get_Discrete_Bounds(scgms::TBounds* const time_bounds, scgms::TBounds* const level_bounds, size_t *level_count) const {
	if (level_count)
		*level_count = mLevels.size();

	if (mLevels.size() == 0)
		return S_FALSE;

	if (time_bounds) {
		time_bounds->Min = mTimes[0];
		time_bounds->Max = mTimes[mTimes.size() - 1];
	}

	if (level_bounds) {
		auto res = std::minmax_element(mLevels.begin(), mLevels.end());
		level_bounds->Min = *res.first;
		level_bounds->Max = *res.second;
	}

	return S_OK;
}

template <typename T>
class CSort_Iterator_Value {
	friend T;
protected:
	double mTime, mLevel;
public:
	CSort_Iterator_Value(const double time, const double level) : mTime(time), mLevel(level) {};
	CSort_Iterator_Value(const T &rhs) : mTime(rhs.mTimes[rhs.mIndex]), mLevel(rhs.mLevels[rhs.mIndex]) {};

	bool operator<(const T& rhs) const {
		return mTime < rhs.mTimes[rhs.mIndex];
	}
};

class CSort_Iterator_Reference {
	friend CSort_Iterator_Value<CSort_Iterator_Reference>;
protected:
	size_t mIndex = 0;
	std::vector<double>& mTimes;
	std::vector<double>& mLevels;
public:
	CSort_Iterator_Reference(const size_t index, std::vector<double>& times, std::vector<double>& levels) : mIndex(index), mTimes(times), mLevels(levels) {};
	
	CSort_Iterator_Reference& operator=(const CSort_Iterator_Reference& rhs) {				
		assert(mIndex < mTimes.size() && (rhs.mIndex < mTimes.size()));
		mTimes[mIndex] = rhs.mTimes[rhs.mIndex];
		mLevels[mIndex] = rhs.mLevels[rhs.mIndex];
		
		return *this; 
	}
	
	CSort_Iterator_Reference& operator=(const CSort_Iterator_Value<CSort_Iterator_Reference>& rhs) {
		mTimes[mIndex] = rhs.mTime;
		mLevels[mIndex] = rhs.mLevel;
		return *this;
	}

	
	bool operator<(const CSort_Iterator_Reference& rhs) const {
		const size_t cnt = mTimes.size();
		if ((mIndex >= cnt) || (rhs.mIndex >= cnt)) return mIndex < rhs.mIndex;
		return mTimes[mIndex] < rhs.mTimes[rhs.mIndex];
	}
	

	bool operator<(const CSort_Iterator_Value<CSort_Iterator_Reference>& rhs) const {
		assert(mIndex < mTimes.size());
		return mTimes[mIndex] < rhs.mTime;
	}


	void swap(CSort_Iterator_Reference& rhs) {
		assert(mIndex < mTimes.size() && (rhs.mIndex < mTimes.size()));
		std::swap(mTimes[mIndex], mTimes[rhs.mIndex]);
		std::swap(mLevels[mIndex], mLevels[rhs.mIndex]);
		std::swap(mIndex, rhs.mIndex);
		
	}

};

void swap(CSort_Iterator_Reference lhs, CSort_Iterator_Reference rhs) noexcept {
	lhs.swap(rhs);
}


class CSort_Iterator {
public:	
	using value_type = CSort_Iterator_Value<CSort_Iterator_Reference>;
	using difference_type = std::ptrdiff_t;	
	using pointer = CSort_Iterator_Reference*;	
	using reference = CSort_Iterator_Reference;
	using iterator_category = std::random_access_iterator_tag;	
protected:
	size_t mIndex = 0;
	std::vector<double>& mTimes;
	std::vector<double>& mLevels;	
public:
	CSort_Iterator(const size_t index, std::vector<double>& times, std::vector<double>& levels) : mIndex(index), mTimes(times), mLevels(levels) {};

	value_type operator*() const {
		return value_type(mTimes[mIndex], mLevels[mIndex]);
	}

	reference operator*()  {
		return CSort_Iterator_Reference(mIndex, mTimes, mLevels);
	}

	CSort_Iterator& operator++() { ++mIndex; return *this; }
	CSort_Iterator operator++(int ) {return CSort_Iterator{ mIndex++, mTimes, mLevels }; }
	CSort_Iterator& operator--() { --mIndex; return *this; }
	CSort_Iterator operator--(int) { return CSort_Iterator{ mIndex--, mTimes, mLevels }; }

	difference_type operator - (const CSort_Iterator& rhs) const { return mIndex - rhs.mIndex; }
	CSort_Iterator operator - (const size_t diff) { return CSort_Iterator{ mIndex - diff, mTimes, mLevels }; }
	CSort_Iterator operator + (const size_t diff) { return CSort_Iterator{ mIndex + diff, mTimes, mLevels }; }
	
	CSort_Iterator& operator=(const CSort_Iterator& rhs) { mIndex = rhs.mIndex; return *this; }	
	

	bool operator==(const CSort_Iterator& rhs) const {
		const size_t cnt = mTimes.size();
		if (mIndex == rhs.mIndex) return true;
		if ((mIndex >= cnt) || (rhs.mIndex >= cnt)) return mIndex == false;
		return mTimes[mIndex] == rhs.mTimes[rhs.mIndex];
	}

	bool operator!=(const CSort_Iterator& rhs) const { 
		const size_t cnt = mTimes.size();
		if ((mIndex >= cnt) || (rhs.mIndex >= cnt)) return mIndex != rhs.mIndex;		
		return mTimes[mIndex] != rhs.mTimes[rhs.mIndex]; 
	}


	bool operator<(const CSort_Iterator& rhs) const { 
		const size_t cnt = mTimes.size();
		if ((mIndex >= cnt) || (rhs.mIndex >= cnt)) return mIndex < rhs.mIndex;
		return mTimes[mIndex] < rhs.mTimes[rhs.mIndex]; 
	}


};


HRESULT IfaceCalling CMeasured_Signal::Update_Levels(const double *times, const double *levels, const size_t count) {
	const auto times_original_size = mTimes.size();
	double last_time = times_original_size != 0 ? mTimes[times_original_size - 1] : 0.0;
	bool need_to_sort = false;
	for (size_t i = 0; i < count; i++) {
		//check whether the times[i] is already present and we will updated it

		const auto last = mTimes.begin() + times_original_size;			//need to reevaluate as the insertion might change memory addresses
		auto first = std::lower_bound(mTimes.begin(), last, times[i]);
		if (!(first == last) && !(times[i] < *first)) {

			//we have found the time, we just update the value
			const size_t levels_index = std::distance(mTimes.begin(), first);
			mLevels[levels_index] = levels[i];
		} else {
			//the value is not there, we have to append now
			if (times[i] > last_time) {
				//we can append the level without the need to resort the levels
				mTimes.push_back(times[i]);
				mLevels.push_back(levels[i]);
				last_time = times[i];		//updat the correct last time
			} else {
				need_to_sort = true;	//we insert between past values => need to sort 
				mTimes.push_back(times[i]);
				mLevels.push_back(levels[i]);
			}					
		}
	}

	if (need_to_sort) 			//by using CSort_Iterator, we actually sort both vectors at once without no need for a temporal vectors such as indicies, etc.
		std::sort(CSort_Iterator{ 0, mTimes, mLevels }, CSort_Iterator{ mTimes.size(), mTimes, mLevels });

	return S_OK;
}

HRESULT IfaceCalling CMeasured_Signal::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params, const double* times, double* const levels, const size_t count, const size_t derivation_order) const {
	if (count == 0) return S_FALSE;
	if ((times == nullptr) || (levels == nullptr)) return E_INVALIDARG;

	return mApprox ? mApprox->GetLevels(times, levels, count, derivation_order) : E_FAIL;
}

HRESULT IfaceCalling CMeasured_Signal::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	return E_NOTIMPL;
}
