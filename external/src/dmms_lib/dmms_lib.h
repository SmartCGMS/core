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
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#pragma once

#include "../descriptor.h"
#include "../../../../common/rtl/FilterLib.h"
#include "../../../../common/rtl/Dynamic_Library.h"

#include <thread>
#include <mutex>

#include <Windows.h>

#include "../dmms_sdk/DmmsEngineDef.h"
#include "../dmms_sdk/DmmsEngineLib.h"

#include "../dmms_proc/PacketStructures.h"

struct DMMS_Engine_Deletor
{
	void operator()(dmms::IDmms* eng)
	{
		// eng->close() deletes the eng pointer itself
	}
};

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * DMMS discrete model (pump setting to, and data source from T1DMS simulator)
 */
class CDMMS_Lib_Discrete_Model : public scgms::CBase_Filter, public virtual scgms::IDiscrete_Model, public dmms::ISimCallbacks
{
	private:
		dmms_model::TParameters mParameters{ {} };

		// placeholder for output signal names; unused for now
		static std::array<char*, 0> mDmms_Out_Signal_Names;

		// DMMS dynamic library; keep in mind that this library needs to remain loaded during simulation
		static std::unique_ptr<CDynamic_Library> mDmmsLib;
		// instance of DMMS engine
		std::unique_ptr<dmms::IDmms, DMMS_Engine_Deletor> mDmmsEngine;

		std::wstring mDMMS_Scenario_File;
		std::wstring mDMMS_Out_File;
		std::wstring mDMMS_Results_Dir;

		bool mRunning = false;
		double mLastTime = std::numeric_limits<double>::quiet_NaN();
		double mTimeStart = std::numeric_limits<double>::quiet_NaN();

		std::unique_ptr<std::thread> mDmmsSimThread;
		std::mutex mDmmsSimMtx;
		std::condition_variable mDmmsSimCv;

		TDMMS_To_SmartCGMS mToSmartCGMS{{}};
		TSmartCGMS_To_DMMS mToDMMS;
		bool mToDMMSValuesReady = false;
		bool mToSmartCGMSValuesReady = false;

		struct DMMS_Announced_Events
		{
			struct {
				double time = std::numeric_limits<double>::quiet_NaN();
				double grams = 0;
				double lastTime = 0;
			} announcedMeal;

			struct {
				double time = std::numeric_limits<double>::quiet_NaN();
				double intensity = 0;
				double lastTime = std::numeric_limits<double>::quiet_NaN();
				double duration = 0;
			} announcedExcercise;
		};

		DMMS_Announced_Events mAnnounced;

	protected:
		uint64_t mSegment_Id = scgms::Invalid_Segment_Id;
		void Emit_Signal_Level(const GUID& id, double device_time, double level);
		void Emit_Shut_Down(double device_time);

		void DMMS_Simulation_Thread_Fnc();

	protected:
		// scgms::CBase_Filter iface implementation
		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

	public:
		CDMMS_Lib_Discrete_Model(scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output);
		virtual ~CDMMS_Lib_Discrete_Model();

		// scgms::IDiscrete_Model iface
		virtual HRESULT IfaceCalling Initialize(const double current_time, const uint64_t segment_id) override final;
		virtual HRESULT IfaceCalling Step(const double time_advance_delta) override final;

		// ::ISimCallbacks iface
		virtual HRESULT IfaceCalling iterationCallback(const dmms::SubjectObject *subjObject, const dmms::SensorSigArray *sensorSigArray, const dmms::NextMealObject *nextMealObject, const dmms::NextExerciseObject *nextExerciseObject,
			const dmms::TimeObject *timeObject, dmms::ModelInputObject *modelInputsToModObject, const dmms::InNamedSignals *inNamedSignals, dmms::OutSignalArray *outSignalArray, dmms::RunStopStatus *runStopStatus) override final;
		virtual HRESULT IfaceCalling initializeCallback(const uint8_t useUniversalSeed, const double universalSeed, const dmms::SimInfo *simInfo) override final;
		virtual HRESULT IfaceCalling outSignalName(dmms::OutputSignalNames *names) override final;
		virtual HRESULT IfaceCalling numOutputSignals(size_t *count) override final;
		virtual HRESULT IfaceCalling cleanupCallback() override final;
};

#pragma warning( pop )
