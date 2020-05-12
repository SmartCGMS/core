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

#ifdef _WIN32
#include <WS2tcpip.h>
#endif
#include <iostream>
#include <chrono>

#undef min

#include "network_model.h"
#include "../../../../common/utils/string_utils.h"
#include "../../../../common/rtl/rattime.h"
#include "../../../../common/lang/dstrings.h"
#include "../../../../common/utils/net_utils.h"

void CNetwork_Discrete_Model::CSession_Handler::Set_State(NSession_State state)
{
	std::unique_lock<std::mutex> lck(mSession_State_Mtx);

	mState = state;

	mSession_State_Changed_Cv.notify_all();
}

CNetwork_Discrete_Model::CSession_Handler::NSession_State CNetwork_Discrete_Model::CSession_Handler::Get_State() const
{
	return mState;
}

void CNetwork_Discrete_Model::CSession_Handler::Setup()
{
	if (Get_State() != NSession_State::None)
		return;

	auto& slot = mParent.mSlot();
	// if no session has been started, generate client-side secret for session restore
	if (slot.session_id == scgms::Invalid_Session_Id)
		slot.session_secret = Generate_GUIDv4();

	Send_Handshake_Request(scgms::One_Minute, slot.session_id, slot.session_secret, mParent.mRequested_Model_GUID);
}

void CNetwork_Discrete_Model::CSession_Handler::Set_Needs_Reinit()
{
	Set_State(NSession_State::None);
}

bool CNetwork_Discrete_Model::CSession_Handler::Process_Pending_Packet()
{
	scgms::TNet_Packet_Header *hdr;
	hdr = reinterpret_cast<scgms::TNet_Packet_Header*>(mParent.mInBuffer.data());

	switch (hdr->opcode)
	{
		// unknown opcode
		default:
			mParent.Emit_Error(dsExtModel_Unknown_Opcode);
			return false;

		// wrong opcode in this direction (SE_ prefixed opcodes are sent by us)
		case scgms::NOpcodes::SE_ADVANCE_MODEL:
		case scgms::NOpcodes::SE_HANDSHAKE_REQUEST:
			mParent.Emit_Error(dsExtModel_Invalid_Opcode_Direction);
			return false;

		case scgms::NOpcodes::ES_HANDSHAKE_REPLY:
			return Process_Handshake_Reply(*hdr, Get_Fixed_Body<scgms::TPacket_Handshake_Reply_Body>(hdr), Get_Dynamic_Body<scgms::TPacket_Handshake_Reply_Body, scgms::TNet_Requested_Signal_Item>(hdr));
		case scgms::NOpcodes::ES_ADVANCE_MODEL:
			return Process_Advance_Model(*hdr, Get_Fixed_Body<scgms::TPacket_Advance_Model_Body>(hdr), Get_Dynamic_Body<scgms::TPacket_Advance_Model_Body, scgms::TNet_Data_Item>(hdr));
		case scgms::NOpcodes::UU_KEEPALIVE_REQUEST:
			return Process_Keepalive_Request(*hdr);
		case scgms::NOpcodes::UU_KEEPALIVE_REPLY:
			return Process_Keepalive_Response(*hdr);
		case scgms::NOpcodes::UU_TEARDOWN_REQUEST:
			return Process_Teardown_Request(*hdr);
		case scgms::NOpcodes::UU_TEARDOWN_REPLY:
			return Process_Teardown_Reply(*hdr);
	}

	return true;
}

bool CNetwork_Discrete_Model::CSession_Handler::Ensure_State(const NSession_State desired_state)
{
	std::unique_lock<std::mutex> lck(mSession_State_Mtx);

	mInterrupted = false;

	while (!mInterrupted && Get_State() != desired_state)
		mSession_State_Changed_Cv.wait(lck);

	return (Get_State() == desired_state);
}

void CNetwork_Discrete_Model::CSession_Handler::Interrupt()
{
	std::unique_lock<std::mutex> lck(mSession_State_Mtx);

	mInterrupted = true;

	mSession_State_Changed_Cv.notify_all();
}

bool CNetwork_Discrete_Model::CSession_Handler::Process_Handshake_Reply(scgms::TNet_Packet_Header& header, scgms::TPacket_Handshake_Reply_Body& packet, scgms::TNet_Requested_Signal_Item* signal_items)
{
	if (Get_State() != NSession_State::Initiating && Get_State() != NSession_State::Restoring)
	{
		mParent.Emit_Error(dsExtModel_Invalid_State_Handshake_Reply);
		return false;
	}

	if (packet.status != scgms::NNet_Status_Code::OK)
	{
		// attempt to reinitialize session that does not exist
		if (packet.status == scgms::NNet_Status_Code::FAIL_UNK_SESSION && mParent.mSlot().session_id != scgms::Invalid_Session_Id)
		{
			// this is due to the remote endpoint crashed or performed reset without state save
			mParent.Emit_Error(dsExtModel_Remote_Fatal_Error);
			return false;
		}
		else if (packet.status == scgms::NNet_Status_Code::FAIL_VERSION)
			mParent.Emit_Error(dsExtModel_Protocol_Version_Mismatch);
		else if (packet.status == scgms::NNet_Status_Code::FAIL_UNK_MODEL)
			mParent.Emit_Error(dsExtModel_Unknown_Model_Requested);
		else if (packet.status == scgms::NNet_Status_Code::FAIL_NO_SLOT)
			return true;
		else
			mParent.Emit_Error(dsExtModel_Unknown_Handshake_Error);

		return false;
	}

	auto& slot = mParent.mSlot();

	slot.session_id = packet.session_id;
	mParent.Set_Requested_Signals(signal_items, packet.requested_signal_count);
	mParent.Set_Synchronization_Source(packet.sync_source);

	Set_State(NSession_State::Running);

	return true;
}

