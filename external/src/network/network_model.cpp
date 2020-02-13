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

#ifdef _WIN32
#include <WS2tcpip.h>
#endif

#include <iostream>
#include <chrono>

#undef min

#include "network_model.h"
#include "../../../../common/utils/string_utils.h"
#include "../../../../common/rtl/rattime.h"
#include "../../../../common/rtl/UILib.h"
#include "../../../../common/lang/dstrings.h"
#include "../../../../common/utils/XMLParser.h"
#include "../../../../common/rtl/FilesystemLib.h"
#include "../../../../common/utils/net_utils.h"

std::array<CNetwork_Discrete_Model::TConnection_Slot, CNetwork_Discrete_Model::Connection_Pool_Size> CNetwork_Discrete_Model::mConnection_Pool;
std::mutex CNetwork_Discrete_Model::mConnection_Pool_Mtx;
std::condition_variable CNetwork_Discrete_Model::mConnection_Pool_Cv;

CNetwork_Discrete_Model::CNetwork_Discrete_Model(scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output)
	: CBase_Filter(output), mSlot(*this), mSession(*this)
{
	mNetwork_Initialized = Init_Network();
}

CNetwork_Discrete_Model::~CNetwork_Discrete_Model()
{
	if (mNetThread && mNetThread->joinable())
		mNetThread->join();

	if (mNetwork_Initialized)
		Deinit_Network();
}

CNetwork_Discrete_Model::TSlot CNetwork_Discrete_Model::Alloc_Pool_Slot()
{
	std::unique_lock<std::mutex> lck(mConnection_Pool_Mtx);

	size_t allocSlot = Invalid_Pool_Slot;
	do
	{
		for (size_t i = 0; i < Connection_Pool_Size; i++)
		{
			if (mConnection_Pool[i].available)
			{
				allocSlot = i;
				mConnection_Pool[i].available = false;
				break;
			}
		}

		if (allocSlot == Invalid_Pool_Slot)
			mConnection_Pool_Cv.wait(lck);

	} while (mRunning && allocSlot == Invalid_Pool_Slot);

	return static_cast<TSlot>(allocSlot);
}

void CNetwork_Discrete_Model::Free_Pool_Slot(TSlot slot)
{
	std::unique_lock<std::mutex> lck(mConnection_Pool_Mtx);

	mConnection_Pool[slot].available = true;
	mConnection_Pool[slot].session_id = scgms::Invalid_Session_Id;
	mConnection_Pool_Cv.notify_one();
}

void CNetwork_Discrete_Model::Network_Thread_Fnc()
{
	int retval;
	fd_set rdset;
	FD_ZERO(&rdset);

	timeval tv;

	mRunning = true;

	size_t pingCounter = 0;

	while (mRunning)
	{
		mSession.Setup();

		FD_ZERO(&rdset);
		FD_SET(mSlot().skt, &rdset);

		tv.tv_usec = 1;
		tv.tv_sec = 1;

		retval = select(static_cast<int>(mSlot().skt) + 1, &rdset, nullptr, nullptr, &tv);

		if (!mRunning)
			break;

		if (retval == 0)
		{
			// send ping once every X ticks
			if (pingCounter++ >= mPing_Interval)
			{
				pingCounter = 0;

				scgms::TNet_Packet<void> keepalive(scgms::NOpcodes::UU_KEEPALIVE_REQUEST);
				mSession.Send_Packet(keepalive);
			}

			if (mPending_Shut_Down)
				mSession.Send_Teardown_Request();
		}
		else if (retval < 0)
		{
			if (Connect(mRemoteAddr, mRemotePort) != S_OK)
			{
				Signal_Tear_Down();
				break;
			}
		}
		else
		{
			if (FD_ISSET(mSlot().skt, &rdset))
			{
				if (!Handle_Incoming_Data())
					break;
			}
		}
	}
}

