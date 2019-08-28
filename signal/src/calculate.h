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
class CCalculate_Filter : public glucose::ISynchronous_Filter, public glucose::ICalculate_Filter_Inspection, public virtual refcnt::CReferenced {
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
	bool mSolve_On_Time_Segment_End = false;
	bool mSolve_All_Segments = false;
	int64_t mReference_Level_Threshold_Count = 0;
	int64_t mReference_Level_Counter = 0;
	void Schedule_Solving(const GUID &level_signal_id);
	void Run_Solver(const uint64_t segment_id, glucose::SDevice_Event_Vector& events);
	double Calculate_Fitness(glucose::ITime_Segment **segments, const size_t segment_count, glucose::SMetric metric, glucose::IModel_Parameter_Vector *parameters);
protected:
	bool mWarm_Reset_Done = false;
	solver::TSolver_Progress mSolver_Progress{ 0, 0, 0, FALSE };
	glucose::TSolver_Status mSolver_Status;
protected:
	std::map<int64_t, std::unique_ptr<CTime_Segment>> mSegments;
	std::vector<glucose::SModel_Parameter_Vector> mParameter_Hints;
	std::unique_ptr<CTime_Segment>& Get_Segment(const uint64_t segment_id);
	void Add_Level(const uint64_t segment_id, const GUID &signal_id, const double level, const double time_stamp, glucose::SDevice_Event_Vector& events);
	void Add_Parameters_Hint(glucose::SModel_Parameter_Vector parameters);
public:
	CCalculate_Filter();
	virtual ~CCalculate_Filter() {};

	virtual HRESULT Configure(glucose::IFilter_Configuration* configuration) override;
	virtual HRESULT Execute(glucose::IDevice_Event_Vector* events) override;

	virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override final;
	virtual HRESULT IfaceCalling Get_Solver_Progress(solver::TSolver_Progress* const progress) override;
	virtual HRESULT IfaceCalling Get_Solver_Information(GUID* const calculated_signal_id, glucose::TSolver_Status* const status) const override;
	virtual HRESULT IfaceCalling Cancel_Solver() override;
};

#pragma warning( pop )
