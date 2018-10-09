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
#include "../../../common/rtl/SolverLib.h"
#include "../../../common/rtl/referencedImpl.h"

#include "time_segment.h"

#include <memory>
#include <set>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Filter class for calculating signals from incoming parameters
 */
class CCalculate_Filter : public glucose::IFilter, public glucose::ICalculate_Filter_Inspection, public virtual refcnt::CReferenced {
protected:
	glucose::SFilter_Pipe mInput;
	glucose::SFilter_Pipe mOutput;
protected:
	// calculated signal ID
	GUID mCalculated_Signal_Id = Invalid_GUID;
	GUID mReference_Signal_Id = Invalid_GUID;
	double mPrediction_Window = 0.0;
	glucose::SModel_Parameter_Vector mDefault_Parameters, mLower_Bound, mUpper_Bound;
	GUID mSolver_Id = Invalid_GUID;
	GUID mMetric_Id = Invalid_GUID;
	bool mUse_Relative_Error = true;
	bool mUse_Squared_Differences = false;
	bool mPrefer_More_Levels = false;		//i.e. result is once more divided by the number of levels evaluated
	double mMetric_Threshold = 0.0;
	bool mUse_Measured_Levels = false;
	int64_t mLevels_Required = 0;
protected:
	bool mSolver_Enabled = false;
	bool mSolving_Scheduled = false;
	bool mSolve_On_Calibration = true;
	bool mSolve_All_Segments = false;
	int64_t mReference_Level_Threshold_Count = 0;
	int64_t mReference_Level_Counter = 0;
	void Schedule_Solving(const GUID &level_signal_id);
	void Run_Solver(const uint64_t segment_id);
	double Calculate_Fitness(glucose::ITime_Segment **segments, const size_t segment_count, glucose::SMetric metric, glucose::IModel_Parameter_Vector *parameters);
	void Configure(glucose::SFilter_Parameters shared_configuration);
protected:
	bool mWarm_Reset_Done = false;
	glucose::TSolver_Progress mSolver_Progress;
protected:
	std::map<int64_t, std::unique_ptr<CTime_Segment>> mSegments;
	std::vector<glucose::SModel_Parameter_Vector> mParameter_Hints;
	std::unique_ptr<CTime_Segment>& Get_Segment(const uint64_t segment_id);
	void Add_Level(const uint64_t segment_id, const GUID &signal_id, const double level, const double time_stamp);	
	void Add_Parameters_Hint(glucose::SModel_Parameter_Vector parameters);
public:
	CCalculate_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);
	virtual ~CCalculate_Filter() {};

	virtual HRESULT Run(glucose::IFilter_Configuration* configuration) override;
	virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override final;
	virtual HRESULT IfaceCalling Get_Solver_Progress(glucose::TSolver_Progress* const progress) override;
	virtual HRESULT IfaceCalling Cancel_Solver() override;
};

#pragma warning( pop )
