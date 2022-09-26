#include "Client.h"

#include <iostream>

Client::Client()
	: mSocket(INVALID_SOCKET)
	, mNetworkID(0)
	, mRecvOver()
	, mPrevSize(0)
	, mPacketBuf{}
	, mIsAlive(false)
	, mStatus(ESocketStatus::FREE)
	, mPosition()
{
	mName[0] = '\0';
}

void Client::Init()
{
	mSocket = INVALID_SOCKET;
	mPrevSize = 0;
	memset(mPacketBuf, 0, MAX_PACKET_SIZE);
	mIsAlive = false;
	mName[0] = '\0';
	mStatus = ESocketStatus::FREE;
	mPosition = { 0,0,0 };
}
