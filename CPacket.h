#pragma once
#include<iostream>
#include<string>

#include"CUtil.h"

using namespace std;

#pragma warning(disable:4996)

#define PACKET_TYPE_STANDARD 0

// CPacket :: ��Ŷ�� Ŭ����ȭ��Ų Ŭ�����̴�.
class CPacket
{
public:
	// ��, Ŭ���̾�Ʈ�κ��� ���� �����͸� �ܼ��� Ŭ����ȭ�ϴ� ��쿡 ���ȴ�.
	CPacket(char* data);
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
	CStandardPacketContent(char* data);
	~CStandardPacketContent() {};

	string GetCommand();
	void SetCommand(string command);
private:
	string m_Command;
};