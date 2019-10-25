#pragma once
#include<iostream>
#include<string>

#include"CUtil.h"

using namespace std;

#pragma warning(disable:4996)

#define PACKET_TYPE_STANDARD 0

// CPacket :: 패킷을 클래스화시킨 클래스이다.
class CPacket
{
public:
	// 즉, 클라이언트로부터 받은 데이터를 단순히 클래스화하는 경우에 사용된다.
	CPacket(char* data);
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

class CStandardPacketContent
{
public:
	CStandardPacketContent(char* data);
	~CStandardPacketContent() {};

	string GetCommand();
	void SetCommand(string command);
private:
	string m_Command;
};