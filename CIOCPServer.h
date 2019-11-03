#pragma once

#include<iostream>
#include<list>
#include<map>
#include<string>
#include<stdlib.h>
#include<process.h>
#include<WinSock2.h>
#include<Windows.h>

#include"CDataBaseManager.h"
#include"CPacketQueue.h"
#include"CPacket.h"
#include"CClient.h"
#include"CGameRoom.h"
#include"CGameLogics.h"
#include"CUtil.h"

#pragma warning(disable:4996)

using namespace std;

#define READ 0
#define WRITE 1
#define MAX_BUFFER_SIZE 1000

// GQCS���� Ȱ���� Ŭ���̾�Ʈ ����(1Ŭ���̾�Ʈ�� 1�Ҵ�)
typedef struct
{
	SOCKET			hClientSocket;		// Ŭ���̾�Ʈ ����
	SOCKADDR_IN		clientAddress;		// Ŭ���̾�Ʈ ��Ʈ��ũ�� �ּ�
	CPacketQueue*	packetQueue;		// ����ȭ�� ��Ŷ�� �ӽ� ������ ���� ť
	// Ŭ���̾�Ʈ Ŭ���� �����ؾ� ��
}PER_HANDLE_DATA, * LPPER_HANDLE_DATA;

// GQCS���� Ȱ���� �޼��� ����(1����´� 1�Ҵ�)
typedef struct
{
	OVERLAPPED		overlapped;					// OVERLAPPED IO �̿뿡 ���̴� ����ü
	WSABUF			wsaBuf;						// OVERLAPPED IO �̿뿡 ���̴� ���� ������ ����ü
	char			buffer[MAX_BUFFER_SIZE];	// OVERLAPPED IO�� ���Ǵ� ���̷�Ʈ ����
	int				readWriteFlag;				// ���� IO �۾��� �������� ǥ���ϴ� ����
}PER_IO_DATA, * LPPER_IO_DATA;

// CIOCPServer Ŭ����
// ��Ư�� �ټ��� Ŭ���̾�Ʈ�� ��Ʈ��ũ ��� ����� �����ϴ� Ŭ����
class CIOCPServer
{
public:
	// Singleton Pattern ������ ���� ��� �Լ�
	static CIOCPServer* GetInstance();

	// ��Ÿ public ��� �Լ�
	bool Init(const int PORT);	// ���� �ʱ�ȭ �Լ�
	
	void RefreshRoomInfo(int num);
	void Release();				// �̱��� ��ü�� �޸𸮿��� ������ �� ȣ��Ǵ� �Լ�

private:
	// ������
	CIOCPServer() {};
	~CIOCPServer() {};

	// Singleton Pattern ������ ���� ���
	static CIOCPServer* m_pInstance;

	// ��Ÿ private ���
	WSADATA			m_WsaData;			// Windows Socket API Startup�� ���̴� ����ü
	HANDLE			m_hCP;				// Windows OS���� �����ϴ� IO Multiplexer(IO Completion Port)
	SOCKET			m_hServerSocket;	// Ŭ���̾�Ʈ�� Accept�ϱ� ���� ���̴� ���� ����
	SOCKADDR_IN		m_ServerAddress;	// ���� ������ ���ε��ϱ� ���� ���̴� �ּ� ����ü

	map<SOCKET, CClient*>		m_ClientList;		// ���� ������ ���� ���� Ŭ���̾�Ʈ ����Ʈ
	map<int, CGameRoom*>		m_RoomList;			// ���� ������ ������� �� ����Ʈ(map���� �ε����� ����)
	int							m_nLastRoomIndex;	// �� �ϳ� ������� ������ 1�� ��� �����ϴ� ����
	CRITICAL_SECTION			m_CS;				// ���� �ִ� ����� �ǵ帱 �� ����ֱ� ���� CS

	// ��Ÿ private ��� �Լ�
	static unsigned int __stdcall acceptProcedure(void * nullpt);			// acceptor
	static unsigned int __stdcall workerProcedure(void * hCompletionPort);	// worker

	// �������� �б⸦ ��ġ�� ���⸦ �� �� ������ �����ϴ� �Լ�
	void sendRequest(SOCKET socket, char* data, unsigned short type, unsigned short size);

	void initWSARecv(LPPER_IO_DATA * ioInfo);
	void initWSASend(LPPER_IO_DATA * ioInfo, const char * message, int msgLength);
	void closeClient(LPPER_HANDLE_DATA * handleInfo, LPPER_IO_DATA * ioInfo);
};