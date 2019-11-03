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

// GQCS에서 활용할 클라이언트 정보(1클라이언트당 1할당)
typedef struct
{
	SOCKET			hClientSocket;		// 클라이언트 소켓
	SOCKADDR_IN		clientAddress;		// 클라이언트 네트워크상 주소
	CPacketQueue*	packetQueue;		// 단편화된 패킷의 임시 저장을 위한 큐
	// 클라이언트 클래스 삽입해야 함
}PER_HANDLE_DATA, * LPPER_HANDLE_DATA;

// GQCS에서 활용할 메세지 정보(1입출력당 1할당)
typedef struct
{
	OVERLAPPED		overlapped;					// OVERLAPPED IO 이용에 쓰이는 구조체
	WSABUF			wsaBuf;						// OVERLAPPED IO 이용에 쓰이는 버퍼 설정용 구조체
	char			buffer[MAX_BUFFER_SIZE];	// OVERLAPPED IO에 사용되는 다이렉트 버퍼
	int				readWriteFlag;				// 현재 IO 작업이 무엇인지 표현하는 변수
}PER_IO_DATA, * LPPER_IO_DATA;

// CIOCPServer 클래스
// 불특정 다수의 클라이언트와 네트워크 통신 기능을 제공하는 클래스
class CIOCPServer
{
public:
	// Singleton Pattern 구현을 위한 멤버 함수
	static CIOCPServer* GetInstance();

	// 기타 public 멤버 함수
	bool Init(const int PORT);	// 서버 초기화 함수
	
	void RefreshRoomInfo(int num);
	void Release();				// 싱글톤 객체를 메모리에서 해제할 때 호출되는 함수

private:
	// 생성자
	CIOCPServer() {};
	~CIOCPServer() {};

	// Singleton Pattern 구현을 위한 멤버
	static CIOCPServer* m_pInstance;

	// 기타 private 멤버
	WSADATA			m_WsaData;			// Windows Socket API Startup에 쓰이는 구조체
	HANDLE			m_hCP;				// Windows OS에서 제공하는 IO Multiplexer(IO Completion Port)
	SOCKET			m_hServerSocket;	// 클라이언트를 Accept하기 위해 쓰이는 서버 소켓
	SOCKADDR_IN		m_ServerAddress;	// 서버 소켓을 바인딩하기 위해 쓰이는 주소 구조체

	map<SOCKET, CClient*>		m_ClientList;		// 현재 서버에 접속 중인 클라이언트 리스트
	map<int, CGameRoom*>		m_RoomList;			// 현재 서버에 만들어진 방 리스트(map으로 인덱스와 매핑)
	int							m_nLastRoomIndex;	// 방 하나 만들어질 때마다 1씩 계속 증가하는 변수
	CRITICAL_SECTION			m_CS;				// 위에 있는 멤버들 건드릴 때 잠궈주기 위한 CS

	// 기타 private 멤버 함수
	static unsigned int __stdcall acceptProcedure(void * nullpt);			// acceptor
	static unsigned int __stdcall workerProcedure(void * hCompletionPort);	// worker

	// 서버에서 읽기를 마치고 쓰기를 할 때 스스로 실행하는 함수
	void sendRequest(SOCKET socket, char* data, unsigned short type, unsigned short size);

	void initWSARecv(LPPER_IO_DATA * ioInfo);
	void initWSASend(LPPER_IO_DATA * ioInfo, const char * message, int msgLength);
	void closeClient(LPPER_HANDLE_DATA * handleInfo, LPPER_IO_DATA * ioInfo);
};