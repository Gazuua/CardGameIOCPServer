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
	DeleteCriticalSection(&GetInstance()->m_CS);
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

	// CRITICAL_SECTION �ڵ��� �̸� �Ҵ�
	InitializeCriticalSection(&GetInstance()->m_CS);

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
	::bind(m_hServerSocket, (SOCKADDR*)&m_ServerAddress, sizeof(m_ServerAddress));
	listen(m_hServerSocket, SOMAXCONN);

	// listen �غ� ������ accept�� ���� accept thread�� �ϳ� ����
	_beginthreadex(NULL, 0, acceptProcedure, NULL, 0, NULL);

	// ��� ���� ���� �غ� ������ ������ IP�ּҿ� ��Ʈ��ȣ�� �ܼ�â�� ����Ͽ� �˸�
	gethostname(hostname, 256);
	host = gethostbyname(hostname);
	for (int i = 0; i < 4; i++)
		ipAddr[i] = *(host->h_addr_list[0]+i);

	GetInstance()->m_ClientList.clear();
	GetInstance()->m_RoomList.clear();
	GetInstance()->m_nLastRoomIndex = 0;

	puts("============= IOCP ���� �ʱ� ���� �Ϸ�!=============");
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
		// Ŭ���̾�Ʈ�� accept �޴´�
		hClientSocket = accept((SOCKET)GetInstance()->m_hServerSocket, (SOCKADDR*)&clientAddress, &addrLength);

		// Ŭ���̾�Ʈ�� ���� ���� �� ���� HANDLE_DATA ����ü�� �Ҵ��Ѵ�.
		handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
		handleInfo->hClientSocket = hClientSocket;
		memcpy(&(handleInfo->clientAddress), &clientAddress, addrLength);
		handleInfo->packetQueue = new CPacketQueue();

		// Ŭ���̾�Ʈ ����Ʈ(Map)�� ���� ������ Ŭ���̾�Ʈ�� �߰����ش�.
		// ���� ����� ���� �����̹Ƿ� �Ӱ迵�� ó�����ش�.
		EnterCriticalSection(&GetInstance()->m_CS);
		GetInstance()->m_ClientList[hClientSocket] = new CClient(hClientSocket);
		LeaveCriticalSection(&GetInstance()->m_CS);

		// Ŭ���̾�Ʈ�� ������ �ܼ�â�� �˸���.
		printf("Client Accepted :: %s/%d\n", inet_ntoa(clientAddress.sin_addr), clientAddress.sin_port);

		// �ش� ������ Completion Port�� �߰��Ͽ� �����Ѵ�.
		CreateIoCompletionPort((HANDLE)hClientSocket, GetInstance()->m_hCP, (DWORD)handleInfo, 0);

		// ���� WSARecv�� ���� �����Ѵ�.
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

	while (1)
	{
		// GQCS(Ŭ���̾�Ʈ���� ��� �۾��� �Ϸ�Ǹ� ��ȯ�ϴ� �Լ�)�� ���ϰ��� �޴´�.
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
				if (recv == -1) {
					CIOCPServer::GetInstance()->closeClient(&handleInfo, &ioInfo);
					continue;
				}

				// �̹� WSARecv�� ���� ioInfo�� �� �̻� �ʿ����� ������ �����Ѵ�.
				free(ioInfo);

				// ��� ������ ��Ŷ ������ŭ �м� �� �۾� ��û�� ��� �����Ѵ�.
				// �ܺ� ��⿡�� �۾��� �ܺ� ����� �����带 �̿��ϸ� ����� ���߿� �޴´�.
				// ��� ó�� ������ ���� �۾��� ��ٷ� ó���� �ش�.
				for (int i = 0; i < recv; i++)
				{
					// ������ ��Ŷ�� ���� �и��Ѵ�.
					// ���� ������ ��Ŷ�� ���� ������ for�� Ż�� �� �ڵ����� �Ҹ�ȴ�.
					CPacket packet = *(handleInfo->packetQueue->getPacket());
					char* charContent = packet.GetContent();
					unsigned short type = packet.GetPacketType();
					unsigned short size = packet.GetDataSize();

					switch (packet.GetPacketType())
					{
					// STANDARD ��Ŷ�� �⺻���� Echo���� ��Ŷ�̴�.
					case PACKET_TYPE_STANDARD:
					{
						// 1. ���� ������ ����Ѵ�.
						string command(charContent, size);
						cout << "STANDARD ��Ŷ ���� :: " << command << endl;
						
						// 2. ���� ������ �״�� Ŭ���̾�Ʈ�� �����Ѵ�.
						GetInstance()->sendRequest(socket, charContent, type, size);
					}
					break;

					// LOGIN_REQ ��Ŷ�� Ŭ���̾�Ʈ�� �α��� ��û�̴�.
					case PACKET_TYPE_LOGIN_REQ:
					{
						// 1. ����� �� ���� ������ ������ �����Ѵ�.
						bool bSuccess = false;
						char result[7];
						int resultSize = 0;

						// 2. Ŭ���̾�Ʈ�κ��� ���� ���̵�� ��й�ȣ�� �и��Ѵ�.
						string info(charContent, size);
						string id(info.substr(0, info.find('/')));
						string pw(info.substr(info.find('/') + 1));

						// 3. DB ��⿡ ����� �α��� ��û �Լ��� ȣ���Ѵ�.
						// �α����� �����Ѵٸ� LoginRequest()�� 3��° �μ��� ���� CClient*��
						// �ش� ������ ID, ����, �������� ����ȴ�.
						bSuccess = CDataBaseManager::GetInstance()
							->LoginRequest(id, pw, GetInstance()->m_ClientList[socket]);

						// 4. �����ϸ� bSuccess�� ���� ���ΰ� ��ȯ�ȴ�.
						// �̿� ���� Ŭ���̾�Ʈ�� ������ ���ڿ��� �����Ѵ�.
						if (bSuccess) {
							resultSize = 7;
							memcpy(result, "success", resultSize);
						}
						else {
							resultSize = 6;
							memcpy(result, "failed", resultSize);
						}

						// 5. �α��� ��û ����� Ŭ���̾�Ʈ�� �����Ѵ�.
						GetInstance()->sendRequest(socket, result, PACKET_TYPE_LOGIN_RES, resultSize);
					}
					break;

					// REGISTER_REQ ��Ŷ�� Ŭ���̾�Ʈ�� ȸ������ ��û�̴�.
					case PACKET_TYPE_REGISTER_REQ:
					{
						// 1. ����� �� ���� ������ ������ �����Ѵ�.
						bool bSuccess = false;
						char result[7];
						int resultSize = 0;

						// 2. Ŭ���̾�Ʈ�κ��� ���� ���̵�� ��й�ȣ�� �и��Ѵ�.
						string info(charContent, size);
						string id(info.substr(0, info.find('/')));
						string pw(info.substr(info.find('/') + 1));

						// 3. DB ��⿡ ����� ȸ������ ��û �Լ��� ȣ���Ѵ�.
						if (id.length() >= 4 && pw.length() >= 4)
							bSuccess = CDataBaseManager::GetInstance()->RegisterRequest(id, pw);
						
						// 4. �����ϸ� bSuccess�� ���� ���ΰ� ��ȯ�ȴ�.
						// �̿� ���� Ŭ���̾�Ʈ�� ������ ���ڿ��� �����Ѵ�.
						if (bSuccess) {
							resultSize = 7;
							memcpy(result, "success", resultSize);
						}
						else {
							resultSize = 6;
							memcpy(result, "failed", resultSize);
						}

						// 5. ȸ������ ��û ����� Ŭ���̾�Ʈ�� �����Ѵ�.
						GetInstance()->sendRequest(socket, result, PACKET_TYPE_REGISTER_RES, resultSize);
					}
					break;

					// LOBBY_REQ ��Ŷ�� Ŭ���̾�Ʈ�� �κ� ���� ��û�̴�.
					case PACKET_TYPE_ENTER_LOBBY_REQ:
					{
						// 1. ����� �� ���� ������ ������ �����Ѵ�.
						bool bSuccess = false;
						char result[2000] = "";
						int resultSize = 0;
						CClient* client = GetInstance()->m_ClientList[socket];

						// 2. ��û Ŭ���̾�Ʈ�� ���̵� �����Ѵ�.
						string id(charContent, size);

						// 3. �ش��ϴ� ���̵� ���� ȸ���� ������ DB�� ��û�Ѵ�.
						// ���� �� UserInfoRequest()�� �� ��° �μ��� ������ CClient*�� ������ ����ȴ�.
						bSuccess = CDataBaseManager::GetInstance()->UserInfoRequest(id, client);

						// 4. �����ϸ� bSuccess�� ���� ���ΰ� ��ȯ�ȴ�.
						// �����ϸ� �ش� Ŭ���̾�Ʈ�� ������ �����ڷ� ��� result�� �����Ѵ�.
						if (bSuccess) {
							resultSize = sprintf(result, "%s/%d/%d/%d/%d",
								client->getID().c_str(), client->getWin(),
								client->getLose(), client->getMoney(), client->getRoom());
						}
						// 4.5. ���� ó�� - ���� �ش��ϴ� ���̵��� ������ ���ٸ� result��
						// "null" �̶�� �����Ѵ�.
						else {
							strcpy(result, "null");
							resultSize = 4;
						}

						// 5. �ش� ������ ������ �����Ѵ�.
						GetInstance()->sendRequest(socket, result, PACKET_TYPE_USER_INFO_RES, resultSize);
						
						// 6. �� ����� �����ϱ� ���� ��� ����� �������� �ʱ�ȭ���ش�.
						bSuccess = false;
						memset(result, 0, sizeof(result));
						strcpy(result, "");
						resultSize = 0;

						// 7. �� ��ȣ, �� �̸�, ���� �ο��� ������ ������ '/'�� �̿��� result�� �����Ѵ�.
						map<int, CGameRoom*>::iterator iter;
						for (iter = GetInstance()->m_RoomList.begin(); 
							iter != GetInstance()->m_RoomList.end(); iter++)
						{
							char temp[100];
							CGameRoom* room = iter->second;
							resultSize += sprintf(temp, "%d/%s/%d/",
								iter->first, room->GetName().c_str(), room->GetUserNumber());

							strcat(result, temp);
						}

						// 8. �ִ� ���̸� 1 ������ ������ �����ڴ� ���� �ʰ� �Ѵ�.
						resultSize--;

						// 8.5. ����ó�� - ���� ��� ����Ʈ ����� 0�̸� "null"�� ������.
						if (GetInstance()->m_RoomList.size() == 0)
						{
							strcpy(result, "null");
							resultSize = 4;
						}

						// 9. �� ����Ʈ�� �����Ѵ�.
						GetInstance()->sendRequest(socket, result, PACKET_TYPE_ROOM_LIST_RES, resultSize);
					}
					break;
					
					// ROOM_MAKE_REQ ��Ŷ�� Ŭ���̾�Ʈ�� �� ����� ��û�̴�.
					case PACKET_TYPE_ROOM_MAKE_REQ:
					{
						// 1. ����� �� ���� ������ ������ �����Ѵ�.
						char result[20];
						int resultSize = 0;

						// 2. Ŭ���̾�Ʈ�κ��� ���� �� ������ �����Ѵ�.
						string title(charContent, size);

						// 3. ���ο� ���� �����Ѵ�.
						// ���ο� �� ��ȣ�� �ε��̿� ���� ��ĥ ���� ����� ����.
						CGameRoom* room = new CGameRoom();
						CClient* client = GetInstance()->m_ClientList[socket];
						int roomIndex = GetInstance()->m_nLastRoomIndex;
						
						// 4. Ŭ���̾�Ʈ�� �����Ű�� �� ������ �����ϸ�, �̸� �ش� �濡�� �����Ѵ�.
						client->OnEnterRoom(roomIndex);
						room->SetName(title);
						room->OnEnter(client);

						// 5. CRITICAL_SECTION ====================================
						// CIOCPServer�� ���� ����� �� ����Ʈ�� ���� ���� ���� �߰��Ѵ�.
						EnterCriticalSection(&GetInstance()->m_CS);
						GetInstance()->m_RoomList[roomIndex] = room;
						GetInstance()->m_nLastRoomIndex++;
						LeaveCriticalSection(&GetInstance()->m_CS);
						// CRITICAL_SECTION ====================================

						// 6. �� �� ��ȣ�� Ŭ���̾�Ʈ�� �����Ѵ�.
						resultSize = sprintf(result, "%d", roomIndex);
						GetInstance()->sendRequest(socket, result, PACKET_TYPE_ROOM_MAKE_RES, resultSize);
					}
					break;

					// ENTER_ROOM_REQ ��Ŷ�� Ŭ���̾�Ʈ�� �� ���� ��û�̴�.
					case PACKET_TYPE_ENTER_ROOM_REQ:
					{
						// 1. ����� �� ���� ������ ������ �����Ѵ�.
						char result[20];
						int resultSize = 0;
						int ret = 0;

						// 2. �ش� Ŭ���̾�Ʈ�� �����ϰ� �;��ϴ� ���� �����Ѵ�.
						int number = atoi(charContent);

						// 3. �ش��ϴ� ���� �����ϴ��� �˻��Ѵ�.
						CGameRoom* room = GetInstance()->m_RoomList[number];
						CClient* client = GetInstance()->m_ClientList[socket];

						// 4. ���� �������� �ʴ´ٸ� ��ȯ���� -1�� �����Ѵ�.
						if (room == NULL) ret = -1;
						// 4.5. �����Ѵٸ�
						else
						{
							// 5. �ش� ���� ���°� READY�������� üũ�Ѵ�.
							if (room->GetRoomState() == GAME_STATE_READY)
								// READY ���¿��� �ο��� ���� �� �ִٸ� -1�� ��ȯ�ȴ�.
								ret = room->OnEnter(client);
							// 5.5. READY ���°� �ƴϸ� �̹� �������̶�� ���̹Ƿ� ��ȯ���� -1��.
							else
								ret = -1;
						}

						// 6. ���������� ret���� �˻��Ͽ� -1�� �ƴϸ� �����̴�.
						if (ret != -1)
						{
							// ���� �� ���� ���� Ŭ���̾�Ʈ�� �濡 ������� �ش�.
							client->OnEnterRoom(number);
							strcpy(result, "success");
							resultSize = 7;
						}
						// 6.5. ret���� -1�̸� �����̴�.
						else {
							// ���� �� ���� ��Ŷ�� ������.
							strcpy(result, "failed");
							resultSize = 6;
						}
						
						// 7. ���� ��û�� ���� ����� Ŭ���̾�Ʈ�� �����Ѵ�.
						GetInstance()->sendRequest(socket, result, PACKET_TYPE_ENTER_ROOM_RES, resultSize);
					}
					break;

					// ROOM_INFO_REQ ��Ŷ�� �濡 ���� ������ Ŭ���̾�Ʈ�� ���� ��û�̴�.
					case PACKET_TYPE_ROOM_INFO_REQ:
					{
						// 1. �ش� Ŭ���̾�Ʈ�� ������ �� ��ȣ�� �޾� �����Ѵ�.
						int number = atoi(charContent);

						// 2. �ش��ϴ� ��ȣ�� ���� �� ����Ʈ���� �����´�.
						CGameRoom* room = GetInstance()->m_RoomList[number];

						// 3. �� �̸��� �迭�� �����Ѵ�.
						char roomName[100];
						int nameSize = room->GetName().size();
						strcpy(roomName, room->GetName().c_str());

						// 4. �� �̸��� Ŭ���̾�Ʈ�� �����Ͽ� �ش�.
						GetInstance()->sendRequest(socket, roomName, PACKET_TYPE_ROOM_INFO_RES, nameSize);

						// 5. ���� �ش� �� �ȿ� �ִ� ���� ������ �����ֱ� ���� iterator�� �����Ѵ�.
						list<CClient*>::iterator iter;
						list<CClient*> list = room->GetUserList();

						// 6. ����� �� ���� ������ ������ �����Ѵ�.
						// result�� ��� strcat���� �̾���̱� ���� �ʱⰪ�� �������� �ʱ�ȭ���ش�.
						char result[500] = "";
						int resultSize = 0;

						// 7. ���� ������ ������ '/'�� ���´�.
						// ������ "�̸�/��/�̸�/��....." �̴�.
						for (iter=list.begin(); iter != list.end(); iter++)
						{
							char temp[50];
							resultSize += sprintf(temp, "%s/%d/",
								(*iter)->getID().c_str(), (*iter)->getMoney());

							strcat(result, temp);
						}
						
						// 8. ������ �����ڸ� �����ϱ� ���� resultSize�� 1 �ٿ��ش�.
						resultSize--;

						// 9. �� �ȿ� �ִ� ��� Ŭ���̾�Ʈ�� ������ �����Ѵ�.
						for (iter = list.begin(); iter != list.end(); iter++)
						{
							GetInstance()->sendRequest((*iter)->getSocket(), result,
								PACKET_TYPE_ROOM_USER_RES, resultSize);
						}
					}
					break;

					// EXIT_ROOM_REQ ��Ŷ�� Ŭ���̾�Ʈ�� �濡�� ���� �� ������ �����ϴ� ��Ŷ�̴�.
					case PACKET_TYPE_EXIT_ROOM_REQ:
					{
						// 1. �ش� Ŭ���̾�Ʈ�� ������ �� ��ȣ�� �޾� �����Ѵ�. 
						int number = atoi(charContent);
						int roomSize = 0;

						// 2. �ش��ϴ� ��ȣ�� ���� �� ����Ʈ���� �����´�.
						CGameRoom* room = GetInstance()->m_RoomList[number];
						CClient* client = GetInstance()->m_ClientList[socket];

						// 3. �ش��ϴ� Ŭ���̾�Ʈ�� �����Ų��.
						roomSize = room->OnExit(client);
						client->OnExitRoom();

						// 4. ���� �濡 ����� ���ٸ� ���� ���Ľ�Ű�� case ����� ����������.
						if (roomSize == 0) {
							GetInstance()->m_RoomList.erase(number);
							delete room;
							break;
						}

						// 5. �ش� �� �ȿ� �ִ� ���� ������ �����ֱ� ���� iterator�� �����Ѵ�.
						list<CClient*>::iterator iter;
						list<CClient*> list = room->GetUserList();

						// 6. ����� �� ���� ������ ������ �����Ѵ�.
						// result�� ��� strcat���� �̾���̱� ���� �ʱⰪ�� �������� �ʱ�ȭ���ش�.
						char result[500] = "";
						int resultSize = 0;

						// 7. ���� ������ ������ '/'�� ���´�.
						// ������ "�̸�/��/�̸�/��....." �̴�.
						for (iter = list.begin(); iter != list.end(); iter++)
						{
							char temp[50];
							resultSize += sprintf(temp, "%s/%d/",
								(*iter)->getID().c_str(), (*iter)->getMoney());

							strcat(result, temp);
						}

						// 8. ������ �����ڸ� �����ϱ� ���� resultSize�� 1 �ٿ��ش�.
						resultSize--;

						// 9. �� �ȿ� �ִ� ��� Ŭ���̾�Ʈ�� ������ �����Ѵ�.
						for (iter = list.begin(); iter != list.end(); iter++)
						{
							GetInstance()->sendRequest((*iter)->getSocket(), result, 
								PACKET_TYPE_ROOM_USER_RES, resultSize);
						}
					}
					break;

					// ROOM_CHAT_REQ�� Ŭ���̾�Ʈ�� ���� ä�� �޼����̴�.
					case PACKET_TYPE_ROOM_CHAT_REQ:
					{
						// 1. Ŭ���̾�Ʈ�� ���� �ִ� ���� �˾Ƴ���.
						CGameRoom* room = GetInstance()->m_RoomList
							[GetInstance()->m_ClientList[socket]->getRoom()];

						// 2. �ش� �� �ȿ� �ִ� �����鿡�� �����ֱ� ���� iterator�� �����Ѵ�.
						list<CClient*>::iterator iter;
						list<CClient*> list = room->GetUserList();

						// 3. �� �ȿ� �ִ� ��� Ŭ���̾�Ʈ�� �޼����� �����Ѵ�.
						for (iter = list.begin(); iter != list.end(); iter++)
						{
							GetInstance()->sendRequest((*iter)->getSocket(), charContent,
								PACKET_TYPE_ROOM_CHAT_RES, size);
						}
					}
						break;

					// ROOM_START_REQ�� Ŭ���̾�Ʈ�� ���� ���� ��û�̴�.
					case PACKET_TYPE_ROOM_START_REQ:
					{
						// 1. �� ��ȣ�� ��Ŷ���� ���Ƿ� ���� ��´�.
						int number = atoi(charContent);
						CGameRoom* room = GetInstance()->m_RoomList[number];

						// 2. ���� ���¸� ���� �������� �ٲ۴�.
						room->OnStart();

						// 2. �ش� �� �ȿ� �ִ� �����鿡�� �˷��ֱ� ���� iterator�� �����Ѵ�.
						list<CClient*>::iterator iter;
						list<CClient*> list = room->GetUserList();

						// 3. �� �ȿ� �ִ� ��� Ŭ���̾�Ʈ�� �޼����� �����Ѵ�.
						for (iter = list.begin(); iter != list.end(); iter++)
						{
							GetInstance()->sendRequest((*iter)->getSocket(), charContent,
								PACKET_TYPE_ROOM_START_RES, size);
						}
					}
						break;

					// ENTER_GAME_REQ�� ���ӿ� ���� Ŭ���̾�Ʈ�� ������ ��û�ϴ� ��Ŷ�̴�.
					case PACKET_TYPE_ENTER_GAME_REQ:
					{
						// 1. Ŭ���̾�Ʈ�� ���� �ִ� ���� �˾Ƴ���.
						int num = atoi(charContent);
						char result[50];
						int resultSize = 0;

						// 2. ��� ���������� �Ѹ���.
						GetInstance()->RefreshGameRoomInfo(num);

						// 3. ī�� ������ �Ѹ���.
						// ī�� ������ game�����׸�Ʈ ���� ���Խÿ� �� ���� �ֹǷ�..
						CClient* client = GetInstance()->m_ClientList[socket];
						CCard* one = client->getCardOne();
						CCard* two = client->getCardTwo();

						// 4. ���� ���� "����/������/����/������" �� ������.
						resultSize = sprintf(result, "%d/%d/%d/%d", one->GetNumber(), one->GetSpecial(),
							two->GetNumber(), two->GetSpecial());

						// 5. ī�� ������ ��û�ڿ��� �Ѹ���.
						GetInstance()->sendRequest(socket, result, PACKET_TYPE_GAME_CARD_RES, resultSize);
					}
						break;
					
					// GAME_BETTING_REQ�� Ŭ���̾�Ʈ�� �õ��� ���� ��û�̴�.
					case PACKET_TYPE_GAME_BETTING_REQ:
					{
						// 1. Ŭ���̾�Ʈ�� ���� ���� ������ �����Ѵ�.
						int bet = atoi(charContent);

						// 2. Ŭ���̾�Ʈ�� Ŭ���̾�Ʈ�� ���� �ִ� ���� �˾Ƴ���.
						CClient* client = GetInstance()->m_ClientList[socket];
						CGameRoom* room = GetInstance()->m_RoomList
							[client->getRoom()];

						// 3. ���� ������ ���� Ŭ���̾�Ʈ�� ���� ���¸� �����Ѵ�.
						switch (bet)
						{
							// �׾����� �׾��ٰ� �������ش�.
						case GAME_BETTING_DIE:
							client->setDie(true);
							break;
							// ���̸� �����ǵ���ŭ Ŭ���̾�Ʈ�� ���øӴϸ� �ø���.
						case GAME_BETTING_CALL:
							client->setBetMoney(room->GetMoney());
							room->AddMoneySum(client->getBetMoney());
							break;
							// �����̸� �����ǵ��� 2��� ���øӴϸ� �ø���.
						case GAME_BETTING_DOUBLE:
						{
							room->SetMoney(room->GetMoney() * 2);
							client->setBetMoney(room->GetMoney());
							room->AddMoneySum(client->getBetMoney());

							// ������ ������ �ɾ����� �� �����϶�� ����� ��ο��� �˸���.
							list<CClient*>::iterator iter;
							list<CClient*> list = room->GetUserList();
							char result[10] = "abc";
							int resultSize = sizeof("abc");
							for (iter = list.begin(); iter != list.end(); iter++)
								GetInstance()->sendRequest((*iter)->getSocket(), result, 
									PACKET_TYPE_GAME_BETTING_DOUBLE_RES, resultSize);
						}
							break;
						}

						// 4. ������ ������ ���� :: 1�� ���� �� �װų�
						// ����ִ� ��ΰ� �����ǵ���ŭ�� ���� �����ߴ��� Ȯ���Ѵ�.
						list<CClient*>::iterator iter;
						list<CClient*> list = room->GetUserList();
						int alive = 0;
						int called = 0;

						for (iter = list.begin(); iter != list.end(); iter++)
						{
							// ����ִ� �� ī��Ʈ +
							if (!(*iter)->isDead()) 
								alive++;

							// ���رݾ� �̻� ������ ��� +
							if ((*iter)->getBetMoney() >= room->GetMoney()) 
								called++;
						}

						char result[5000];
						memset(result, 0, sizeof(result));
						int resultSize = 0;

						// ����ִ� ����� �� ���̴�?
						if (alive == 1) {
							// ����ִ� ���(����)���� ���� �ְ� ���� ���� ��Ŷ�� ������.
							// ���� ���� ��Ŷ���� ������ �̸�/�����ڵ�/���� ���ΰ� ���� �ȴ�.
							for (iter = list.begin(); iter != list.end(); iter++)
							{
								char temp[50];

								if (!(*iter)->isDead()) {
									CDataBaseManager::GetInstance()->UserWinRequest(
										*iter, room->GetMoneySum() - (*iter)->getBetMoney()
									);
								}
								else {
									CDataBaseManager::GetInstance()->UserLoseRequest(
										*iter, (*iter)->getBetMoney()
									);
								}

								resultSize += sprintf(temp, "%s/%d/%d/",
									(*iter)->getID().c_str(),
									(*iter)->getJokboCode(),
									(*iter)->isDead());

								strcat(result, temp);
							}

							// ������ �����ڸ� �����ϱ� ���� �ε����� 1 ����.
							resultSize--;

							for (iter = list.begin(); iter != list.end(); iter++)
							{
								// Ŭ���̾�Ʈ �ȿ� ���ӿ� ������� �ʱ�ȭ �� �����Ѵ�.
								(*iter)->OnEndGame();

								// ��ο��� ������ �������� �˸���.
								GetInstance()->sendRequest((*iter)->getSocket(), result,
									PACKET_TYPE_GAME_SET_RES, resultSize);
							}
							// �濡 ī�� ����Ʈ�� ������
							room->ReleaseCardSet();
							room->SetRoomState(GAME_STATE_READY);
							// case ���� ����������.
							break;
						}

						int maxJokbo = 0;
						bool equal = false;

						// ����ִ� ��� ����� ���رݾ� �̻� �ݺ����� �ߴ�?
						if (called == alive) {
							// ������ ���ϰ� ���и� ������ ��
							// ���ڿ��� ���� �ְ� ���� ���� ��Ŷ�� ������.
							// ���ºθ� �׳� ������ �ȴ�.
							for (iter = list.begin(); iter != list.end(); iter++)
							{
								char temp[50];
								
								if(maxJokbo == 0) maxJokbo = (*iter)->getJokboCode();
								else {
									if (maxJokbo < (*iter)->getJokboCode())
										maxJokbo = (*iter)->getJokboCode();
									else if (maxJokbo == (*iter)->getJokboCode())
										equal = true;
								}

								resultSize += sprintf(temp, "%s/%d/%d/",
									(*iter)->getID().c_str(),
									(*iter)->getJokboCode(),
									(*iter)->isDead());

								i++;
								strcat(result, temp);
							}

							// ������ �����ڸ� �����ϱ� ���� �ε����� 1 ����.
							resultSize--;

							i = 0;
							// ���ºθ� �ƹ��͵� �� ���൵ �ǰ�
							// ���ºΰ� �ƴϸ� DBó���� �Ѵ�.
							if (!equal) {
								for (iter = list.begin(); iter != list.end(); iter++)
								{
									if (maxJokbo == (*iter)->getJokboCode()) {
										CDataBaseManager::GetInstance()->UserWinRequest(
											*iter, room->GetMoneySum() - (*iter)->getBetMoney()
										);
									}
									else {
										CDataBaseManager::GetInstance()->UserLoseRequest(
											*iter, (*iter)->getBetMoney()
										);
									}
								}
							}

							// ����������
							for (iter = list.begin(); iter != list.end(); iter++)
							{
								// Ŭ���̾�Ʈ �ȿ� ���ӿ� ������� �ʱ�ȭ �� �����Ѵ�.
								(*iter)->OnEndGame();

								// ��ο��� ������ �������� �˸���.
								GetInstance()->sendRequest((*iter)->getSocket(), result,
									PACKET_TYPE_GAME_SET_RES, resultSize);
							}

							// �濡 ī�� ����Ʈ�� ������
							room->ReleaseCardSet();
							room->SetRoomState(GAME_STATE_READY);
							// �׸��� case ���� ����������.
							break;
						}

						// 5. ������� ������ ���� ���� ���� �ƴϹǷ�, ���ӹ� ���¸� ���������Ѵ�.
						GetInstance()->RefreshGameRoomInfo(client->getRoom());
					}
						break;

					default:
						break;
					}
				}

				// ����� ��û�� ��� �����ٸ� ���� ��Ŷ ������ ���� WSARecv�� �����Ѵ�.
				LPPER_IO_DATA newIoInfo;
				GetInstance()->initWSARecv(&newIoInfo);
				WSARecv(socket, &(newIoInfo->wsaBuf), 1, NULL, &flags, &(newIoInfo->overlapped), NULL);
			}
			// ���� �Ϸ� �� 
			else free(ioInfo);
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

