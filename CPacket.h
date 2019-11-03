#pragma once
#include<iostream>
#include<string>

#include"CUtil.h"

using namespace std;

#pragma warning(disable:4996)

#define PACKET_TYPE_STANDARD 0			// ���� �ۼ��ſ� ��Ŷ Ÿ��
#define PACKET_TYPE_LOGIN_REQ 1			// Ŭ���̾�Ʈ���� �α��� ��û
#define PACKET_TYPE_LOGIN_RES 2			// Ŭ���̾�Ʈ�� �α��� ��û ����
#define PACKET_TYPE_REGISTER_REQ 3		// Ŭ���̾�Ʈ���� ȸ������ ��û
#define PACKET_TYPE_REGISTER_RES 4		// Ŭ���̾�Ʈ�� ȸ������ ��û ����
#define PACKET_TYPE_ENTER_LOBBY_REQ 5	// �κ� ������ Ŭ���̾�Ʈ�� ������ ��û�ϴ� ��Ŷ Ÿ��
#define PACKET_TYPE_USER_INFO_RES 6		// �κ��� ���� ������ �����ϴ� ��Ŷ Ÿ��
#define PACKET_TYPE_ROOM_LIST_RES 7		// �κ��� roomlist ������ �����ϴ� ��Ŷ Ÿ��
#define PACKET_TYPE_ROOM_MAKE_REQ 8		// �κ񿡼� Ŭ���̾�Ʈ�� ���� ���� �� ��û�ϴ� ��Ŷ Ÿ��
#define PACKET_TYPE_ROOM_MAKE_RES 9		// �� ���� ����� ���� ���� ��Ŷ Ÿ��
#define PACKET_TYPE_ENTER_ROOM_REQ 10	// �濡 �����ϰ� ���� Ŭ���̾�Ʈ�� ��û
#define PACKET_TYPE_ENTER_ROOM_RES 11	// �濡 �����ϰ� ���� Ŭ���̾�Ʈ�� ��û�� ���� ����
#define PACKET_TYPE_ROOM_INFO_REQ 12	// �濡 �ִ� Ŭ���̾�Ʈ�� ���� ������ ��û�ϴ� ��Ŷ
#define PACKET_TYPE_ROOM_INFO_RES 13	// �濡 �ִ� Ŭ���̾�Ʈ���� ���� ������ �ִ� ����
#define PACKET_TYPE_ROOM_USER_RES 14	// �濡 �ִ� Ŭ���̾�Ʈ���� �ش� ���� ���� ����� �ִ� ����
#define PACKET_TYPE_EXIT_ROOM_REQ 15	// �濡�� ������ Ŭ���̾�Ʈ�� ������ �˸��� ��Ŷ

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