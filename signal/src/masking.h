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
