#include "hold.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/lang/dstrings.h"

#include <iostream>
#include <chrono>

CHold_Filter::CHold_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput(inpipe), mOutput(outpipe), mSimulationOffset(0.0), mNotified(0), mMsWait(0)
{
	//
}

void CHold_Filter::Run_Main()
{
	glucose::UDevice_Event evt;
	bool hold;

	while (mInput.Receive(evt))
	{
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
		}

		if (hold)
		{
			mQueue.push(evt.release());
		}
		else
		{
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
	glucose::UDevice_Event evt;
	time_t t_now;
	double j_now;

	while (mRunning)
	{
		try
		{
			glucose::IDevice_Event *raw_event;
			mQueue.pop(raw_event);
			evt.reset(raw_event);
		}
		catch (tbb::user_abort &)
		{
			break;
		}

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
				time_t tdiff = static_cast<time_t>(round((evt.device_time - j_now) * MSecsPerDay / 1000.0));
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
}

void CHold_Filter::Simulation_Step(size_t stepcount)
{
	std::unique_lock<std::mutex> lck(mHoldMtx);

	mNotified += stepcount;
	mHoldCv.notify_all();
}

HRESULT CHold_Filter::Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration)
{
	glucose::TFilter_Parameter *cbegin, *cend;
	if (configuration->get(&cbegin, &cend) != S_OK)
		return E_FAIL;

	for (glucose::TFilter_Parameter* cur = cbegin; cur < cend; cur += 1)
	{
		wchar_t *begin, *end;
		if (cur->config_name->get(&begin, &end) != S_OK)
			continue;

		std::wstring confname{ begin, end };

		if (confname == rsHold_Values_Delay)
			mMsWait = static_cast<size_t>(cur->int64);
	}

	mRunning = true;

	mHoldThread = std::make_unique<std::thread>(&CHold_Filter::Run_Hold, this);

	Run_Main();

	return S_OK;
};