bool CNetwork_Discrete_Model::Handle_Incoming_Data()
{
	scgms::TNet_Packet_Header *hdr;

	unsigned long byteCnt = 0;
	if (ioctlsocket(mSlot().skt, FIONREAD, &byteCnt) < 0 || byteCnt <= 0)
	{
		if (Connect(mRemoteAddr, mRemotePort) != S_OK)
		{
			Emit_Error(dsExtModel_Could_Not_Connect);
			Signal_Tear_Down();
			return false;
		}

		return true;
	}

	// since the mInBuffer is pre-allocated, we always point to a valid piece of memory
	hdr = reinterpret_cast<scgms::TNet_Packet_Header*>(mInBuffer.data());

	bool expectHdr = (mPendingBytes < sizeof(scgms::TNet_Packet_Header));

	// expect the rest of header, if a header is expected; otherwise expect the rest of packet, if the header is already received
	size_t expectedBytes = expectHdr ? sizeof(scgms::TNet_Packet_Header) - mPendingBytes : hdr->length - mPendingBytes;
	expectedBytes = std::min(expectedBytes, mInBuffer.size());

	int retval = recv(mSlot().skt, mInBuffer.data() + mPendingBytes, static_cast<int>(expectedBytes), 0);
	if (retval <= 0)
	{
		if (Connect(mRemoteAddr, mRemotePort) != S_OK)
		{
			Emit_Error(dsExtModel_Could_Not_Connect);
			Signal_Tear_Down();
			return false;
		}
	}
	else
	{
		mPendingBytes += static_cast<size_t>(retval);
		if (expectHdr)
		{
			if (mPendingBytes >= sizeof(scgms::TNet_Packet_Header))
			{
				// protocol magic must match
				if (hdr->magic != scgms::ProtoMagic)
				{
					Emit_Error(dsExtModel_Invalid_Remote_Protocol_Magic);
					Signal_Tear_Down();
					return false;
				}

				// length must not exceed the maximum length
				if (hdr->length > scgms::Max_Packet_Length)
				{
					Emit_Error(dsExtModel_Invalid_Remote_Protocol_Packet_Length);
					Signal_Tear_Down();
					return false;
				}

				// all OK, expect data
			}
			else
				return true; // not enough bytes received, read again
		}

		// number of received bytes never exceeds the length stated in header
		if (mPendingBytes == hdr->length)
		{
			mPendingBytes = 0;

			// process packet; this returns false when some state constraints are violated
			if (!mSession.Process_Pending_Packet())
			{
				Emit_Error(dsExtModel_Invalid_Packet_State);
				Signal_Tear_Down();
				return false;
			}
		}
	}

	return true;
}

HRESULT CNetwork_Discrete_Model::Connect(const std::wstring& addr, uint16_t port)
{
	int result;
	const std::string saddr = Narrow_WString(addr);

	auto socket_cleanup = [this]() {
		if (mSlot().skt != INVALID_SOCKET)
		{
			shutdown(mSlot().skt, SD_BOTH);
			closesocket(mSlot().skt);
			mSlot().skt = INVALID_SOCKET;
		}
	};

	socket_cleanup();

	sockaddr_in conaddr;

	conaddr.sin_family = AF_INET;
	conaddr.sin_port = htons(port);
	if (InetPtonA(AF_INET, saddr.c_str(), &conaddr.sin_addr.s_addr) != 1)
		return E_INVALIDARG;

	mPendingBytes = 0;

	size_t retry = 0;

	while (retry++ <= mConnection_Retry_Count || mConnection_Retry_Count == 0)
	{
		if (mSlot().skt >= 0)
			socket_cleanup();

		mSlot().skt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (mSlot().skt < 0 || mSlot().skt == INVALID_SOCKET) {
			Emit_Error(dsExtModel_Local_Network_Subsystem_Error);
			return ENODEV;
		}

		if (!Set_Socket_Blocking_State(mSlot().skt, false))
		{
			Emit_Error(dsExtModel_Local_Network_Subsystem_Error);
			socket_cleanup();
			return E_FAIL;
		}

		result = connect(mSlot().skt, reinterpret_cast<sockaddr*>(&conaddr), sizeof(conaddr));
		if (result < 0)
		{
			if (WSAGetLastError() == WSAEINPROGRESS)
			{
				fd_set wrset;
				FD_ZERO(&wrset);
				FD_SET(mSlot().skt, &wrset);
				timeval tv;
				tv.tv_sec = static_cast<decltype(tv.tv_sec)>(mSocket_Timeout);
				tv.tv_usec = 1;

				result = select(static_cast<int>(mSlot().skt) + 1, nullptr, &wrset, nullptr, &tv);
				if (result < 0)
				{
					Emit_Error(dsExtModel_Local_Network_Subsystem_Error);
					socket_cleanup();
					return E_FAIL;
				}
				else if (result == 0)
					continue;
				else
				{
					int lon = sizeof(int);
					int valopt;
#ifdef _WIN32
					if (getsockopt(mSlot().skt, SOL_SOCKET, SO_ERROR, (char*)(&valopt), &lon) < 0)
#else
					socklen_t l_lon = static_cast<socklen_t>(lon);
					if (getsockopt(mSlot().skt, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &l_lon) < 0)
#endif
					{
						Emit_Error(dsExtModel_Local_Network_Subsystem_Error);
						socket_cleanup();
						return E_FAIL;
					}
					if (valopt)
						continue;

					break;
				}
			}
		}
		else
			break;
	}

	mSession.Set_Needs_Reinit();
	Set_Socket_Blocking_State(mSlot().skt, true);

	return S_OK;
}

