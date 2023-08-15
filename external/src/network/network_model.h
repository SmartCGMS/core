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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#pragma once

#ifdef _WIN32
	#include <Windows.h>
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <string>
	#include <netdb.h>
	#include <fcntl.h>
#endif

#include "../descriptor.h"
#include "../../../../common/rtl/FilterLib.h"
#include "../../../../common/rtl/Dynamic_Library.h"
#include "../../../../common/utils/winapi_mapping.h"
#include "../../../../common/iface/NetIface.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Network generic discrete model
 */
class CNetwork_Discrete_Model : public scgms::CBase_Filter, public scgms::IDiscrete_Model
{
	using TSlot = size_t;
	static_assert(std::is_unsigned_v<TSlot>, "CNetwork_Discrete_Model::TSlot type has to be unsigned");
	static constexpr TSlot Invalid_Pool_Slot = ~(static_cast<TSlot>(0));

	// how many connections to hold during simulation; setting this to 1 will result in serializing all external simulations
	static constexpr size_t Connection_Pool_Size = 16;

	private:
		struct TConnection_Slot
		{
			SOCKET skt = INVALID_SOCKET;
			bool available = true;
			uint64_t session_id = scgms::Invalid_Session_Id;
			GUID session_secret = Invalid_GUID;
		};

		// global connection pool
		static std::array<TConnection_Slot, Connection_Pool_Size> mConnection_Pool;
		// global connection pool mutex used for slot assignment
		static std::mutex mConnection_Pool_Mtx;
		// global condition variable in case something's waiting for a slot to be freed
		static std::condition_variable mConnection_Pool_Cv;

	private:
		bool mRunning = false;
		double mCur_Time = 0;
		double mCur_Ext_Time = 0;
		double mStart_Time = 0;

		std::vector<scgms::TNet_Data_Item> mPending_External_Data;
		std::mutex mExt_Model_Mtx;
		std::condition_variable mExt_Model_Cv;

		size_t mConnection_Retry_Count = 0;
		size_t mSocket_Timeout = 0;
		size_t mPing_Interval = 0;
		GUID mRequested_Model_GUID = Invalid_GUID;
		std::wstring mRequested_Subject_Name;
		double mMax_Time = 0;

		std::wstring mRemoteAddr;
		uint16_t mRemotePort = 0;

		std::vector<char> mInBuffer;
		size_t mPendingBytes = 0;

		bool mNetwork_Initialized = false;

	private:
		/*
		 * RAII structure for holding and deallocating a connection pool slot during a single pass
		 */
		struct TPool_Slot_Guard
		{
			CNetwork_Discrete_Model& parent;
			std::atomic<TSlot> pool_slot = Invalid_Pool_Slot;

			TPool_Slot_Guard(CNetwork_Discrete_Model& _parent) : parent(_parent) {};
			~TPool_Slot_Guard() { Release_Slot(); }

			// precondition: !Has_Slot()
			void Set_Slot(TSlot slot) { pool_slot = slot; }
			bool Has_Slot() const { return (pool_slot != Invalid_Pool_Slot); }
			void Release_Slot() {
				if (Has_Slot()) {
					TSlot slot = pool_slot.exchange(Invalid_Pool_Slot);
					parent.Release_Pool_Slot(slot);
				}
			}

			// precondition: Has_Slot()
			TConnection_Slot& operator()() const { return mConnection_Pool[pool_slot]; }
		};

		TPool_Slot_Guard mSlot;

		// allocates network slot; blocks if no slot is available
		TSlot Acquire_Pool_Slot();
		// frees a network slot, signalizes waiting entities in Alloc_Pool_Slot
		void Release_Pool_Slot(TSlot slot);

		// handles incoming data on the slot socket
		bool Handle_Incoming_Data();

	private:
		/*
		 * Structure maintaining session-related tasks
		 */
		class CSession_Handler
		{
			public:
				// enumeration of possible session states
				enum class NSession_State
				{
					None,					// no messages were exchanged yet
					Initiating,				// handshake request was sent with a request for a new session
					Restoring,				// handshake request was sent after reconnection with a request to restore existing session
					Running,				// session is up and running
					Tearing_Down,			// session is being teared down (teardown request sent)
					Teared_Down,			// session terminated on both sides
				};

			private:
				// parent reference, always valid
				CNetwork_Discrete_Model& mParent;
				// current session state
				NSession_State mState = NSession_State::None;

				// mutex to guard sending method
				std::mutex mSend_Mtx;

