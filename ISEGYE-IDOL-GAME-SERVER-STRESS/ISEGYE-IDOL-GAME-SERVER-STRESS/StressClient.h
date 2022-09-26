#pragma once
#include <mutex>
#include <WinSock2.h>
#include <vector>

class Room;

using namespace std;

enum class EClientState
{
	NotServerConnect,			// 서버에 접속 안되어있는 상태
	NotMatching,				// 매칭 안한 경우
	ChoiceCharacter,			// room에 접속해서 캐릭터 변경해야하는 상태
	InGame,						// 게임중
};

class StressClient
{
public:
	StressClient();

	void StressUpdate();
private:
	mutex					mLock;
	SOCKET					mSocket;
	int32_t					mNetworkID;							// 클라이언트 아이디
	//Exover					mRecvOver;							// 확장 overlapped 구조체
	//int32_t					mPrevSize;							// 이전에 받아놓은 양
	//char					mPacketBuf[MAX_PACKET_SIZE];		// 조각난 거 받아두기 위한 버퍼
	//bool					mIsAlive;							// 플레이 도중에 살아있는지(HP가 0이 아닌경우)
	//ESocketStatus			mStatus;							// 접속했나 안했나
	//ECharacterType			mCharacterType;						// 플레이어 캐릭터
	//Room*					mRoomPtr;							// 클라이언트가 속한 룸
	//wchar_t					mName[MAX_USER_NAME_LENGTH];		// 플레이어 이름
	//Item					mUsingItems[MAX_USING_ITEM];		// 전투시에 사용되는 인벤토리
	//Item					mUnUsingItems[MAX_UN_USING_ITEM];	// 보유 유물 보관 인벤토리
	//int16_t					mFirstAttackState;					// 선공 스텟
	//bool					mBattleReady;						// 전투 준비 되었는지
};
