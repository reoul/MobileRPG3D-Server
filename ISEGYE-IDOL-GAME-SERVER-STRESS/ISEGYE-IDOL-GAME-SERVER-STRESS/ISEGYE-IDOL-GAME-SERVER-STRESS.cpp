#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <WinSock2.h>
#include <MSWSock.h>
#include <thread>
#include <vector>
#include <Windows.h>
#include "StressClient.h"
#include <Random.h>
#include "PacketStruct.h"

constexpr size_t MAX_TEST_CLIENT = 100;

struct startMatchingPacket
{
	const uint16_t size;
	const EPacketType type;
	const int32_t networkID;
	const wchar_t name[11];
};

void StressThread()
{
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(51341);
	serverAddr.sin_addr.S_un.S_addr = inet_addr("221.151.106.33");

	SOCKET sockets[40];

	for (size_t j = 0; j < 600; ++j)
	{
		size_t mil;

		{
			Random<int> gen(0, 1000);
			mil = gen();
		}
		Sleep(mil);

		std::wcout << j << L" 번째 시작" << std::endl;
		for (SOCKET& s : sockets)
		{
			s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		}
		std::wcout << L"전부 할당 완료" << std::endl;

		for (SOCKET socket : sockets)
		{
			connect(socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
		}
		std::wcout << L"전부 접속 완료" << std::endl;

		Sleep(120000);

		// 매칭
		{
			startMatchingPacket packet{ sizeof(startMatchingPacket), EPacketType::cs_startMatching, 0, L"플레이어" };

			for (SOCKET socket : sockets)
			{
				send(socket, (const char*)&packet, sizeof(startMatchingPacket), 0);
			}
		}

		std::wcout << L"매칭 완료" << std::endl;

		{
			Random<int> gen(200, 400);
			mil = gen();
		}
		Sleep(mil);

		// 캐릭터 선택
		{
			for (size_t k = 0; k < 10; ++k)
			{
				cs_sc_changeCharacterPacket packet{ 100, ECharacterType::Viichan };
				for (SOCKET socket : sockets)
				{
					send(socket, (const char*)&packet, sizeof(cs_sc_changeCharacterPacket), 0);
				}
			}
		}

		std::wcout << L"캐릭터 선택 완료" << std::endl;

		{
			Random<int> gen(200, 400);
			mil = gen();
		}
		Sleep(mil);

		// 전투
		{
			constexpr size_t BATTLE_COUNT = 1000;
			for (size_t u = 0; u < BATTLE_COUNT; ++u)
			{
				// 아이템 추가
				for (size_t k = 0; k < 8; ++k)
				{
					{
						Random<int> gen(50, 100);
						mil = gen();
					}
					cs_sc_AddNewItemPacket packet{ sizeof(cs_sc_AddNewItemPacket), EPacketType::cs_sc_addNewItem, 100, 5 };
					for (SOCKET socket : sockets)
					{
						send(socket, (const char*)&packet, sizeof(cs_sc_AddNewItemPacket), 0);
					}
					Sleep(mil);
				}
				std::wcout << L"아이템 추가 완료" << std::endl;
				{
					Random<int> gen(50, 100);
					mil = gen();
				}

				Sleep(mil);

				// 아이템 이동
				for (size_t k = 0; k < 4; ++k)
				{
					{
						Random<int> gen(50, 100);
						mil = gen();
					}
					cs_sc_changeItemSlotPacket packet{ 100, 5, 9 };
					for (SOCKET socket : sockets)
					{
						send(socket, (const char*)&packet, sizeof(cs_sc_changeItemSlotPacket), 0);
					}
					Sleep(mil);
				}
				std::wcout << L"아이템 이동 완료" << std::endl;

				{
					Random<int> gen(50, 100);
					mil = gen();
				}

				Sleep(mil);

				// 전투 완료 전송
				{
					cs_battleReadyPacket packet{ sizeof(cs_battleReadyPacket), EPacketType::cs_battleReady, 100, 0 };
					for (SOCKET socket : sockets)
					{
						send(socket, (const char*)&packet, sizeof(cs_battleReadyPacket), 0);
					}
				}
				

				std::wcout << L"전투 준비 완료" << std::endl;

				{
					Random<int> gen(1000, 2000);
					mil = gen();
				}

				Sleep(mil);
			}
		}

		for (SOCKET& socket : sockets)
		{
			closesocket(socket);
			socket = INVALID_SOCKET;
		}
		std::wcout << L"전부 해제 완료" << std::endl;
		Sleep(1000);
	}
}

int main()
{
	std::locale::global(std::locale("Korean"));

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	std::vector<std::thread> workerThreads;

	for (size_t i = 0; i < std::thread::hardware_concurrency(); ++i)
	{
		workerThreads.emplace_back(StressThread);
	}

	for (auto& th : workerThreads)
	{
		th.join();
	}

	WSACleanup();
	return 0;
}
