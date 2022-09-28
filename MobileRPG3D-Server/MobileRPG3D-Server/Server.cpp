#include "Server.h"

#include <MSWSock.h>

#include "Exover.h"
#include "Global.h"
#include "SettingData.h"
#include "PacketStruct.h"
#include <reoul/MemoryStream.h>

using namespace std;

HANDLE Server::sIocp;
SOCKET Server::sListenSocket;

void Server::Start()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		LogWarning("윈속 초기화 실패");
		exit(-1);
	}

	sIocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	sListenSocket = ::WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (sListenSocket == INVALID_SOCKET)
	{
		Log("Listen 소켓 생성 실패");
	}

	sockaddr_in serverAddr = {};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	CreateIoCompletionPort(reinterpret_cast<HANDLE>(sListenSocket), sIocp, 10345, 0);

	if (::bind(sListenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
	{
		Log("바인드 에러");
		::closesocket(sListenSocket);
		assert(false);
	}

	if (::listen(sListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		Log("Listen 에러");
		::closesocket(sListenSocket);
		assert(false);
	}

	SOCKET clientSocket = ::WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	Exover accept_over;
	accept_over.c_socket = clientSocket;
	ZeroMemory(&accept_over, sizeof(accept_over.over));
	accept_over.type = EOperationType::Accept;
	::AcceptEx(sListenSocket, clientSocket, accept_over.io_buf, NULL, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		NULL, &accept_over.over);

	for (int i = 0; i < MAX_USER; ++i)
	{
		g_clients[i].SetNetworkID(i);
	}

	Log("서버 시작");

	vector<thread> workerThreads;
	for (size_t i = 0; i < thread::hardware_concurrency(); ++i)
	{
		workerThreads.emplace_back(WorkerThread);
	}
	Log("{0}개의 쓰레드 작동", thread::hardware_concurrency());

	while (true)
	{
		if (!g_bIsRunningServer)
		{
			break;
		}
	}

	for (auto& th : workerThreads)
	{
		th.join();
	}

	// 윈속 종료 
	WSACleanup();
}

void Server::Disconnect(int userID)
{
	if (g_clients[userID].GetStatus() == ESocketStatus::FREE)
	{
		return;
	}

	{
		lock_guard<mutex> lg(g_clients[userID].GetMutex());
		g_clients[userID].SetStatus(ESocketStatus::ALLOCATED);	//처리 되기 전에 FREE하면 아직 떠나는 뒷처리가 안됐는데 새 접속을 받을 수 있음

		::closesocket(g_clients[userID].GetSocket());

		cs_sc_disconnectServerPacket packet(userID);
		SendPacketToConnectClients(&packet);

		wchar_t name[MAX_USER_NAME_LENGTH];
		wcscpy(name, g_clients[userID].GetName());
		g_clients[userID].Init();
		g_clients[userID].SetStatus(ESocketStatus::FREE);
	}

	Log("네트워크 {0}번 클라이언트 서버 접속 해제", userID);
}

void Server::WorkerThread()
{
	while (true)
	{
		DWORD io_byte;
		ULONG_PTR key;
		WSAOVERLAPPED* over;
		bool sucess = ::GetQueuedCompletionStatus(sIocp, &io_byte, &key, &over, 1000);
		DWORD errorCode = ::GetLastError();

		// 서버가 끝낸 상태
		if (!g_bIsRunningServer)
		{
			break;
		}
		// 타임아웃인 경우
		if (sucess == FALSE && over == nullptr)
		{
			continue;
		}

		Exover* exover = reinterpret_cast<Exover*>(over);
		int userID = static_cast<int>(key);

		switch (exover->type)
		{
		case EOperationType::Recv:			//받은 패킷 처리 -> overlapped구조체 초기화 -> recv
		{
			if (0 == io_byte)
			{
				Disconnect(userID);

				if (EOperationType::Send == exover->type)
					delete exover;
			}
			else
			{
				Client& client = g_clients[userID];
				PacketConstruct(userID, io_byte);
				ZeroMemory(&client.GetRecvOver().over, sizeof(client.GetRecvOver().over));
				DWORD flags = 0;
				::WSARecv(client.GetSocket(), &client.GetRecvOver().wsabuf, 1, NULL, &flags, &client.GetRecvOver().over, NULL);
			}
			break;
		}
		case EOperationType::Send:			//구조체 delete
			if (0 == io_byte)
			{
				Disconnect(userID);
			}

			LogWrite("네트워크 {0}번 클라이언트 {1}Byte 패킷 전송", userID, io_byte);
			delete exover;
			break;
		case EOperationType::Accept:			//CreateIoCompletionPort으로 클라소켓 iocp에 등록 -> 초기화 -> recv -> accept 다시(다중접속)
		{
			int networkID = -1;
			for (int i = 0; i < MAX_USER; ++i)
			{
				lock_guard<mutex> gl{ g_clients[i].GetMutex() }; //이렇게 하면 unlock이 필요 없다. 이 블록에서 빠져나갈때 unlock을 자동으로 해준다.
				if (ESocketStatus::FREE == g_clients[i].GetStatus())
				{
					g_clients[i].SetStatus(ESocketStatus::ALLOCATED);
					networkID = i;
					break;
				}
			}

			//main에서 소켓을 worker스레드로 옮겨오기 위해 listen소켓은 전역변수로, client소켓은 멤버로 가져왔다.
			SOCKET clientSocket = exover->c_socket;

			if (networkID == -1)
				::closesocket(clientSocket); // send_login_fail_packet();
			else
			{
				const HANDLE result = ::CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSocket), sIocp, networkID, 0);
				if (result == NULL)
				{
					LogWarning("{0}번 소켓 IOCP 등록 실패", networkID);
					::closesocket(clientSocket);
				}
				else
				{
					g_clients[networkID].SetPrevSize(0); //이전에 받아둔 조각이 없으니 0
					g_clients[networkID].SetSocket(clientSocket);

					ZeroMemory(&g_clients[networkID].GetRecvOver().over, sizeof(g_clients[networkID].GetRecvOver().over));
					g_clients[networkID].GetRecvOver().type = EOperationType::Recv;
					g_clients[networkID].GetRecvOver().wsabuf.buf = g_clients[networkID].GetRecvOver().io_buf;
					g_clients[networkID].GetRecvOver().wsabuf.len = MAX_BUF_SIZE;

					// 새 클라이언트에게 서버에 연결되었다고 알림
					Log("네트워크 {0}번 클라이언트 서버 접속", networkID);

					g_clients[networkID].SetStatus(ESocketStatus::ACTIVE);

					sc_connectServerPacket packet(networkID);
					SendPacket(networkID, &packet);

					DWORD flags = 0;
					::WSARecv(clientSocket, &g_clients[networkID].GetRecvOver().wsabuf, 1, NULL, &flags, &g_clients[networkID].GetRecvOver().over, NULL);
				}
			}
			//소켓 초기화 후 다시 accept
			clientSocket = ::WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			exover->c_socket = clientSocket; //새로 받는 소켓을 넣어준다. 안그러면 클라들이 같은 소켓을 공유한다.
			ZeroMemory(&exover->over, sizeof(exover->over)); //accept_over를 exover라는 이름으로 받았으니 exover를 재사용
			::AcceptEx(sListenSocket, clientSocket, exover->io_buf, NULL,
				sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, NULL, &exover->over);
			break;
		}
		}
	}
}

