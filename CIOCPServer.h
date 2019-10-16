#pragma once

#include<iostream>
#include<stdlib.h>
#include<process.h>
#include<WinSock2.h>
#include<Windows.h>

#pragma warning(disable:4996)

#define READ 0
#define WRITE 1
#define MAX_BUFFER_SIZE 1450

using namespace std;

typedef struct
{
	SOCKET			hClientSocket;
	SOCKADDR_IN		clientAddress;
}PER_HANDLE_DATA, * LPPER_HANDLE_DATA;

typedef struct
{
	OVERLAPPED		overlapped;
	WSABUF			wsaBuf;
	char			buffer[MAX_BUFFER_SIZE];
	int				readWriteFlag;
}PER_IO_DATA, * LPPER_IO_DATA;

class CIOCPServer
{
public:
	// Singleton Pattern 구현을 위한 멤버 함수
	static CIOCPServer* GetInstance();

	// 기타 public 멤버 함수
	bool Init(const int PORT);

	void Release();

private:
	// 생성자
	CIOCPServer();
	~CIOCPServer() {};

	// Singleton Pattern 구현을 위한 멤버
	static CIOCPServer* m_pInstance;

	// 기타 private 멤버
	WSADATA			m_WsaData;
	HANDLE			m_hCP;
	SOCKET			m_hServerSocket;
	SOCKADDR_IN		m_ServerAddress;

	// 기타 private 멤버 함수
	static unsigned int __stdcall acceptProcedure(void * nullpt);
	static unsigned int __stdcall workerProcedure(void * hCompletionPort);

	void initWSARecv(LPPER_IO_DATA * ioInfo);
	void initWSASend(LPPER_IO_DATA * ioInfo, const char * message, int msgLength);
};