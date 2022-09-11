#pragma once
#include <cstdint>

enum class EPacketType : uint8_t
{
	sc_connectServer,
	sc_connectRoom,
	sc_disconnect,
	cs_startMatching,
	cs_sc_addNewItem,
	cs_sc_changeItemSlot,
	cs_sc_upgradeItem,
	cs_sc_changeCharacter,
	sc_battleInfo,
	cs_battleReady,
	Max,
};