/**
 * \brief 들어온 패킷을 가공
 * \param userID 유저 ID
 * \param ioByteLength 패킷 데이터 길이
 */
void Server::PacketConstruct(int userID, int ioByteLength)
{
	Client& curUser = g_clients[userID];
	Exover& recvOver = curUser.GetRecvOver();

	int restByte = ioByteLength;		//이만큼 남은걸 처리해줘야 한다
	char* p = recvOver.io_buf;			//처리해야할 데이터의 포인터가 필요하다
	int packetSize = 0;				//이게 0이라는 것은 이전에 처리하던 패킷이 없다는 것 

	LogWrite("네트워크 {0}번 클라이언트 {1}Byte 패킷 받음", userID, ioByteLength);

	// 이전에 받아둔 패킷이 있다면
	if (curUser.GetPrevSize() != 0)
	{
		packetSize = reinterpret_cast<uint16_t*>(curUser.GetPacketBuf())[0]; //재조립을 기다기는 패킷 사이즈
	}

	while (restByte > 0)	//처리해야할 데이터가 남아있으면 처리해야한다.
	{
		// 이전에 처리해야할 패킷이 없다면
		if (0 == packetSize)
		{
			// 지금 들어온 패킷의 사이즈를 넣는다
			packetSize = reinterpret_cast<uint16_t*>(p)[0];
		}

		// 나머지 데이터로 패킷을 만들 수 있나 없나 확인
		// 지금 처리해야 하는 패킷의 사이즈가 남은 데이터랑 기존에 더한것보다 작다면(즉 패킷 사이즈 만큼 채울수 있는 데이터가 있다면)
		if (packetSize <= restByte + curUser.GetPrevSize())
		{
			//만들어서 처리한 데이터 크기만큼 패킷 사이즈에서 빼주기
			memcpy(curUser.GetPacketBuf() + curUser.GetPrevSize(), p, packetSize - curUser.GetPrevSize());

			p += packetSize - curUser.GetPrevSize();
			restByte -= packetSize - curUser.GetPrevSize();
			packetSize = 0;														//이 패킷은 이미 처리를 했고 다음 패킷 사이즈는 모름.

			ProcessPacket(userID, curUser.GetPacketBuf());

			curUser.SetPrevSize(0);
		}
		else	//패킷 하나를 만들 수 없다면 버퍼에 복사해두고 포인터와 사이즈 증가
		{
			memcpy(curUser.GetPacketBuf() + curUser.GetPrevSize(), p, restByte); //남은 데이터 몽땅 받는데, 지난번에 받은 데이터가 남아있을 경우가 있으니, 그 뒤에 받아야한다.
			curUser.SetPrevSize(curUser.GetPrevSize() + restByte);
			restByte = 0;
			p += restByte;
		}
	}
}

