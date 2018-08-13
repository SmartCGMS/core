#include "hold.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/lang/dstrings.h"

#include <iostream>
#include <chrono>
#include <cmath>

CHold_Filter::CHold_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput(inpipe), mOutput(outpipe), mNotified(0), mSimulationOffset(0.0), mMsWait(0)
{
	//
}

void CHold_Filter::Run_Main() {	
	bool hold;

	for (; glucose::UDevice_Event evt = mInput.Receive(); ) {
		hold = true;
		switch (evt.event_code)
		{
			case glucose::NDevice_Event_Code::Simulation_Step:
				Simulation_Step((size_t)evt.signal_id.Data1);
				hold = false;
				break;
			case glucose::NDevice_Event_Code::Solve_Parameters:
			case glucose::NDevice_Event_Code::Suspend_Parameter_Solving:
			case glucose::NDevice_Event_Code::Resume_Parameter_Solving:
			case glucose::NDevice_Event_Code::Information:
			case glucose::NDevice_Event_Code::Warning:
			case glucose::NDevice_Event_Code::Error:
				hold = false;
				break;
			default:
				break;
		}

		if (hold) {
			mQueue.push(evt.release());
		} else {
			if (!mOutput.Send(evt) )
				break;
		}
	}

	mRunning = false;

	mQueue.abort();
	Simulation_Step(1);

	if (mHoldThread->joinable())
		mHoldThread->join();
}

void CHold_Filter::Run_Hold()
{
	
	time_t t_now;
	double j_now;

	while (mRunning)
	{
		try
		{
			glucose::IDevice_Event *raw_event;
			mQueue.pop(raw_event);
			glucose::UDevice_Event evt{ raw_event };
		

			t_now = static_cast<time_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
			j_now = Unix_Time_To_Rat_Time(t_now) + mSimulationOffset;

			std::unique_lock<std::mutex> lck(mHoldMtx);

			if (mMsWait)
			{
				if (mNotified == 0)
					mHoldCv.wait_for(lck, std::chrono::milliseconds(mMsWait));
			
				// check again - the state may have changed during wait
				if (mNotified != 0)
					mNotified--;
			}
			else
			{
				// if the device time is in future, wait for this amount of time to simulate real-time measurement
				while (mNotified == 0 && evt.device_time > j_now)
				{
					time_t tdiff = static_cast<time_t>(std::round((evt.device_time - j_now) * MSecsPerDay / 1000.0));
					mHoldCv.wait_for(lck, std::chrono::seconds(tdiff));

					t_now = static_cast<time_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
					j_now = Unix_Time_To_Rat_Time(t_now) + mSimulationOffset;
				}

				// accumulate simulation offset, if notified and the time is still lower than desired device time
				if (mNotified != 0 && evt.device_time > j_now)
				{
					// accumulate simulation offset, so the next value will come with spacing relevant to current value,
					// not the actual time (so if the spacing is 5 minutes and somebody notifies 2 minutes before current value
					// time, simulation offset increases by 2 minutes and the next value will fire in 5 minutes instead of 7)
					mSimulationOffset += evt.device_time - j_now;

					mNotified--;
				}
			}

			if (!mOutput.Send(evt) )
				break;

		}
		catch (tbb::user_abort &)
		{
			break;
		}
	}
}

void CHold_Filter::Simulation_Step(size_t stepcount)
{
	std::unique_lock<std::mutex> lck(mHoldMtx);

	mNotified += stepcount;
	mHoldCv.notify_all();
}

HRESULT CHold_Filter::Run(refcnt::IVector_Container<glucose::TFilter_Parameter>* const configuration) {
	glucose::SFilter_Parameters shared_configuration = refcnt::make_shared_reference_ext<glucose::SFilter_Parameters, glucose::IFilter_Configuration>(configuration, true);
	mMsWait = shared_configuration.Read_Int(rsHold_Values_Delay);

	mRunning = true;

	mHoldThread = std::make_unique<std::thread>(&CHold_Filter::Run_Hold, this);

	Run_Main();

	return S_OK;
};