void CIOCPServer::RefreshRoomInfo(int num)
{
	// �� �Լ��� �������� ���̷�Ʈ�� Ư�� ���� Ŭ���̾�Ʈ�鿡��
	// �� ������ ���� ��Ŷ�� ���� ���ΰ�ħ�� �ִ� �Լ��̴�.
	CGameRoom* room = this->m_RoomList[num];

	// �ش� �� �ȿ� �ִ� ���� ������ �����ֱ� ���� iterator�� �����Ѵ�.
	list<CClient*>::iterator iter;
	list<CClient*> list = room->GetUserList();

	// ����� �� ���� ������ ������ �����Ѵ�.
	// result�� ��� strcat���� �̾���̱� ���� �ʱⰪ�� �������� �ʱ�ȭ���ش�.
	char result[500] = "";
	int resultSize = 0;
	
	// ���� ������ ������ '/'�� ���´�.
	// ������ "�̸�/��/�̸�/��....." �̴�.
	for (iter = list.begin(); iter != list.end(); iter++)
	{
		char temp[50];
		resultSize += sprintf(temp, "%s/%d/",
			(*iter)->getID().c_str(), (*iter)->getBetMoney());

		strcat(result, temp);
	}

	// ������ �����ڸ� �����ϱ� ���� resultSize�� 1 �ٿ��ش�.
	resultSize--;

	// �� �ȿ� �ִ� ��� Ŭ���̾�Ʈ�� ������ �����Ѵ�.
	for (iter = list.begin(); iter != list.end(); iter++)
	{
		GetInstance()->sendRequest((*iter)->getSocket(), result,
			PACKET_TYPE_ROOM_USER_RES, resultSize);
	}
}

