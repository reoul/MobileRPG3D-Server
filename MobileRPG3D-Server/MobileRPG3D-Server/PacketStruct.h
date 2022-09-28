#pragma once
#include <cstdint>
#include "Vector3.h"
#include "reoul/functional.h"
#include "Server.h"
#include "reoul/MemoryStream.h"

enum class EPacketType : uint8_t
{
	sc_connectServer,
	sc_connectNewClient,
	cs_requestConnClientsInfo,
	sc_connClientsInfo,
	cs_sc_characterMove,
	cs_sc_disconnectServer,
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
	const_wrapper<int32_t> networkID;

	sc_connectServerPacket(int networkID)
		: Packet(sizeof(sc_connectServerPacket), EPacketType::sc_connectServer)
		, networkID(networkID)
	{
	}
};

struct sc_connectNewClientPacket : private Packet
{
	const_wrapper<int32_t> networkID;
	const_wrapper<Vector3> position;

	sc_connectNewClientPacket(int networkID, Vector3 pos)
		: Packet(sizeof(sc_connectNewClientPacket), EPacketType::sc_connectNewClient)
		, networkID(networkID)
		, position(pos)
	{
	}
};

struct cs_requestConnClientsInfoPacket : private Packet
{
	const_wrapper<int32_t> networkID;
	const_wrapper<Vector3> position;
};

struct sc_connClientsInfo : private Packet
{
	static void SendClientsList(int networkID)
	{
		int16_t curClientCount = 0;
		WriteMemoryStream writeStream;

		for (const Client& c : g_clients)
		{
			if (c.GetStatus() == ESocketStatus::ACTIVE)
				++curClientCount;
		}
		--curClientCount;	// 자신 카운팅 제외

		writeStream.Write(static_cast<int16_t>(sizeof(int16_t) + sizeof(byte) + sizeof(int16_t) +
			sizeof(int32_t) * curClientCount + sizeof(Vector3) * curClientCount));

		writeStream.Write(EPacketType::sc_connClientsInfo);
		writeStream.Write(curClientCount);

		sc_connectNewClientPacket connectNewClientPacket(networkID, g_clients[networkID].GetPosition());

		for (int32_t i = 0; i < networkID; ++i)
		{
			if (g_clients[i].GetStatus() == ESocketStatus::ACTIVE)
			{
				Server::SendPacket(i, &connectNewClientPacket);
				writeStream.Write(i);
				writeStream.Write(g_clients[i].GetPosition().x);
				writeStream.Write(g_clients[i].GetPosition().y);
				writeStream.Write(g_clients[i].GetPosition().z);
			}
		}
		for (int32_t i = networkID + 1; i < MAX_USER; ++i)
		{
			if (g_clients[i].GetStatus() == ESocketStatus::ACTIVE)
			{
				Server::SendPacket(i, &connectNewClientPacket);
				writeStream.Write(i);
				writeStream.Write(g_clients[i].GetPosition().x);
				writeStream.Write(g_clients[i].GetPosition().y);
				writeStream.Write(g_clients[i].GetPosition().z);
			}
		}

		Server::SendPacket(networkID, const_cast<char*>(writeStream.GetBufferPtr()));
	}
};

struct cs_sc_characterMovePacket : private Packet
{
	const_wrapper<int32_t> networkID;
	const_wrapper<float> joystickDistance;
	const_wrapper<Vector3> curPosition;

	cs_sc_characterMovePacket(int networkID, Vector3 position)
		: Packet(sizeof(cs_sc_characterMovePacket), EPacketType::cs_sc_characterMove)
		, networkID(networkID)
		, curPosition(position)
		, joystickDistance(0)
	{
	}
};

struct cs_sc_disconnectServerPacket : private Packet
{
	const_wrapper<int32_t> networkID;

	cs_sc_disconnectServerPacket(int32_t networkID)
		: Packet(sizeof(cs_sc_disconnectServerPacket), EPacketType::cs_sc_disconnectServer)
		, networkID(networkID)
	{
	}
};

#pragma pack(pop)
