#pragma once
#include <mutex>
#include <WinSock2.h>

#include "Exover.h"

using namespace std;

class Client
{
public:
	Client();

	void					Init();
	const SOCKET&			GetSocket() const;
	void					SetSocket(SOCKET socket);
	int32_t					GetNetworkID() const;
	void					SetNetworkID(int32_t networkID);
	Exover&					GetRecvOver();
	int32_t					GetPrevSize() const;
	void					SetPrevSize(int32_t size);
	char*					GetPacketBuf();
	bool					IsAlive() const;
	ESocketStatus			GetStatus() const;
	void					SetStatus(ESocketStatus status);
	wchar_t*				GetName();
	const wchar_t*			GetName() const;
	std::mutex&				GetMutex();
private:
	std::mutex				mLock;
	SOCKET					mSocket;
	int32_t					mNetworkID;						// 클라이언트 아이디
	Exover					mRecvOver;						// 확장 overlapped 구조체
	int32_t					mPrevSize;						// 이전에 받아놓은 양
	char					mPacketBuf[MAX_PACKET_SIZE];	// 조각난 거 받아두기 위한 버퍼
	bool					mIsAlive;						// 플레이 도중에 살아있는지(HP가 0이 아닌경우)
	ESocketStatus			mStatus;						// 접속했나 안했나
	wchar_t					mName[MAX_USER_NAME_LENGTH];	// 플레이어 이름
};

inline const SOCKET& Client::GetSocket() const
{
	return mSocket;
}

inline void Client::SetSocket(SOCKET socket)
{
	mSocket = socket;
}

inline int32_t Client::GetNetworkID() const
{
	return mNetworkID;
}

inline void Client::SetNetworkID(int32_t networkID)
{
	mNetworkID = networkID;
}

inline Exover& Client::GetRecvOver()
{
	return mRecvOver;
}

inline int32_t Client::GetPrevSize() const
{
	return mPrevSize;
}

inline void Client::SetPrevSize(int32_t size)
{
	mPrevSize = size;
}

inline char* Client::GetPacketBuf()
{
	return mPacketBuf;
}

inline bool Client::IsAlive() const
{
	return mIsAlive;
}

inline ESocketStatus Client::GetStatus() const
{
	return mStatus;
}

inline void Client::SetStatus(ESocketStatus status)
{
	mStatus = status;
}

inline wchar_t* Client::GetName()
{
	return mName;
}

inline const wchar_t* Client::GetName() const
{
	return mName;
}

inline std::mutex& Client::GetMutex()
{
	return mLock;
}