void CIOCPServer::RefreshGameRoomInfo(int num)
{
	// �� �Լ��� �������� ���̷�Ʈ�� ���� ���� Ư�� ���� Ŭ���̾�Ʈ�鿡��
	// �� ������ ���� ��Ŷ�� ���� ���ΰ�ħ�� �ִ� �Լ��̴�.
	// ������ �� ���� ������ ������ ����.
	
	// 1. ���� ����(�̸�/������ ��)
	// 2. �ǵ� ����(���� �ǵ�/��ü �ǵ�)
	CGameRoom* room = this->m_RoomList[num];

	// �ش� �� �ȿ� �ִ� ���� ������ �����ֱ� ���� iterator�� �����Ѵ�.
	list<CClient*>::iterator iter;
	list<CClient*> list = room->GetUserList();

	// ����� �� ���� ������ ������ �����Ѵ�.
	// result�� ��� strcat���� �̾���̱� ���� �ʱⰪ�� �������� �ʱ�ȭ���ش�.
	char result[500] = "";
	int resultSize = 0;

	// ���� ������ ������ '/'�� ���´�.
	// ������ "�̸�/������ ��/�̸�/������ ��....." �̴�.
	for (iter = list.begin(); iter != list.end(); iter++)
	{
		char temp[50];
		resultSize += sprintf(temp, "%s/%d/",
			(*iter)->getID().c_str(), (*iter)->getBetMoney());

		strcat(result, temp);
	}

	// ������ �����ڸ� �����ϱ� ���� resultSize�� 1 �ٿ��ش�.
	resultSize--;

	// �� �ȿ� �ִ� ��� Ŭ���̾�Ʈ�� ������ �����Ѵ�.
	for (iter = list.begin(); iter != list.end(); iter++)
	{
		GetInstance()->sendRequest((*iter)->getSocket(), result,
			PACKET_TYPE_GAME_USER_RES, resultSize);
	}

	// ��� ������ �ʱ�ȭ�Ѵ�.
	memset(result, 0, sizeof(result));
	resultSize = 0;

	resultSize = sprintf(result, "%d/%d", room->GetMoney(), room->GetMoneySum());

	// �� �ȿ� �ִ� ��� Ŭ���̾�Ʈ�� ������ �����Ѵ�.
	for (iter = list.begin(); iter != list.end(); iter++)
	{
		GetInstance()->sendRequest((*iter)->getSocket(), result,
			PACKET_TYPE_GAME_INFO_RES, resultSize);
	}
}

