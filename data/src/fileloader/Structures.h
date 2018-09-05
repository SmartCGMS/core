/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
 */

#pragma once

#include <cstdint>

/*
 * Substitutional tiny implementation for std::optional / boost::optional, since this project
 * could not be compiled with C++17 standard due to SimpleINI and xlnt incompatibility
 * NOTE: this is definitelly not complete implementation, it just implements what we truly need
 *       within extraction
 */
template<typename T>
class COptional
{
	private:
		T mVal;
		bool mHasValue = false;

	public:
		T & operator=(T const& val) { mVal = val; mHasValue = true; return mVal; }
		auto operator<(T const& val) const { return mVal < val; }
		auto operator>(T const& val) const { return mVal > val; }
		auto operator<=(T const& val) const { return mVal <= val; }
		auto operator>=(T const& val) const { return mVal >= val; }
		auto operator==(T const& val) const { return mVal == val; }

		T value() const { return mVal; }

		void reset() { mHasValue = false; }
		bool has_value() const { return mHasValue; }
};

/*
 * Structure for holding one record from source file
 * This does not contain any more columns since we don't need to hold i.e. segment ID because
 * the extraction is just temporary
 */
struct CMeasured_Value
{
	// julian date of measured date/time
	double mMeasuredAt;
	// BG value
	COptional<double> mBlood;
	// IG value
	COptional<double> mIst;
	// ISIG value
	COptional<double> mIsig;
	// insulin bolus amount
	COptional<double> mInsulin;
	// carbohydrates amount
	COptional<double> mCarbohydrates;
	// BG sensor calibration value
	COptional<double> mCalibration;

	// reset optional fields to allow reuse
	void reuse()
	{
		mBlood.reset();
		mIst.reset();
		mIsig.reset();
		mInsulin.reset();
		mCarbohydrates.reset();
		mCalibration.reset();
	}
};
