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
#include "../../../common/rtl/DeviceLib.h"

#include <map>
#include <set>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance



class CTime_Segment : public glucose::ITime_Segment, public virtual refcnt::CNotReferenced {
protected:
	glucose::SFilter_Pipe mOutput;
	const GUID mCalculated_Signal_Id;
	GUID mReference_Signal_Id = Invalid_GUID;
	const int64_t mSegment_id;
protected:
	glucose::SSignal mCalculated_Signal;
	std::map<GUID, glucose::SSignal> mSignals;
	glucose::SSignal Get_Signal_Internal(const GUID &signal_id);
protected:
	double mPrediction_Window;
	double mLast_Pending_time = std::numeric_limits<double>::quiet_NaN();	//it is faster to query it rather than to query the vector correctly
	std::set<double> mPending_Times;
	std::set<double> mEmitted_Times;	//to avoid duplicities in the output
	glucose::SModel_Parameter_Vector mWorking_Parameters;
public:
	CTime_Segment(const int64_t segment_id, const GUID &calculated_signal_id, glucose::SModel_Parameter_Vector &working_parameters, const double prediction_window, glucose::SFilter_Pipe output);
	virtual ~CTime_Segment() {};

	virtual HRESULT IfaceCalling Get_Signal(const GUID *signal_id, glucose::ISignal **signal) override;

	bool Add_Level(const GUID &signal_id, const double level, const double time_stamp);
	bool Set_Parameters(glucose::SModel_Parameter_Vector parameters);
	glucose::SModel_Parameter_Vector Get_Parameters();
	bool Calculate(const std::vector<double> &times, std::vector<double> &levels);		//calculates using the working parameters
	void Emit_Levels_At_Pending_Times();
	void Clear_Data();	
};

#pragma warning( pop )