void CIOCPServer::sendRequest(SOCKET socket, char* data, unsigned short type, unsigned short size)
{
	// �� �Լ������� RESPONSE TYPE�� ��Ŷ�� ���۵Ǹ�
	// Ŭ���̾�Ʈ���� REQUEST�� �����ؾ߸� �����ϴ� ������ �۽� ��Ŷ���� ����Ѵ�.
	// �������� ���� �ɵ������� �����ϴ� ��Ŷ�� public �Լ��� DirectSendRequest()���� ã�ƺ� ��.

	// ��Ŷ �۽� ����
	// ��� Ŭ����, ����ü ���� �� �����ʹ� ���� ���簡 �ȴ�.
	
	// 1. CPacket ����
	// ���� ��Ŷ�� ���� Ÿ�԰� �����͸� �����Ѵ�.
	// ���� ������ �Լ� ���� �� �ڵ� �Ҹ�ȴ�.
	CPacket sendPacket(data, type, size);

	// 2. LPPER_IO_PACKET ���� 
	// CPacket�� Encode() �Լ��� ������, ��� ������ ������ ��Ŷ�̴�.
	// char* ��Ŷ �� ��� / ��Ŷ ����� ����� ����ü�̴�.
	// ���� �Ҵ�Ǿ�����, ����ü�̱� ������ �Ҹ��ϱ� �� �ݵ�� free()�� ����� �Ѵ�.
	LPPER_IO_PACKET sendData = sendPacket.Encode();

	// 3. LPPER_IO_DATA ����
	// �ܹ���(���⼭�� �۽�)���� ���� ���� ����� ��Ŷ�̴�.
	// ���� �Ҵ�Ǿ�����, �۽��� �������Ǹ� multiSendCount�� ���� 
	// ��Ư���� 1�� �����忡�� �ڵ� �Ҹ�ȴ�.
	LPPER_IO_DATA sendIoInfo;

	// 4. WSASend ����
	// ioInfo�� sendData�� WSASend�� �̿��ϱ� ���� �ʱ�ȭ�� �ش�.
	GetInstance()->initWSASend(&sendIoInfo, sendData->data, sendData->size);

	// 5. WSASend
	// ��� ������ ������ ���� ���� ��Ŷ�� �ش��ϴ� Ŭ���̾�Ʈ�� ������.
	WSASend(socket, &(sendIoInfo->wsaBuf), 1, NULL, 0, &(sendIoInfo->overlapped), NULL);

	// 6. LPPER_IO_PACKET ����
	// 2������ ���� �Ҵ�� ��Ŷ�� �ݵ�� �����Ѵ�.
	free(sendData->data);
	free(sendData);
	
	// ��� ������ ������ �Լ��� ����ȴ�.
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
}

