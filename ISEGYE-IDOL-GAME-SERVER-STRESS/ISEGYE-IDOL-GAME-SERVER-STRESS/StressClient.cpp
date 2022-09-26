#include "StressClient.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include "PacketStruct.h"

StressClient::StressClient()
	: mSocket(INVALID_SOCKET)
	, mNetworkID(0)
	/*, mRecvOver()
	, mPrevSize(0)
	, mPacketBuf{}
	, mIsAlive(false)
	, mStatus(ESocketStatus::FREE)
	, mCharacterType(ECharacterType::Woowakgood)
	, mRoomPtr(nullptr)
	, mName{}
	, mUsingItems{}
	, mUnUsingItems{}
	, mFirstAttackState(0)
	, mBattleReady(false)*/
{
}

void StressClient::StressUpdate()
{
}
