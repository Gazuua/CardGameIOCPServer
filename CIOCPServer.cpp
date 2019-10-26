#include "CIOCPServer.h"

// �̱��� �ν��Ͻ��� ���α׷����� ȣ���ϴ� �Լ�
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
}

// IOCP ���� �ʱ�ȭ �Լ�
bool CIOCPServer::Init(const int PORT)
{
	SYSTEM_INFO			SystemInfo;		// CPU �ھ� ���� �ľ�
	char				hostname[256];	// �ڽ��� hostname �ľ�
	hostent*			host;			// �ڽ��� ip �ּ� �ľ�
	unsigned char		ipAddr[4];		// �ڽ��� ip �ּ� ����

	// Windows Socket API(WSA)�� �ʱ�ȭ
	if (WSAStartup(MAKEWORD(2, 2), &m_WsaData) != 0)
		return false;

	// IO Completion Port �Ҵ�
	m_hCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// System���� ��� ������ CPU �ھ��� ������ŭ worker thread ���� 
	// �ش� thread���� GQCS���� Block�ǹǷ� CPU ���� ����
	GetSystemInfo(&SystemInfo);
	for (unsigned int i = 0; i < SystemInfo.dwNumberOfProcessors; i++) 
		_beginthreadex(NULL, 0, workerProcedure, (LPVOID)m_hCP, 0, NULL);

	// OVERLAPPED IO(��ø�� �񵿱� �����)�� �����ϵ��� WSASocket �Լ��� ���� ������ �Ҵ�
	m_hServerSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	memset(&m_ServerAddress, 0, sizeof(m_ServerAddress));
	m_ServerAddress.sin_family = AF_INET;
	m_ServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	m_ServerAddress.sin_port = htons(55248);

	// bind �� listen
	bind(m_hServerSocket, (SOCKADDR*)&m_ServerAddress, sizeof(m_ServerAddress));
	listen(m_hServerSocket, SOMAXCONN);

	// listen �غ� ������ accept�� ���� accept thread�� �ϳ� ����
	_beginthreadex(NULL, 0, acceptProcedure, NULL, 0, NULL);

	// ��� ���� ���� �غ� ������ ������ IP�ּҿ� ��Ʈ��ȣ�� �ܼ�â�� ����Ͽ� �˸�
	gethostname(hostname, 256);
	host = gethostbyname(hostname);
	for (int i = 0; i < 4; i++)
		ipAddr[i] = *(host->h_addr_list[0]+i);

	puts("=============���� �ʱ� ���� �Ϸ�!=============\n");
	printf("���� �ּ� -> %d.%d.%d.%d/%d\n", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3], PORT);

	return true;
}

