#include "CIOCPServer.h"

// 싱글톤 인스턴스를 프로그램에서 호출하는 함수
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
}

// IOCP 서버 초기화 함수
bool CIOCPServer::Init(const int PORT)
{
	SYSTEM_INFO			SystemInfo;		// CPU 코어 개수 파악
	char				hostname[256];	// 자신의 hostname 파악
	hostent*			host;			// 자신의 ip 주소 파악
	unsigned char		ipAddr[4];		// 자신의 ip 주소 저장

	// Windows Socket API(WSA)를 초기화
	if (WSAStartup(MAKEWORD(2, 2), &m_WsaData) != 0)
		return false;

	// IO Completion Port 할당
	m_hCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// System에서 사용 가능한 CPU 코어의 갯수만큼 worker thread 시작 
	// 해당 thread들은 GQCS에서 Block되므로 CPU 낭비 없음
	GetSystemInfo(&SystemInfo);
	for (unsigned int i = 0; i < SystemInfo.dwNumberOfProcessors; i++) 
		_beginthreadex(NULL, 0, workerProcedure, (LPVOID)m_hCP, 0, NULL);

	// OVERLAPPED IO(중첩된 비동기 입출력)가 가능하도록 WSASocket 함수로 서버 소켓을 할당
	m_hServerSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	memset(&m_ServerAddress, 0, sizeof(m_ServerAddress));
	m_ServerAddress.sin_family = AF_INET;
	m_ServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	m_ServerAddress.sin_port = htons(55248);

	// bind 및 listen
	bind(m_hServerSocket, (SOCKADDR*)&m_ServerAddress, sizeof(m_ServerAddress));
	listen(m_hServerSocket, SOMAXCONN);

	// listen 준비가 끝나고 accept를 위해 accept thread를 하나 시작
	_beginthreadex(NULL, 0, acceptProcedure, NULL, 0, NULL);

	// 모든 서버 동작 준비가 끝나면 서버의 IP주소와 포트번호를 콘솔창에 출력하여 알림
	gethostname(hostname, 256);
	host = gethostbyname(hostname);
	for (int i = 0; i < 4; i++)
		ipAddr[i] = *(host->h_addr_list[0]+i);

	puts("=============서버 초기 설정 완료!=============\n");
	printf("서버 주소 -> %d.%d.%d.%d/%d\n", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3], PORT);

	return true;
}

