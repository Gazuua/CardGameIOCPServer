#pragma once
#include<iostream>
#include<string>

#include"CUtil.h"

using namespace std;

#pragma warning(disable:4996)

#define PACKET_TYPE_STANDARD 0		// STANDARD :: 에코 송수신용 패킷 타입
#define PACKET_TYPE_LOGIN_REQ 1		// LOGIN_REQ :: 클라이언트에서 로그인 요청
#define PACKET_TYPE_LOGIN_RES 2		// LOGIN_RES :: 클라이언트의 로그인 요청 응답
#define PACKET_TYPE_REGISTER_REQ 3	// REGISTER_REQ :: 클라이언트에서 회원가입 요청
#define PACKET_TYPE_REGISTER_RES 4	// REGISTER_REQ :: 클라이언트의 회원가입 요청 응답

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