void CIOCPServer::closeClient(LPPER_HANDLE_DATA* handleInfo, LPPER_IO_DATA* ioInfo)
{
	int roomNumber = -1;
	int roomState = 0;
	// Ŭ���̾�Ʈ�� ���� �����ϱ� �� �Ӱ� �������� ó���� �Ѵ�.
	EnterCriticalSection(&GetInstance()->m_CS);

	CClient* client = this->m_ClientList[(*handleInfo)->hClientSocket];
	roomNumber = client->getRoom();
	// Ŭ���̾�Ʈ�� �濡 �־��� ��� ���� �� ���� ó���� �� �ش�.
	if (client->getRoom() != -1) {
		CGameRoom* room = this->m_RoomList[roomNumber];
		room->OnExit(client);
		roomState = room->GetRoomState();

		// ���� �濡 ����� ���ٸ� ���� ���Ľ�Ų��.
		if (room->GetUserNumber() == 0) {
			GetInstance()->m_RoomList.erase(roomNumber);
			delete room;
			roomNumber = -1;
			roomState = 0;
		}
	}
	// Ŭ���̾�Ʈ ������ ����Ʈ������ �ش� Ŭ���̾�Ʈ�� �� �ش�.
	this->m_ClientList.erase(client->getSocket());

	LeaveCriticalSection(&GetInstance()->m_CS);

	// ���������� Ŭ���̾�Ʈ�� �����ϸ�
	delete client;
	
	// ���� �����ִ� ��� �濡 �ִ� ����鿡�� �����Ѵ�.
	if (roomNumber != -1)
	{
		if (roomState == 0)
			this->RefreshRoomInfo(roomNumber);
		else
			this->RefreshGameRoomInfo(roomNumber);
	}

	// �׸��� �����ܿ� �����ִ� ���ϰ� �׿� ������ ���� �����ϰ� ������.
	closesocket((*handleInfo)->hClientSocket);
	printf("Client Disconnected :: %s/%d\n", inet_ntoa((*handleInfo)->clientAddress.sin_addr),
		(*handleInfo)->clientAddress.sin_port);
	delete (*handleInfo)->packetQueue;
	free(*handleInfo);
	free(*ioInfo);
}