void CNetwork_Discrete_Model::Set_Requested_Signals(scgms::TNet_Requested_Signal_Item* signal_items, size_t count)
{
	mRequested_Signals.clear();

	mBuffered_Signal_Count = 0;

	for (size_t i = 0; i < count; i++)
	{
		mRequested_Signals[signal_items[i].signal_id] = {
			signal_items[i].signal_id,
			signal_items[i].flags,
			0,
			0,
			false
		};

		if ((signal_items[i].flags & scgms::Netmodel_Signal_Flag_Requires_Fresh_Value) == 0)
			mBuffered_Signal_Count++;
	}
}

void CNetwork_Discrete_Model::Set_Synchronization_Source(scgms::NNet_Sync_Source sync_src)
{
	mSync_Source = sync_src;
}

void CNetwork_Discrete_Model::Signal_Data_Obtained(double current_device_time, scgms::TNet_Data_Item* data_items, size_t count)
{
	std::unique_lock<std::mutex> lck(mExt_Model_Mtx);

	if (!mRunning || mPending_Shut_Down)
		return;

	mCur_Ext_Time = current_device_time;
	mPending_External_Data.assign(data_items, data_items + count);

	mExt_Model_Cv.notify_all();
}

void CNetwork_Discrete_Model::Signal_Tear_Down()
{
	// lock scope
	{
		std::unique_lock<std::mutex> lck(mExt_Model_Mtx);

		mRunning = false;

		mExt_Model_Cv.notify_all();
	}
}

void CNetwork_Discrete_Model::Send_Requested_Data()
{
	if (mPending_Shut_Down)
		return;

	std::vector<scgms::TNet_Data_Item> items;

	for (auto mitr : mRequested_Signals)
	{
		items.push_back({ mitr.second.signalId, mitr.second.lastTime, mitr.second.lastValue });
		mitr.second.hasNewValue = false;
	}

	mSession.Send_Advance_Model(items.data(), items.size());

	mPending_Signal_Count = 0;
}

HRESULT CNetwork_Discrete_Model::Do_Execute(scgms::UDevice_Event event)
{
	if (event.event_code() == scgms::NDevice_Event_Code::Level)
	{
		auto itr = mRequested_Signals.find(event.signal_id());
		if (itr != mRequested_Signals.end())
		{
			auto& rec = itr->second;
			
			rec.lastValue = event.level();
			rec.lastTime = event.device_time();
			rec.hasNewValue = true;

			if ((rec.flags & scgms::Netmodel_Signal_Flag_Requires_Fresh_Value) != 0)
				mPending_Signal_Count++;

			// do not forward feedback-link-received events
			return S_OK;
		}
	}
	else if (event.event_code() == scgms::NDevice_Event_Code::Shut_Down)
	{
		std::unique_lock<std::mutex> lck(mExt_Model_Mtx);

		mPending_Shut_Down = true;

		size_t retries = 0;

		while (++retries < 5 && mRunning)
		{
			mSession.Send_Teardown_Request();
			mExt_Model_Cv.wait_for(lck, std::chrono::milliseconds(200));
		}

		mRunning = false;
		mExt_Model_Cv.notify_all();
	}

	return Send(event);
}