				// session state change synchronization elements
				std::mutex mSession_State_Mtx;
				std::condition_variable mSession_State_Changed_Cv;
				bool mInterrupted = false;
				bool mReinitExisting = false;

			private:
				int Send(const void* data, size_t length);

				void Set_State(NSession_State state);

			public:
				CSession_Handler(CNetwork_Discrete_Model& _parent) : mParent(_parent) {};

				void Setup();
				void Set_Needs_Reinit(bool reinit_existing);
				bool Process_Pending_Packet();

				// ensures desired state, waits if not satisfied
				bool Ensure_State(const NSession_State desired_state);
				void Interrupt();

				NSession_State Get_State() const;

				// incoming packet handlers

				bool Process_Handshake_Reply(scgms::TNet_Packet_Header& header, scgms::TPacket_Handshake_Reply_Body& packet, scgms::TNet_Requested_Signal_Item* signal_items);
				bool Process_Advance_Model(scgms::TNet_Packet_Header& header, scgms::TPacket_Advance_Model_Body& packet, scgms::TNet_Data_Item* data_items);
				bool Process_Keepalive_Request(scgms::TNet_Packet_Header& header);
				bool Process_Keepalive_Response(scgms::TNet_Packet_Header& header);
				bool Process_Teardown_Request(scgms::TNet_Packet_Header& header);
				bool Process_Teardown_Reply(scgms::TNet_Packet_Header& header);

				// sending methods

				void Send_Handshake_Request(double tick_interval, uint64_t sessionId = scgms::Invalid_Session_Id, const GUID& session_secret = Invalid_GUID, const GUID& model_id = Invalid_GUID);
				bool Send_Advance_Model(scgms::TNet_Data_Item* data_items, size_t data_items_count);
				bool Send_Keepalive_Request();
				bool Send_Teardown_Request();
				bool Send_Teardown_Reply();

				// generic send method
				template<typename T>
				bool Send_Packet(T& fixed, void* dynamic = nullptr, size_t dynamic_length = 0)
				{
					int res1, res2 = 0;

					std::unique_lock<std::mutex> lck(mSend_Mtx);

					fixed.header.length += static_cast<decltype(fixed.header.length)>(dynamic_length);

					res1 = Send(&fixed, sizeof(T));
					if (res1 < 0)
						return false;

					if (dynamic && dynamic_length > 0)
					{
						res2 = Send(dynamic, dynamic_length);
						if (res2 < 0)
							return false;
					}

					return (static_cast<size_t>(res1) + static_cast<size_t>(res2) == sizeof(T) + dynamic_length);
				}
		};

		CSession_Handler mSession;

	private:
		struct TBuffered_Signal
		{
			GUID signalId;
			uint8_t flags;
			double lastTime;
			double lastValue;
			bool hasNewValue;
		};

		std::map<GUID, TBuffered_Signal> mRequested_Signals;
		size_t mPending_Signal_Count = 0;
		size_t mBuffered_Signal_Count = 0;

		scgms::NNet_Sync_Source mSync_Source = scgms::NNet_Sync_Source::EXTERNAL;

		bool mPending_Shut_Down = false;

		std::unique_ptr<std::thread> mNetThread;

		std::mutex mEmit_Mtx;

		std::vector<scgms::UDevice_Event> mDeferred_Events_To_Send;

	protected:
		uint64_t mSegment_Id = scgms::Invalid_Segment_Id;
		bool Emit_Signal_Level(const GUID& id, double device_time, double level);
		bool Emit_Error(const std::wstring& error, bool deferred = true);

		HRESULT Connect(const std::wstring& addr, uint16_t port);
		void Network_Thread_Fnc();

		void Set_Requested_Signals(scgms::TNet_Requested_Signal_Item* signal_items, size_t count);
		void Set_Synchronization_Source(scgms::NNet_Sync_Source sync_src);
		void Signal_Data_Obtained(double current_device_time, scgms::TNet_Data_Item* data_items, size_t count);
		void Signal_Tear_Down();

		void Send_Requested_Data();
		void Send_Deferred_Events();

	protected:
		// scgms::CBase_Filter iface implementation
		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list &error_description) override final;

	public:
		CNetwork_Discrete_Model(scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output);
		virtual ~CNetwork_Discrete_Model();

		// scgms::IDiscrete_Model iface
		virtual HRESULT IfaceCalling Initialize(const double current_time, const uint64_t segment_id) override final;
		virtual HRESULT IfaceCalling Step(const double time_advance_delta) override final;
};

#pragma warning( pop )
