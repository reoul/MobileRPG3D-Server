#pragma once
#include <cstdint>
#include "Vector3.h"
#include "reoul/functional.h"

enum class EPacketType : uint8_t
{
	sc_connectServer,
	cs_sc_characterMove,
	sc_connClientsInfo,
	Max,
};

#pragma pack(push, 1)

struct Packet
{
	const_wrapper<int16_t> size;
	const_wrapper<EPacketType> type;

	Packet(int16_t size_, EPacketType type_)
		: size(size_)
		, type(type_)
	{
	}
};

struct sc_connectServerPacket : private Packet
{
	const_wrapper<int> networkID;
	const_wrapper<Vector3> position;

	sc_connectServerPacket(int networkID, Vector3 position)
		: Packet(sizeof(sc_connectServerPacket), EPacketType::sc_connectServer)
		, networkID(networkID)
		, position(position)
	{
	}
};

struct cs_sc_characterMovePacket : private Packet
{
	const_wrapper<int> networkID;
	const_wrapper<Vector3> curPosition;

	cs_sc_characterMovePacket(int networkID, Vector3 position)
		: Packet(sizeof(cs_sc_characterMovePacket), EPacketType::cs_sc_characterMove)
		, networkID(networkID)
		, curPosition(position)
	{
	}
};

#pragma pack(pop)