void Server::ProcessPacket(int userID, char* buf)
{
	switch (static_cast<EPacketType>(buf[2])) //[0,1]은 size
	{
		//case EPacketType::cs_startMatching:
		//{
		//	const cs_startMatchingPacket* pPacket = reinterpret_cast<cs_startMatchingPacket*>(buf);
		//	wcscpy(g_clients[pPacket->networkID].GetName(), pPacket->name);
		//	g_clients[pPacket->networkID].GetName()[MAX_USER_NAME_LENGTH - 1] = '\0';
	case EPacketType::cs_requestConnClientsInfo:
	{
		cs_requestConnClientsInfoPacket* pPacket = reinterpret_cast<cs_requestConnClientsInfoPacket*>(buf);
		Log("현재 접속 클라이언트 목록 요청");
		g_clients[pPacket->networkID].SetPosition(pPacket->position);
		sc_connClientsInfo::SendClientsList(pPacket->networkID);
	}
	break;
	case EPacketType::cs_sc_characterMove:
	{
		cs_sc_characterMovePacket* pPacket = reinterpret_cast<cs_sc_characterMovePacket*>(buf);
		g_clients[pPacket->networkID].SetPosition(pPacket->curPosition);

		for (int i = 0; i < pPacket->networkID; ++i)
		{
			if (g_clients[i].GetStatus() == ESocketStatus::ACTIVE)
				SendPacket(i, pPacket);
		}
		for (int i = pPacket->networkID + 1; i < MAX_USER; ++i)
		{
			if (g_clients[i].GetStatus() == ESocketStatus::ACTIVE)
				SendPacket(i, pPacket);
		}
		Vector3 pos = pPacket->curPosition;
		Log("이동 패킷 받음 {0} 번 플레이어 {1}", pPacket->networkID, pPacket->joystickDistance);
		Log("{0} {1} {2}", pos.x, pos.y, pos.z);
		break;
	}
	default:
		LogWarning("미정의 패킷 받음");
		DebugBreak();
		//exit(-1);
		break;
	}
}

void Server::SendPacket(size_t networkID, void* pPacket)
{
	char* buf = reinterpret_cast<char*>(pPacket);

	const Client& client = g_clients[networkID];

	//WSASend의 두번째 인자의 over는 recv용이라 쓰면 안된다. 새로 만들어야 한다.
	Exover* exover = new Exover;
	exover->type = EOperationType::Send;
	ZeroMemory(&exover->over, sizeof(exover->over));
	exover->wsabuf.buf = exover->io_buf;
	const ULONG length = reinterpret_cast<uint16_t*>(buf)[0];
	exover->wsabuf.len = length;
	memcpy(exover->io_buf, buf, length);

	::WSASend(client.GetSocket(), &exover->wsabuf, 1, NULL, 0, &exover->over, NULL);
}

void Server::SendPacket(size_t networkID, void* pPacket, size_t length)
{
	char* buf = reinterpret_cast<char*>(pPacket);

	const Client& client = g_clients[networkID];

	//WSASend의 두번째 인자의 over는 recv용이라 쓰면 안된다. 새로 만들어야 한다.
	Exover* exover = new Exover;
	exover->type = EOperationType::Send;
	ZeroMemory(&exover->over, sizeof(exover->over));
	exover->wsabuf.buf = exover->io_buf;
	exover->wsabuf.len = length;
	memcpy(exover->io_buf, buf, length);

	::WSASend(client.GetSocket(), &exover->wsabuf, 1, NULL, 0, &exover->over, NULL);
}

void Server::SendPacketToConnectClients(void* pPacket)
{
	for (Client& c : g_clients)
	{
		if (c.GetStatus() == ESocketStatus::ACTIVE)
			SendPacket(c.GetNetworkID(), pPacket);
	}
}

