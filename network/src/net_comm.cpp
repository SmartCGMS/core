
#ifdef _WIN32
#include <WS2tcpip.h>
#include <Windows.h>

#pragma comment(lib, "Ws2_32.lib")

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#endif

#include "../../../common/rtl/winapi_mapping.h"

#include "net_comm.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/iface/DeviceIface.h"

#include <cstdint>
#include <limits>

// note to my future self: on Windows, define preprocessor macro NOMINMAX to avoid max() and min() definitions by Windows.h

#pragma pack(push, 1)

// fixed point format for representing double; this is important due to possible incompatibility between float models
struct TNet_Double
{
	uint32_t integer_part;
	uint32_t fraction_part;
};

// fill network double representation from local double
void Fill_Net_Double(double d, TNet_Double& target)
{
	target.integer_part = htonl(static_cast<uint32_t>(d));
	target.fraction_part = htonl(static_cast<uint32_t>((std::numeric_limits<uint32_t>::max() + 1.0)*d));
}

// fill local double representation from given network one
void Fill_From_Net_Double(TNet_Double& src, double& d)
{
	d = static_cast<double>(ntohl(src.integer_part)) + (static_cast<double>(ntohl(src.fraction_part)) / static_cast<double>(std::numeric_limits<uint32_t>::max() + 1.0));
}

// network representation of sensor event; for documentation, see glucose::TSensor_Event
struct TDevice_Event_Network
{
	glucose::NDevice_Event_Code event_code;

	GUID device_id;
	GUID signal_id;

	TNet_Double device_time;
	int64_t logical_time;

	TNet_Double level;
};

// converts network sensor event to local one
void Convert_Net_To_Local(TDevice_Event_Network& src, glucose::TDevice_Event& dst)
{
	dst.event_code = src.event_code;
	dst.device_id = src.device_id;
	dst.signal_id = src.signal_id;
	Fill_From_Net_Double(src.device_time, dst.device_time);
	dst.logical_time = static_cast<int64_t>(ntohl(static_cast<u_long>(src.logical_time)));
	Fill_From_Net_Double(src.level, dst.level);
}

// converts local sensor event to network one
void Convert_Local_To_Net(glucose::TDevice_Event& src, TDevice_Event_Network& dst)
{
	dst.event_code = src.event_code;
	dst.device_id = src.device_id;
	dst.signal_id = src.signal_id;
	Fill_Net_Double(src.device_time, dst.device_time);
	dst.logical_time = static_cast<int64_t>(htonl(static_cast<u_long>(src.logical_time)));
	Fill_Net_Double(src.level, dst.level);
}

#pragma pack(pop)


CNet_Comm::CNet_Comm(glucose::IFilter_Pipe* inpipe, glucose::IFilter_Pipe* outpipe)
	: mInput(inpipe), mOutput(outpipe), mHost(""), mPort(0)
{
	//
}

void CNet_Comm::Run_Recv()
{
	int res;
	SOCKET sock;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0)
	{
		// TODO: error
		return;
	}

	sockaddr_in locAddr;
	locAddr.sin_family = AF_INET;
	locAddr.sin_port = htons(mPort);
	res = InetPtonA(AF_INET, mHost.c_str(), &locAddr.sin_addr.s_addr);
	if (res != 1)
	{
		// TODO: error
		return;
	}

	res = bind(sock, (sockaddr*)&locAddr, sizeof(sockaddr_in));
	if (res < 0)
	{
		// TODO: error
		return;
	}

	res = listen(sock, 1); // allow just one client
	if (res < 0)
	{
		// TODO: error
		return;
	}

	SOCKET remoteSock;
	sockaddr_in remoteAddr;
	socklen_t addrLen = sizeof(sockaddr_in);

	TDevice_Event_Network netEvent;
	glucose::TDevice_Event locEvent;

	while (true)
	{
		// TODO: proper error handling

		// since we support only one client, there's no need to use select() - we have blocking accept and recv call

		remoteSock = accept(sock, (sockaddr*)&remoteAddr, &addrLen);
		if (remoteSock <= 0)
			continue;

		while (true)
		{
			res = recv(remoteSock, (char*)&netEvent, sizeof(TDevice_Event_Network), 0);
			if (res < sizeof(TDevice_Event_Network))
			{
				closesocket(remoteSock);
				break;
			}

			Convert_Net_To_Local(netEvent, locEvent);

			if (mOutput->send(&locEvent) != S_OK)
				break;
		}
	}
}

void CNet_Comm::Run_Send()
{
	int res;
	glucose::TDevice_Event locEvent;
	TDevice_Event_Network netEvent;

	SOCKET sock = (SOCKET)0;
	sockaddr_in targetAddr;
	targetAddr.sin_family = AF_INET;
	targetAddr.sin_port = htons(mPort);
	res = InetPtonA(AF_INET, mHost.c_str(), &targetAddr.sin_addr.s_addr);
	if (res != 1)
	{
		// TODO: error
		return;
	}

	while (mInput->receive(&locEvent) == S_OK)
	{
		Convert_Local_To_Net(locEvent, netEvent);

		while (true)
		{
			if (sock == (SOCKET)0)
			{
				sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if (sock < 0)
				{
					// TODO: error
					return;
				}

				res = connect(sock, (sockaddr*)&targetAddr, sizeof(sockaddr_in));
				if (res < 0)
				{
					// TODO: error, wait, retry
					return;
				}
			}

			res = send(sock, (const char*)&netEvent, sizeof(TDevice_Event_Network), 0);
			if (res < sizeof(TDevice_Event_Network))
			{
				// TODO: error, wait, retry
				return;
			}

			break;
		}
	}
}

HRESULT CNet_Comm::Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration)
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

		if (confname == rsNet_Host)
		{
			if (cur->wstr->get(&begin, &end) != S_OK)
				continue;

			mHost = std::string(begin, end);
		}
		else if (confname == rsNet_Port)
			mPort = static_cast<uint16_t>(cur->int64);
		else if (confname == rsNet_RecvSide)
			mRecvSide = cur->boolean;
	}

	if (mHost.empty() || mPort == 0)
		return EINVAL;

#ifdef _WIN32
	WORD version = MAKEWORD(1, 1);
	WSADATA data;
	if (WSAStartup(version, &data) != 0)
		return EFAULT;
#endif

	if (mRecvSide)
		Run_Recv();
	else
		Run_Send();

	return S_OK;
};