bool CNetwork_Discrete_Model::CSession_Handler::Process_Advance_Model(scgms::TNet_Packet_Header& header, scgms::TPacket_Advance_Model_Body& packet, scgms::TNet_Data_Item* data_items)
{
	// data may come between tear down requests (e.g.; when we just requested Tear_Down and the remote has just send the data packet) - just ignore them
	if (Get_State() == NSession_State::Tearing_Down || Get_State() == NSession_State::Teared_Down)
		return true;

	if (Get_State() != NSession_State::Running)
	{
		mParent.Emit_Error(dsExtModel_Invalid_State_Data);
		return false;
	}

	mParent.Signal_Data_Obtained(packet.device_time, data_items, packet.signal_count);

	return true;
}

bool CNetwork_Discrete_Model::CSession_Handler::Process_Keepalive_Request(scgms::TNet_Packet_Header& header)
{
	scgms::TNet_Packet<void> resp(scgms::NOpcodes::UU_KEEPALIVE_REPLY);
	return Send_Packet(resp);
}

bool CNetwork_Discrete_Model::CSession_Handler::Process_Keepalive_Response(scgms::TNet_Packet_Header& header)
{
	// do nothing - this just refreshes the session state in a generic manner
	return true;
}

bool CNetwork_Discrete_Model::CSession_Handler::Process_Teardown_Request(scgms::TNet_Packet_Header& header)
{
	// allow Tearing_Down state due to mutual simultaneous session teardown
	if (Get_State() != NSession_State::Running && Get_State() != NSession_State::Tearing_Down)
	{
		mParent.Emit_Error(dsExtModel_Invalid_State_Teardown_Request);
		return false;
	}

	Set_State(NSession_State::Tearing_Down);
	Interrupt();

	mParent.Signal_Tear_Down();

	return Send_Teardown_Reply();
}

bool CNetwork_Discrete_Model::CSession_Handler::Process_Teardown_Reply(scgms::TNet_Packet_Header& header)
{
	if (Get_State() != NSession_State::Tearing_Down)
	{
		mParent.Emit_Error(dsExtModel_Invalid_State_Teardown_Reply);
		return false;
	}

	mParent.Signal_Tear_Down();
	Interrupt();

	return true;
}

void CNetwork_Discrete_Model::CSession_Handler::Send_Handshake_Request(double tick_interval, uint64_t sessionId, const GUID& session_secret, const GUID& model_id)
{
	scgms::TNet_Packet<scgms::TPacket_Handshake_Request_Body> req(scgms::NOpcodes::SE_HANDSHAKE_REQUEST);

	req.body.protocol_version_major = scgms::ProtoVersionMajor;
	req.body.protocol_version_minor = scgms::ProtoVersionMinor;
	req.body.session_id = sessionId;
	req.body.tick_interval = tick_interval;
	memcpy(&req.body.session_secret, &session_secret, sizeof(GUID));
	memcpy(&req.body.requested_model_id, &model_id, sizeof(GUID));
	wcsncpy_s(req.body.subject_name, mParent.mRequested_Subject_Name.c_str(), std::min(scgms::Subject_Name_Length, mParent.mRequested_Subject_Name.length()));

	if (sessionId == scgms::Invalid_Session_Id)
		Set_State(NSession_State::Initiating);
	else
		Set_State(NSession_State::Restoring);

	Send_Packet(req);
}

bool CNetwork_Discrete_Model::CSession_Handler::Send_Advance_Model(scgms::TNet_Data_Item* data_items, size_t data_items_count)
{
	scgms::TNet_Packet<scgms::TPacket_Advance_Model_Body> req(scgms::NOpcodes::SE_ADVANCE_MODEL);

	req.body.device_time = mParent.mCur_Time;
	req.body.signal_count = static_cast<uint8_t>(data_items_count);

	return Send_Packet(req, data_items, data_items_count * sizeof(scgms::TNet_Data_Item));
}

bool CNetwork_Discrete_Model::CSession_Handler::Send_Keepalive_Request()
{
	scgms::TNet_Packet<void> keepalive(scgms::NOpcodes::UU_KEEPALIVE_REQUEST);

	return Send_Packet(keepalive);
}

bool CNetwork_Discrete_Model::CSession_Handler::Send_Teardown_Request()
{
	Set_State(NSession_State::Tearing_Down);
	Interrupt();

	scgms::TNet_Packet<void> req(scgms::NOpcodes::UU_TEARDOWN_REQUEST);
	return Send_Packet(req);
}

bool CNetwork_Discrete_Model::CSession_Handler::Send_Teardown_Reply()
{
	Interrupt();
	scgms::TNet_Packet<void> resp(scgms::NOpcodes::UU_TEARDOWN_REPLY);
	return Send_Packet(resp);
}

int CNetwork_Discrete_Model::CSession_Handler::Send(const void* data, size_t length)
{
	return send(mParent.mSlot().skt, reinterpret_cast<const char*>(data), static_cast<int>(length), 0);
}