// Ŭ���̾�Ʈ�� ���ӿ� ���� ������ ����ϴ� accept thread�� proc �Լ�
unsigned int __stdcall CIOCPServer::acceptProcedure(void* nullpt)
{
	SOCKET				hClientSocket;		// accept�� Ŭ���̾�Ʈ ������ ���� �ڵ�
	SOCKADDR_IN			clientAddress;		// Ŭ���̾�Ʈ �ּ� ����ü
	LPPER_HANDLE_DATA	handleInfo;			// Ŭ���̾�Ʈ�� �������ִ� ���� �����ϱ� ���� ����ü
	LPPER_IO_DATA		ioInfo;				// Ŭ���̾�Ʈ�κ��� �Է¹ޱ� ���� �Ҵ�Ǵ� ����ü

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

	// �ذ��ؾ� �� ����
	// 1. �� ��Ŷ�� ���� ���� �߷��� �� ���� �ִ�. (��Ʈ��ũ ȯ�� �Ҿ��� or MSS�� �ʰ��ϴ� ��Ŷ)
	// 2. ���� ���� ��Ŷ�� ���ļ� �ϳ��� �� ���� �ִ�. (Ŭ���̾�Ʈ���� ������ �����ϸ� ����...)
	// >> handleInfo�� packetQueue�� �߰��Ͽ�, Ư�� ���ڿ��� �������� ��Ŷ�� ���� �� �籸��
	// >> ��Ŷ �ۼ��� ���� �� �������� ����

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
				// EOF(���� ���� ��ȣ) ���� ��
				if (transferredBytes == 0)
				{
					CIOCPServer::GetInstance()->closeClient(&handleInfo, &ioInfo);
					continue;
				}

				// �� �κ��� ��ü �������ݿ� ����Ͽ� PacketQueue�� �̿��ϴ� ��� ����̴�.
				// ��Ŷ�� �߸��ų� �پ �� ���� ���� ���� ��Ŷ�� ó���Ͽ� �ش�.
				// OnRecv()�� ��ȯ���� Ŭ����ȭ�Ǿ� �ٷ� ����� �� �ִ� ��Ŷ�� �����̴�.
				int recv = handleInfo->packetQueue->OnRecv(ioInfo->buffer, transferredBytes);

				// OnRecv()���� -1�� ��ȯ�ϸ� ���� �߸��Ǿ����� Ŭ���̾�Ʈ�� ������ �����ϸ� �ȴ�.
				if (recv == -1)	CIOCPServer::GetInstance()->closeClient(&handleInfo, &ioInfo);

				// �̹� WSARecv�� ���� ioInfo�� �� �̻� �ʿ����� ������ �����Ѵ�.
				free(ioInfo);

				// ��� ������ ��Ŷ ������ŭ �м� �� �۾� ��û�� ��� �����Ѵ�.
				// �ܺ� ��⿡�� �۾��� �ܺ� ����� �����带 �̿��ϸ� ����� ���߿� �޴´�.
				// ��� ó�� ������ ���� �۾��� ��ٷ� ó���� �ش�.
				for (int i = 0; i < recv; i++)
				{
					// 1. CPacket -> ���� �Ҵ� �� ���� ����������, ���������� ���� ��
					// �ش� ����� �������� �� �ڵ����� Ŭ������ ���� �Ҵ�� �޸𸮰� �Ҹ�ȴ�.
					CPacket packet = *(handleInfo->packetQueue->getPacket());
					unsigned short type = packet.GetPacketType();
					unsigned short size = packet.GetDataSize();
					char* charContent = packet.GetContent();

					switch (packet.GetPacketType())
					{
						// STANDARD ��Ŷ�� �⺻���� Echo���� ��Ŷ�̴�.
						// ���� �����͸� ����ϰ� �ٽ� Ŭ���̾�Ʈ�� �����ش�.
					case PACKET_TYPE_STANDARD:
					{
						// �۾� ���� : Echo ���
						CStandardPacketContent content(charContent, size); // ���� ����
						cout << "STANDARD ��Ŷ ���� :: " << content.GetCommand() << endl;
						
						// �� ���� �ִ� packet�� ���� case ��� Ż�� �� �ڵ��Ҹ�ȴ�.
						CPacket sendPacket(charContent, type, size);
						// 2. LPPER_IO_PACKET -> Encode() �Լ��� ���� �Ҵ� �� ���� ���簡 �Ǹ�
						// ����ü�̱� ������ �Ҹ��ϱ� �� �ݵ�� free()�� ����� �Ѵ�.
						LPPER_IO_PACKET sendData = sendPacket.Encode();
						// 3. LPPER_IO_DATA -> multiSendCount�� 0�� �� �� ��Ư�� �����忡�� �Ҹ�ȴ�.
						LPPER_IO_DATA sendIoInfo;
						GetInstance()->initWSASend(&sendIoInfo, sendData->data, sendData->size);
						WSASend(socket, &(sendIoInfo->wsaBuf), 1, NULL, 0, &(sendIoInfo->overlapped), NULL);
						free(sendData->data);
						free(sendData);
						break;
					}
					}
				}

				// �׸��� ���� ��Ŷ ������ ��ٸ���.
				LPPER_IO_DATA newIoInfo;
				GetInstance()->initWSARecv(&newIoInfo);
				WSARecv(socket, &(newIoInfo->wsaBuf), 1, NULL, &flags, &(newIoInfo->overlapped), NULL);
			}
			// ���� �Ϸ� ��
			else
			{
				// CRITICAL_SECTION (���� �۽��� ��� ������ ���������� ����)
				if (--(ioInfo->multiSendCount) == 0)
					free(ioInfo);
				// CRITICAL_SECTION (���� �۽��� ��� ������ ���������� ����)
			}
		}
		// ���ϰ��� 0�� ��� (���� ����ó���� �ؾ� �ϴ� ����� �������� ���)
		else
		{
			int err = WSAGetLastError();
			switch (err)
			{
			case 64: // Ŭ���̾�Ʈ(����) ������ �̹� ����Ǿ����� �ǹ��ϴ� �ڵ�
				puts("ERROR CODE 64 :: Ŭ���̾�Ʈ ���μ����� ���Ḧ �����Ͽ�, �ش� Ŭ���̾�Ʈ���� ������ �����մϴ�.");
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
	*ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
	memset(&((*ioInfo)->overlapped), 0, sizeof(OVERLAPPED));
	memset(&(*ioInfo)->buffer, 0, sizeof((*ioInfo)->buffer));
	memcpy((*ioInfo)->buffer, message, msgLength);
	(*ioInfo)->wsaBuf.len = msgLength;
	(*ioInfo)->wsaBuf.buf = (*ioInfo)->buffer;
	(*ioInfo)->readWriteFlag = WRITE;
	(*ioInfo)->multiSendCount = 1;
}

void CIOCPServer::initWSASend(LPPER_IO_DATA* ioInfo, const char* message, int msgLength, int multiSendCount)
{
	*ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
	memset(&((*ioInfo)->overlapped), 0, sizeof(OVERLAPPED));
	memset(&(*ioInfo)->buffer, 0, sizeof((*ioInfo)->buffer));
	memcpy((*ioInfo)->buffer, message, msgLength);
	(*ioInfo)->wsaBuf.len = msgLength;
	(*ioInfo)->wsaBuf.buf = (*ioInfo)->buffer;
	(*ioInfo)->readWriteFlag = WRITE;
	(*ioInfo)->multiSendCount = multiSendCount;
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