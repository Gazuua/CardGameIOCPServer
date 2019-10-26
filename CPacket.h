#pragma once
#include<iostream>
#include<string>

#include"CUtil.h"

using namespace std;

#pragma warning(disable:4996)

#define PACKET_TYPE_STANDARD 0		// STANDARD :: ���� �ۼ��ſ� ��Ŷ Ÿ��

// CPacket :: ��Ŷ�� Ŭ����ȭ��Ų Ŭ�����̴�.
class CPacket
{
public:
	// WSARecv ���ڵ� �� ����� �� ���ڿ��� �پ��ִ� data�� ���ڷ� �޴´�.
	CPacket(char* data);
	// WSASend ���ڵ� �� ������ �����Ϳ� �� ũ�� �� ������ �뵵(type)�� ���ڷ� �޴´�.
	CPacket(char* data, unsigned short type, unsigned short size);
	~CPacket();

	// Ŭ����ȭ�� ��Ŷ�� ��� �����ϵ��� char �����ͷ� ���ڵ��ϴ� �Լ�
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
	CStandardPacketContent(char* data, unsigned short size);
	~CStandardPacketContent() {};

	string GetCommand();
	void SetCommand(string command);
private:
	string m_Command;
};