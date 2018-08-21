#pragma once

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"

#include <bitset>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

// minimum count of bits in a bitmask
constexpr size_t BitmaskMinBitCount = 8;
// maximum count of bits in a bitmask (actually template parameter for std::bitset)
constexpr size_t BitmaskMaxBitCount = 64;

/*
 * Filter class for masking input levels using configured bitmask
 */
class CMasking_Filter : public glucose::IFilter, public virtual refcnt::CReferenced
{
	protected:
		// input pipe
		glucose::SFilter_Pipe mInput;
		// output pipe
		glucose::SFilter_Pipe mOutput;

		// masking is performed separatelly for each segment
		std::map<uint64_t, uint8_t> mSegmentMaskState;

		// signal ID to be masked
		GUID mSignal_Id = Invalid_GUID;
		// bitset used for masking
		std::bitset<BitmaskMaxBitCount> mMask;
		// real bit count in a given bitmask
		size_t mBitCount = 0;

	protected:
		bool Parse_Bitmask(std::wstring in);

	public:
		CMasking_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);
		virtual ~CMasking_Filter() {};

		virtual HRESULT Run(glucose::IFilter_Configuration* configuration) override;
};

#pragma warning( pop )