// 클라이언트의 접속에 따른 동작을 담당하는 accept thread의 proc 함수
unsigned int __stdcall CIOCPServer::acceptProcedure(void* nullpt)
{
	SOCKET				hClientSocket;		// accept된 클라이언트 소켓을 담을 핸들
	SOCKADDR_IN			clientAddress;		// 클라이언트 주소 구조체
	LPPER_HANDLE_DATA	handleInfo;			// 클라이언트가 접속해있는 동안 관리하기 위한 구조체
	LPPER_IO_DATA		ioInfo;				// 클라이언트로부터 입력받기 위해 할당되는 구조체

	int					addrLength = sizeof(clientAddress);
	DWORD				flags = 0;
	
	while (1) 
	{
		hClientSocket = accept((SOCKET)GetInstance()->m_hServerSocket, (SOCKADDR*)&clientAddress, &addrLength);

		handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
		handleInfo->hClientSocket = hClientSocket;
		memcpy(&(handleInfo->clientAddress), &clientAddress, addrLength);
		handleInfo->packetQueue = new CPacketQueue();

		printf("Client Accepted :: %s/%d\n", inet_ntoa(clientAddress.sin_addr), clientAddress.sin_port);

		CreateIoCompletionPort((HANDLE)hClientSocket, GetInstance()->m_hCP, (DWORD)handleInfo, 0);

		GetInstance()->initWSARecv(&ioInfo);
		WSARecv(handleInfo->hClientSocket, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);
	}
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
	// 1. 한 패킷이 여러 개로 잘려서 올 때도 있다. (네트워크 환경 불안정 or MSS를 초과하는 패킷)
	// 2. 여러 개의 패킷이 뭉쳐서 하나로 올 때도 있다. (클라이언트에서 쉼없이 전송하면 가능...)
	// >> handleInfo에 packetQueue를 추가하여, 특정 문자열을 기준으로 패킷을 결합 및 재구성
	// >> 패킷 송수신 구조 및 프로토콜 설계

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
				// EOF(연결 종료 신호) 전달 시
				if (transferredBytes == 0)
				{
					CIOCPServer::GetInstance()->closeClient(&handleInfo, &ioInfo);
					continue;
				}

				// 이 부분은 자체 프로토콜에 기반하여 PacketQueue를 이용하는 통신 방법이다.
				// 패킷이 잘리거나 붙어서 올 때도 에러 없이 패킷을 처리하여 준다.
				// OnRecv()의 반환값은 클래스화되어 바로 사용할 수 있는 패킷의 개수이다.
				int recv = handleInfo->packetQueue->OnRecv(ioInfo->buffer, transferredBytes);

				// OnRecv()에서 -1을 반환하면 뭔가 잘못되었으니 클라이언트의 연결을 해제하면 된다.
				if (recv == -1)	CIOCPServer::GetInstance()->closeClient(&handleInfo, &ioInfo);

				// 이번 WSARecv에 쓰인 ioInfo는 더 이상 필요하지 않으니 해제한다.
				free(ioInfo);

				// 사용 가능한 패킷 개수만큼 분석 및 작업 요청을 즉시 진행한다.
				// 외부 모듈에서 작업시 외부 모듈의 스레드를 이용하며 결과는 나중에 받는다.
				// 즉시 처리 가능한 내부 작업은 곧바로 처리해 준다.
				for (int i = 0; i < recv; i++)
				{
					CPacket* packet = handleInfo->packetQueue->getPacket();

					// printf("수신된 패킷 타입 : %hu\n", packet->GetPacketType());
					// printf("데이터 크기 : %d\n", packet->GetDataSize());
					// printf("수신된 메세지 :: %s\n", packet->GetContent());

					// 보내기용 패킷을 선언해둔다

					switch (packet->GetPacketType())
					{
					case PACKET_TYPE_STANDARD:
					{
						CStandardPacketContent content(packet->GetContent());
						// 뭔가 작업을 직접 하거나 요청하고
						break;
					}
					default:
						break;
					}

					// 보내기용 패킷을 셋팅 및 보낸다.
				}

				// 그리고 다음 패킷 수신을 기다린다.
				LPPER_IO_DATA newIoInfo;
				GetInstance()->initWSARecv(&newIoInfo);
				WSARecv(socket, &(newIoInfo->wsaBuf), 1, NULL, &flags, &(newIoInfo->overlapped), NULL);

				// 아래 코드는 에코 송수신
				/*
				GetInstance()->initWSASend(&ioInfo, ioInfo->buffer, transferredBytes);
				WSASend(socket, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);

				GetInstance()->initWSARecv(&ioInfo);
				WSARecv(socket, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);
				*/
			}
			// 쓰기 완료 시
			else
			{
				// 당장은 사용되지 않는 분기점
			}
		}
		// 리턴값이 0인 경우 (뭔가 에러처리를 해야 하는 결과를 통지받은 경우)
		else
		{
			int err = WSAGetLastError();
			switch (err)
			{
			case 64: // 클라이언트(소켓) 연결이 이미 종료되었음을 의미하는 코드
				puts("ERROR CODE 64 :: 클라이언트 프로세스의 종료를 감지하여, 해당 클라이언트와의 연결을 종료합니다.");
				CIOCPServer::GetInstance()->closeClient(&handleInfo, &ioInfo);
				break;
			default:
				printf("============ ERROR CODE :: %d\n", err);
				break;
			}
		}
	}
	return 0;
}

void CIOCPServer::initWSARecv(LPPER_IO_DATA *ioInfo)
{
	*ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
	memset(&((*ioInfo)->overlapped), 0, sizeof(OVERLAPPED));
	memset(&(*ioInfo)->buffer, 0, sizeof((*ioInfo)->buffer));
	(*ioInfo)->wsaBuf.len = MAX_BUFFER_SIZE;
	(*ioInfo)->wsaBuf.buf = (*ioInfo)->buffer;
	(*ioInfo)->readWriteFlag = READ;
}

void CIOCPServer::initWSASend(LPPER_IO_DATA * ioInfo, const char* message, int msgLength)
{
	memset(&((*ioInfo)->overlapped), 0, sizeof(OVERLAPPED));
	memset(&(*ioInfo)->buffer, 0, sizeof((*ioInfo)->buffer));
	memcpy((*ioInfo)->buffer, message, msgLength);
	(*ioInfo)->wsaBuf.len = msgLength;
	(*ioInfo)->wsaBuf.buf = (*ioInfo)->buffer;
	(*ioInfo)->readWriteFlag = WRITE;
}

void CIOCPServer::closeClient(LPPER_HANDLE_DATA* handleInfo, LPPER_IO_DATA* ioInfo)
{
	closesocket((*handleInfo)->hClientSocket);
	printf("Client Disconnected :: %s/%d\n", inet_ntoa((*handleInfo)->clientAddress.sin_addr),
		(*handleInfo)->clientAddress.sin_port);
	delete (*handleInfo)->packetQueue;
	free(*handleInfo);
	free(*ioInfo);
}