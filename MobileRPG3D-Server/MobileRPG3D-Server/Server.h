#pragma once

#include <WinSock2.h>
#include <reoul/logger.h>

class Server
{
public:
	Server() = default;
	static HANDLE sIocp;
	static SOCKET sListenSocket;
	void Start();
private:
	static void Disconnect(int userID);
	static void WorkerThread();
	static void PacketConstruct(int userID, int ioByteLength);
	static void ProcessPacket(int userID, char* buf);
	static void SendPacket(size_t networkID, void* pPacket);
	static void SendPacket(size_t networkID, void* pPacket, size_t length);
};
