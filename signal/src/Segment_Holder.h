#pragma once

#include "../../../common/rtl/SolverLib.h"
#include "../../../common/rtl/DeviceLib.h"

class CSegment_Holder
{
	protected:
		struct TSegment_State
		{
			glucose::STime_Segment segment;
			glucose::IModel_Parameter_Vector* parameters;
			bool opened;
		};

		// calculated signal ID
		const GUID mSignalId;

		// segments vector
		std::map<uint64_t, TSegment_State> mSegments;

	public:
		CSegment_Holder(const GUID& signalId);

		void Start_Segment(uint64_t segmentId);
		void Stop_Segment(uint64_t segmentId);

		// adds level to segment
		bool Add_Level(uint64_t segmentId, const GUID& signal_id, double time, double level);
		// does the segment have parameters set?
		bool Has_Parameters(uint64_t segmentId) const;
		// sets model parameters to segment
		void Set_Parameters(uint64_t segmentId, glucose::IModel_Parameter_Vector* params);
		// retrieves calculated value at specified time
		bool Get_Calculated_At_Time(uint64_t segmentId, double time, double& target);
		// retrieves calculated values of given segment (all of them)
		bool Calculate_All_Values(uint64_t segmentId, std::vector<double>& times, std::vector<double>& levels);
};