HRESULT CNetwork_Discrete_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list &error_description)
{
	if (!mNetwork_Initialized)
	{
		// TODO: report error
		return E_FAIL;
	}

	std::wstring root_path = Get_Application_Dir();
	std::wstring mainfest_path = Path_Append(root_path, L"netmodel_manifest.xml");

	CXML_Parser<wchar_t> parser(mainfest_path);
	if (!parser.Is_Valid())
	{
		// TODO: report error
		return E_FAIL;
	}

	try
	{
		mRemoteAddr = parser.Get_Parameter(L"manifest.connection:host", L"127.0.0.1");

		int tmpPort = std::stoi(parser.Get_Parameter(L"manifest.connection:port", L"8855"));
		if (tmpPort < 0 || tmpPort > 65535)
			throw std::invalid_argument{ "Invalid port" };

		mRemotePort = static_cast<uint16_t>(tmpPort);

		bool ok;

		mConnection_Retry_Count = std::stol(parser.Get_Parameter(L"manifest.connection:retry-count", L"5"));
		mSocket_Timeout = std::stol(parser.Get_Parameter(L"manifest.connection:timeout", L"5"));
		mPing_Interval = std::stol(parser.Get_Parameter(L"manifest.session:ping-interval", L"5"));
		mRequested_Model_GUID = WString_To_GUID(parser.Get_Parameter(L"manifest.model:id", GUID_To_WString(Invalid_GUID)), ok);

		if (!ok)
			throw std::invalid_argument{ "Invalid model GUID" };
	}
	catch (std::exception&/* ex*/)
	{
		// TODO: report error
		return E_FAIL;
	}

	mInBuffer.resize(scgms::Max_Packet_Length);

	mSlot.Set_Slot(Alloc_Pool_Slot());

	// this happens only if a fatal error has occurred
	if (!mSlot.Has_Slot())
		return E_FAIL;

	mNetThread = std::make_unique<std::thread>(&CNetwork_Discrete_Model::Network_Thread_Fnc, this);

	return S_OK;
}

HRESULT IfaceCalling CNetwork_Discrete_Model::Set_Current_Time(const double new_current_time)
{
	mCur_Time = new_current_time;
	mCur_Ext_Time = mCur_Time;

	return S_OK;
}

HRESULT IfaceCalling CNetwork_Discrete_Model::Step(const double time_advance_delta)
{
	std::unique_lock<std::mutex> lck(mExt_Model_Mtx);

	const double nextFutureTime = mCur_Time + time_advance_delta;

	// communicate with external model for the whole requested period, so we may step the external model mutliple times during one Step method call
	while (mCur_Ext_Time < nextFutureTime && !mPending_Shut_Down)
	{
		while (mPending_External_Data.empty() && mRunning)
			mExt_Model_Cv.wait(lck);

		// we don't need the ext model lock anymore - this is here to avoid deaclock on Send, as another thread may be sending us Shut_Down event
		lck.unlock();

		if (!mRunning)
			return S_FALSE;

		// do not wait for buffered signals; this is here due to debugging reasons
		mPending_Signal_Count = mBuffered_Signal_Count;

		for (size_t i = 0; i < mPending_External_Data.size() && !mPending_Shut_Down; i++)
			Emit_Signal_Level(mPending_External_Data[i].signal_id, mPending_External_Data[i].device_time, mPending_External_Data[i].level);

		mPending_External_Data.clear();

		// acquire the lock again
		lck.lock();

		// Send is synchronous, so by the time the loop is finished, all signals should be available
		Send_Requested_Data();
	}

	mCur_Time = nextFutureTime;
	mPending_Shut_Down = false;

	return S_OK;
}

bool CNetwork_Discrete_Model::Emit_Signal_Level(const GUID& id, double device_time, double level)
{
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
	evt.device_id() = network_model::model_id;
	evt.device_time() = device_time;
	evt.signal_id() = id;
	evt.level() = level;
	evt.segment_id() = reinterpret_cast<uint64_t>(this);

	return SUCCEEDED(Send(evt));
}

bool CNetwork_Discrete_Model::Emit_Error(const std::wstring& error)
{
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Error };
	evt.device_id() = network_model::model_id;
	evt.device_time() = Unix_Time_To_Rat_Time(time(nullptr));
	evt.info.set(error.c_str());

	return SUCCEEDED(Send(evt));
}
