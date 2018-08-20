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
