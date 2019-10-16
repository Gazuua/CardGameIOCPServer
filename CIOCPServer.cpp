#include "CIOCPServer.h"

CIOCPServer::CIOCPServer()
{
	printf("������\n");
}

CIOCPServer* CIOCPServer::GetInstance()
{
	// ���� �ν��Ͻ��� �޸� �Ҵ�
	if (!m_pInstance) {
		m_pInstance = new CIOCPServer();
	}
	return m_pInstance;
}

// �޸𸮿��� ����
void CIOCPServer::Release()
{
	WSACleanup();
	delete m_pInstance;
	m_pInstance = nullptr;
	printf("delete�Ϸ�\n");
}

// IOCP ���� �ʱ�ȭ �Լ�
bool CIOCPServer::Init(const int PORT)
{
	SYSTEM_INFO			SystemInfo;		// CPU �ھ� ���� �ľ�
	char				hostname[256];	// �ڽ��� hostname �ľ�
	hostent*			host;			// �ڽ��� ip �ּ� �ľ�
	unsigned char		ipAddr[4];		// �ڽ��� ip �ּ� ����

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

	puts("\n=============���� �ʱ� ���� �Ϸ�!=============\n");
	printf("���� �ּ� -> %d.%d.%d.%d/%d\n", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3], PORT);

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

	printf("acceptThread ����\n");
	
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
	printf("acceptThread ����\n");
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

	// �ذ��ؾ� �� ����
	// 1. �� ��Ŷ�� ���� ���� �߷��� �� ���� �ִ�.
	// 2. ���� ���� ��Ŷ�� ���ļ� �ϳ��� �� ���� �ִ�.

	printf("workerThread ����\n");
	while (1)
	{
		gqcsRet = GetQueuedCompletionStatus(hCP, &transferredBytes, (LPDWORD)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);		
		socket = handleInfo->hClientSocket;

		// ���ϰ��� 1�� ��� (�������� IO ����� �������� ���)
		if (gqcsRet)
		{
			// �б� �Ϸ� ��
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

				printf("���ŵ� �޼��� :: %s\n", ioInfo->buffer);
				GetInstance()->initWSASend(&ioInfo, ioInfo->buffer, transferredBytes);
				WSASend(socket, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);

				GetInstance()->initWSARecv(&ioInfo);
				WSARecv(socket, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);
			}
			// ���� �Ϸ� ��
			else
			{
				// WSASend �Լ��� ���� ����� ȣ��� �� �ݵ�� �ش� ioInfo�� �޸𸮸� ����
				free(ioInfo);
			}
		}
		// ���ϰ��� 0�� ��� (���� ����ó���� �ؾ� �ϴ� ����� �������� ���)
		else
		{
			int err = WSAGetLastError();
			switch (err)
			{
			case 64:
				closesocket(socket);
				puts("Ŭ���̾�Ʈ ���μ����� ���Ḧ �����Ͽ�, �ش� Ŭ���̾�Ʈ���� ������ �����մϴ�.");
				printf("Client Disconnected :: %s/%d\n", inet_ntoa(handleInfo->clientAddress.sin_addr),
					handleInfo->clientAddress.sin_port);
				break;
			default:
				printf("============ ERROR CODE :: %d\n", err);
				break;
			}
		}
	}
	printf("workerThread ����\n");
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
