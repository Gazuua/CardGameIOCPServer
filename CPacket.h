#pragma once
#include<iostream>
#include<string>

#include"CUtil.h"

using namespace std;

#pragma warning(disable:4996)

#define PACKET_TYPE_STANDARD 0			// 에코 송수신용 패킷 타입
#define PACKET_TYPE_LOGIN_REQ 1			// 클라이언트에서 로그인 요청
#define PACKET_TYPE_LOGIN_RES 2			// 클라이언트의 로그인 요청 응답
#define PACKET_TYPE_REGISTER_REQ 3		// 클라이언트에서 회원가입 요청
#define PACKET_TYPE_REGISTER_RES 4		// 클라이언트의 회원가입 요청 응답
#define PACKET_TYPE_ENTER_LOBBY_REQ 5	// 로비에 입장한 클라이언트가 정보를 요청하는 패킷 타입
#define PACKET_TYPE_USER_INFO_RES 6		// 로비의 유저 정보를 제공하는 패킷 타입
#define PACKET_TYPE_ROOM_LIST_RES 7		// 로비의 roomlist 정보를 제공하는 패킷 타입
#define PACKET_TYPE_ROOM_MAKE_REQ 8		// 로비에서 클라이언트가 방을 만들 때 요청하는 패킷 타입
#define PACKET_TYPE_ROOM_MAKE_RES 9		// 방 만든 사람에 대한 응답 패킷 타입
#define PACKET_TYPE_ENTER_ROOM_REQ 10	// 방에 입장하고 싶은 클라이언트의 요청
#define PACKET_TYPE_ENTER_ROOM_RES 11	// 방에 입장하고 싶은 클라이언트의 요청에 대한 응답
#define PACKET_TYPE_ROOM_INFO_REQ 12	// 방에 있는 클라이언트가 방의 정보를 요청하는 패킷
#define PACKET_TYPE_ROOM_INFO_RES 13	// 방에 있는 클라이언트에게 방의 정보를 주는 응답
#define PACKET_TYPE_ROOM_USER_RES 14	// 방에 있는 클라이언트에게 해당 방의 유저 목록을 주는 응답
#define PACKET_TYPE_EXIT_ROOM_REQ 15	// 방에서 나가는 클라이언트가 서버에 알리는 패킷

// CPacket :: 패킷을 클래스화시킨 클래스이다.
class CPacket
{
public:
	// WSARecv 디코딩 시 헤더와 끝 문자열이 붙어있는 data를 인자로 받는다.
	CPacket(char* data);
	// WSASend 인코딩 시 순수한 데이터와 그 크기 및 보내는 용도(type)를 인자로 받는다.
	CPacket(char* data, unsigned short type, unsigned short size);
	~CPacket();

	// 클래스화된 패킷을 통신 가능하도록 char 포인터로 인코딩하는 함수
	LPPER_IO_PACKET Encode();

	unsigned short	GetPacketType();
	unsigned short	GetDataSize();
	char *			GetContent();

private:
	unsigned short	m_PacketType;
	unsigned short	m_DataSize;
	char *			m_Content;
};