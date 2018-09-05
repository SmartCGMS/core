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

#include "../../../common/iface/DeviceIface.h"
#include "../../../common/iface/FilterIface.h"
#include "../../../common/iface/SolverIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/DeviceLib.h"
#include "../../../common/rtl/SolverLib.h"

#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <future>

using TSolution_Hint_Vector = std::vector<glucose::SModel_Parameter_Vector>;

/*
 * Class used as container for calculation and solving, holds results and all input parameters
 */
class CCompute_Holder {
	private:
		struct TSegment_State
		{
			glucose::STime_Segment segment;
			glucose::SModel_Parameter_Vector parameters;
			bool opened;
		};

		// stored segments
		std::map<uint64_t, TSegment_State> mSegments;
		// all solution hints stored; vector of parameter vectors (may be more hints for single model)
		TSolution_Hint_Vector mSolutionHints;
		// is there a solver working at this time?
		bool mSolveInProgress;
		// current solver progress
		glucose::TSolver_Progress mSolverProgress;

		// solver parameter - solver GUID
		const GUID mSolverId;
		// solver parameter - signal GUID
		const GUID mSignalId;
		// instantiated metric
		glucose::SMetric mMetric;
		// solver parameter - levels required for metric
		const size_t mMetricLevelsRequired;
		// solver parameter - use measured levels
		const char mUseMeasuredLevels;

		// lower bound of model parameters; may be nullptr, in this case default lower bounds are used
		glucose::SModel_Parameter_Vector mLowBounds;
		// higher bound of model parameters; may be nullptr, in this case default higher bounds are used
		glucose::SModel_Parameter_Vector mHighBounds;
		// default parameters used when no hint available; may be nullptr, in this case, model descriptor defaults are used
		glucose::SModel_Parameter_Vector mDefaultParameters;

		// cloned segments for solver instance
		std::vector<glucose::STime_Segment> mClonedSegments;
		// IDs of cloned segments
		std::vector<uint64_t> mClonedSegmentIds;
		// temporary model parameters, here we store parameters before comparing them with old and setting them as new solution
		glucose::SModel_Parameter_Vector mTempModelParams;

		// IDs of segments, which had their parameters improved in last solve
		std::vector<uint64_t> mImprovedSegmentIds;
		// the segment ID that was recently stopped; 0 for none
		uint64_t mLastStoppedSegment;

		// use just opened segments to solve parameters
		bool mDetermine_Parameters_Using_All_Known_Segments;

		struct
		{
			std::vector<glucose::ITime_Segment*> clonedSegments;
			std::vector<glucose::IModel_Parameter_Vector*> solutionHints;
			glucose::IModel_Parameter_Vector* paramsTarget;
		} mTmpSolverContainer;

		// maximum time acquired through pipe
		double mMaxTime;

	protected:
		// fills supplied pointers with pointer to default model bounds
		bool Fill_Default_Model_Bounds(const GUID &calculated_signal_id, GUID &reference_signal_id, glucose::SModel_Parameter_Vector &low, glucose::SModel_Parameter_Vector &defaults, glucose::SModel_Parameter_Vector &high);

		// fills solver setup structure
		glucose::TSolver_Setup Prepare_Solver_Setup();

		// solves for given setup
		HRESULT Solve(const glucose::TSolver_Setup &solverSetup);

		// compares mTempModelParams (new) and mModelParams (old) on cloned segments and returns, if it's better solution than the old one
		bool Compare_Solutions(glucose::SMetric metric);

	public:
		CCompute_Holder(const GUID& solver_id, const GUID& signal_id, const glucose::TMetric_Parameters &metric_parameters,
			const size_t metric_levels_required, const char use_measured_levels = 1, bool use_just_opened_segments = false);		

		// starts a new segment
		void Start_Segment(uint64_t segment_id);
		// stops given segment
		void Stop_Segment(uint64_t segment_id);
		// adds level to segment
		bool Add_Level(uint64_t segment_id, const GUID& signal_id, double time, double level);
		// adds a solution hint to given signal id (model)
		void Add_Solution_Hint(glucose::SModel_Parameter_Vector parameters);

		// retrieves maximum time acquired
		double Get_Max_Time() const;

		// starts solver assynchronnously (will return immediatelly)
		std::future<HRESULT> Solve_Async();

		// is there a calculation currently in progress?
		bool Is_Solve_In_Progress() const;
		// aborts current calculation (sets flag to solver, returns immediatelly)
		void Abort();
		// retrieves current percentage of solver progress (returned as integer in range 0-100)
		size_t Get_Solve_Percent_Complete() const;
		// retrieves current metric the solver works with as the best one
		double Get_Solve_Solution_Metric_Value() const;

		// sets bounds (low/high); nullptr means "use default"
		void Set_Bounds(glucose::SModel_Parameter_Vector low, glucose::SModel_Parameter_Vector high);
		// sets defaults (when no hint is available)
		void Set_Defaults(glucose::SModel_Parameter_Vector defaults);
		// retrieves signal ID used for calculations
		GUID Get_Signal_Id() const;

		// retrieves model parameter vector; may be nullptr if no calculation issued
		glucose::SModel_Parameter_Vector Get_Model_Parameters(uint64_t segment_id) const;
		// retrieves vector of improved segments (segments with better solution than in last run)
		void Get_Improved_Segments(std::vector<uint64_t>& target) const;
		// retrieves vector of all segments
		void Get_All_Segment_Ids(std::vector<uint64_t>& target) const;

		// resets model parameters of all segments
		void Reset_Model_Parameters();
};
