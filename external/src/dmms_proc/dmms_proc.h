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

#include "PacketStructures.h"

#include <thread>
#include <mutex>

#include <Windows.h>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * DMMS discrete model (pump setting to, and data source from T1DMS simulator)
 */
class CDMMS_Proc_Discrete_Model : public virtual scgms::CBase_Filter, public virtual scgms::IDiscrete_Model {
	private:
		std::unique_ptr<std::thread> mReceiver_Thread;

		bool mDMMS_Initialized = false;
		bool mRunning = false;

		TSmartCGMS_To_DMMS mToSend;

		std::wstring mRun_Cmd;
		std::wstring mDMMS_Scenario_File;
		std::wstring mDMMS_Out_File;


		TDMMS_IPC mDMMS_ipc;		
		PROCESS_INFORMATION mDMMS_Proc_Info;

		dmms_model::TParameters mParameters;

		// T1DMS time
		double mLastSimTime = 0;
		// SmartCGMS time
		double mLastTime = -1;
		// starting time
		double mTimeStart;


		bool Configure_DMMS();
		
	protected:
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

		bool Initialize_DMMS();
		void Receive_From_DMMS();
		void Deinitialize_DMMS();

	protected:
		// scgms::CBase_Filter iface implementation
		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

	public:
		CDMMS_Proc_Discrete_Model(scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output);
		virtual ~CDMMS_Proc_Discrete_Model();

		// scgms::IDiscrete_Model iface
		virtual HRESULT IfaceCalling Initialize(const double current_time, const uint64_t segment_id) override final;
		virtual HRESULT IfaceCalling Step(const double time_advance_delta) override final;
};

#pragma warning( pop )
