#include "CIOCPServer.h"

CIOCPServer::CIOCPServer()
{
	printf("생성됨\n");
}

CIOCPServer* CIOCPServer::GetInstance()
{
	// 최초 인스턴스에 메모리 할당
	if (!m_pInstance) {
		m_pInstance = new CIOCPServer();
	}
	return m_pInstance;
}

// 메모리에서 해제
void CIOCPServer::Release()
{
	WSACleanup();
	delete m_pInstance;
	m_pInstance = nullptr;
	printf("delete완료\n");
}

// IOCP 서버 초기화 함수
bool CIOCPServer::Init(const int PORT)
{
	SYSTEM_INFO			SystemInfo;		// CPU 코어 개수 파악
	char				hostname[256];	// 자신의 hostname 파악
	hostent*			host;			// 자신의 ip 주소 파악
	unsigned char		ipAddr[4];		// 자신의 ip 주소 저장

	if (WSAStartup(MAKEWORD(2, 2), &m_WsaData) != 0)
		return false;

	m_hCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	GetSystemInfo(&SystemInfo);
	for (unsigned int i = 0; i < SystemInfo.dwNumberOfProcessors; i++) 
		_beginthreadex(NULL, 0, workerProcedure, (LPVOID)m_hCP, 0, NULL);

	m_hServerSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	memset(&m_ServerAddress, 0, sizeof(m_ServerAddress));
	m_ServerAddress.sin_family = AF_INET;
	m_ServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	m_ServerAddress.sin_port = htons(55248);

	bind(m_hServerSocket, (SOCKADDR*)&m_ServerAddress, sizeof(m_ServerAddress));
	listen(m_hServerSocket, SOMAXCONN);

	_beginthreadex(NULL, 0, acceptProcedure, NULL, 0, NULL);

	gethostname(hostname, 256);
	host = gethostbyname(hostname);
	for (int i = 0; i < 4; i++)
		ipAddr[i] = *(host->h_addr_list[0]+i);

	puts("\n=============서버 초기 설정 완료!=============\n");
	printf("서버 주소 -> %d.%d.%d.%d/%d\n", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3], PORT);

	return true;
}

unsigned int __stdcall CIOCPServer::acceptProcedure(void* nullpt)
{
	SOCKET				hClientSocket;
	SOCKADDR_IN			clientAddress;
	LPPER_HANDLE_DATA	handleInfo;
	LPPER_IO_DATA		ioInfo;

	int					addrLength = sizeof(clientAddress);
	DWORD				recvBytes, flags = 0;

	printf("acceptThread 시작\n");
	
	while (1) 
	{
		hClientSocket = accept((SOCKET)GetInstance()->m_hServerSocket, (SOCKADDR*)&clientAddress, &addrLength);
		if (hClientSocket == NULL) 
		{
			puts("ERROR in Accept");
			continue;
		}
		handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
		handleInfo->hClientSocket = hClientSocket;
		memcpy(&(handleInfo->clientAddress), &clientAddress, addrLength);

		printf("Client Accepted :: %s/%d\n", inet_ntoa(clientAddress.sin_addr), clientAddress.sin_port);

		CreateIoCompletionPort((HANDLE)hClientSocket, GetInstance()->m_hCP, (DWORD)handleInfo, 0);

		GetInstance()->initWSARecv(&ioInfo);
		WSARecv(handleInfo->hClientSocket, &(ioInfo->wsaBuf), 1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);
	}
	printf("acceptThread 종료\n");
	return 0;
}

unsigned int __stdcall CIOCPServer::workerProcedure(void* hCompletionPort)
{
	HANDLE				hCP = (HANDLE)hCompletionPort;
	SOCKET				socket;
	DWORD				transferredBytes;
	BOOL				gqcsRet;

	LPPER_HANDLE_DATA	handleInfo;
	LPPER_IO_DATA		ioInfo;
	DWORD				flags = 0;

	// 해결해야 할 사항
	// 1. 한 패킷이 여러 개로 잘려서 올 때도 있다.
	// 2. 여러 개의 패킷이 뭉쳐서 하나로 올 때도 있다.

	printf("workerThread 시작\n");
	while (1)
	{
		gqcsRet = GetQueuedCompletionStatus(hCP, &transferredBytes, (LPDWORD)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);		
		socket = handleInfo->hClientSocket;

		// 리턴값이 1인 경우 (정상적인 IO 결과를 통지받은 경우)
		if (gqcsRet)
		{
			// 읽기 완료 시
			if (ioInfo->readWriteFlag == READ)
			{
				if (transferredBytes == 0)
				{
					closesocket(socket);
					printf("Client Disconnected :: %s/%d\n", inet_ntoa(handleInfo->clientAddress.sin_addr),
						handleInfo->clientAddress.sin_port);
					free(handleInfo);
					free(ioInfo);
					continue;
				}

				printf("수신된 메세지 :: %s\n", ioInfo->buffer);
				GetInstance()->initWSASend(&ioInfo, ioInfo->buffer, transferredBytes);
				WSASend(socket, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);

				GetInstance()->initWSARecv(&ioInfo);
				WSARecv(socket, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);
			}
			// 쓰기 완료 시
			else
			{
				// WSASend 함수에 대한 결과가 호출될 시 반드시 해당 ioInfo의 메모리를 해제
				free(ioInfo);
			}
		}
		// 리턴값이 0인 경우 (뭔가 에러처리를 해야 하는 결과를 통지받은 경우)
		else
		{
			int err = WSAGetLastError();
			switch (err)
			{
			case 64:
				closesocket(socket);
				puts("클라이언트 프로세스의 종료를 감지하여, 해당 클라이언트와의 연결을 종료합니다.");
				printf("Client Disconnected :: %s/%d\n", inet_ntoa(handleInfo->clientAddress.sin_addr),
					handleInfo->clientAddress.sin_port);
				break;
			default:
				printf("============ ERROR CODE :: %d\n", err);
				break;
			}
		}
	}
	printf("workerThread 종료\n");
	return 0;
}

void CIOCPServer::initWSARecv(LPPER_IO_DATA *ioInfo)
{
	*ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
	memset(&((*ioInfo)->overlapped), 0, sizeof(OVERLAPPED));
	(*ioInfo)->wsaBuf.len = MAX_BUFFER_SIZE;
	(*ioInfo)->wsaBuf.buf = (*ioInfo)->buffer;
	(*ioInfo)->readWriteFlag = READ;
}

void CIOCPServer::initWSASend(LPPER_IO_DATA * ioInfo, const char* message, int msgLength)
{
	memset(&((*ioInfo)->overlapped), 0, sizeof(OVERLAPPED));
	memcpy((*ioInfo)->buffer, message, msgLength);
	(*ioInfo)->wsaBuf.len = msgLength;
	(*ioInfo)->wsaBuf.buf = (*ioInfo)->buffer;
	(*ioInfo)->readWriteFlag = 1;
}
