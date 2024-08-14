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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#pragma once

#include <scgms/rtl/FilterLib.h>
#include <scgms/rtl/SolverLib.h>
#include <scgms/rtl/referencedImpl.h>

#include "time_segment.h"

#include <memory>
#include <set>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Filter class for calculating signals from incoming parameters
 */
class CCalculate_Filter : public scgms::CBase_Filter, public scgms::ICalculate_Filter_Inspection {
	protected:
		// calculated signal ID
		GUID mCalculated_Signal_Id = Invalid_GUID;
		GUID mReference_Signal_Id = Invalid_GUID;
		double mPrediction_Window = 0.0;
		scgms::SModel_Parameter_Vector mDefault_Parameters, mLower_Bound, mUpper_Bound;
		GUID mSolver_Id = Invalid_GUID;
		GUID mMetric_Id = Invalid_GUID;
		bool mUse_Relative_Error = true;
		bool mUse_Squared_Differences = false;
		bool mPrefer_More_Levels = false;		//i.e. result is once more divided by the number of levels evaluated
		double mMetric_Threshold = 0.0;
		bool mUse_Measured_Levels = false;
		int64_t mLevels_Required = 0;
		double mTriggered_Solver_Time = 0;

		bool mSolver_Enabled = false;
		bool mSolving_Scheduled = false;
		bool mSolve_On_Calibration = true;
		bool mSolve_On_Time_Segment_End = false;
		bool mSolve_All_Segments = false;
		int64_t mReference_Level_Threshold_Count = 0;
		int64_t mReference_Level_Counter = 0;
		
		bool mWarm_Reset_Done = false;
		solver::TSolver_Progress mSolver_Progress = solver::Null_Solver_Progress;
		scgms::TSolver_Status mSolver_Status = scgms::TSolver_Status::Disabled;
		const bool mAllow_Update = true;			//should make it configurable once needed
		std::map<int64_t, std::unique_ptr<CTime_Segment>> mSegments;
		std::vector<scgms::SModel_Parameter_Vector> mParameter_Hints;

	protected:
		std::unique_ptr<CTime_Segment>& Get_Segment(const uint64_t segment_id);
		void Add_Level(const uint64_t segment_id, const GUID &signal_id, const double level, const double time_stamp);
		void Add_Parameters_Hint(scgms::SModel_Parameter_Vector parameters);
		void Schedule_Solving(const GUID& level_signal_id);
		void Run_Solver(const uint64_t segment_id);
		double Calculate_Fitness(scgms::ITime_Segment** segments, const size_t segment_count, scgms::SMetric metric, scgms::IModel_Parameter_Vector* parameters);

		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

	public:
		CCalculate_Filter(scgms::IFilter *output);
		virtual ~CCalculate_Filter();

		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override final;
		virtual HRESULT IfaceCalling Get_Solver_Progress(solver::TSolver_Progress* const progress) override;
		virtual HRESULT IfaceCalling Get_Solver_Information(GUID* const calculated_signal_id, scgms::TSolver_Status* const status) const override;
		virtual HRESULT IfaceCalling Cancel_Solver() override;
};

#pragma warning( pop )
