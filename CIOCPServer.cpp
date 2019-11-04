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
	DeleteCriticalSection(&GetInstance()->m_CS);
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

	// CRITICAL_SECTION 핸들은 미리 할당
	InitializeCriticalSection(&GetInstance()->m_CS);

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
	::bind(m_hServerSocket, (SOCKADDR*)&m_ServerAddress, sizeof(m_ServerAddress));
	listen(m_hServerSocket, SOMAXCONN);

	// listen 준비가 끝나고 accept를 위해 accept thread를 하나 시작
	_beginthreadex(NULL, 0, acceptProcedure, NULL, 0, NULL);

	// 모든 서버 동작 준비가 끝나면 서버의 IP주소와 포트번호를 콘솔창에 출력하여 알림
	gethostname(hostname, 256);
	host = gethostbyname(hostname);
	for (int i = 0; i < 4; i++)
		ipAddr[i] = *(host->h_addr_list[0]+i);

	GetInstance()->m_ClientList.clear();
	GetInstance()->m_RoomList.clear();
	GetInstance()->m_nLastRoomIndex = 0;

	puts("============= IOCP 서버 초기 설정 완료!=============");
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
		// 클라이언트를 accept 받는다
		hClientSocket = accept((SOCKET)GetInstance()->m_hServerSocket, (SOCKADDR*)&clientAddress, &addrLength);

		// 클라이언트가 접속 중일 때 사용될 HANDLE_DATA 구조체를 할당한다.
		handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
		handleInfo->hClientSocket = hClientSocket;
		memcpy(&(handleInfo->clientAddress), &clientAddress, addrLength);
		handleInfo->packetQueue = new CPacketQueue();

		// 클라이언트 리스트(Map)에 지금 접속한 클라이언트를 추가해준다.
		// 공유 멤버에 대한 접근이므로 임계영역 처리해준다.
		EnterCriticalSection(&GetInstance()->m_CS);
		GetInstance()->m_ClientList[hClientSocket] = new CClient(hClientSocket);
		LeaveCriticalSection(&GetInstance()->m_CS);

		// 클라이언트의 접속을 콘솔창에 알린다.
		printf("Client Accepted :: %s/%d\n", inet_ntoa(clientAddress.sin_addr), clientAddress.sin_port);

		// 해당 소켓을 Completion Port에 추가하여 감시한다.
		CreateIoCompletionPort((HANDLE)hClientSocket, GetInstance()->m_hCP, (DWORD)handleInfo, 0);

		// 최초 WSARecv를 위해 셋팅한다.
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
		// GQCS(클라이언트에서 어떠한 작업이 완료되면 반환하는 함수)의 리턴값을 받는다.
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
				if (recv == -1) {
					CIOCPServer::GetInstance()->closeClient(&handleInfo, &ioInfo);
					continue;
				}

				// 이번 WSARecv에 쓰인 ioInfo는 더 이상 필요하지 않으니 해제한다.
				free(ioInfo);

				// 사용 가능한 패킷 개수만큼 분석 및 작업 요청을 즉시 진행한다.
				// 외부 모듈에서 작업시 외부 모듈의 스레드를 이용하며 결과는 나중에 받는다.
				// 즉시 처리 가능한 내부 작업은 곧바로 처리해 준다.
				for (int i = 0; i < recv; i++)
				{
					// 도착한 패킷을 전부 분리한다.
					// 여기 선언한 패킷은 지역 변수라서 for문 탈출 시 자동으로 소멸된다.
					CPacket packet = *(handleInfo->packetQueue->getPacket());
					char* charContent = packet.GetContent();
					unsigned short type = packet.GetPacketType();
					unsigned short size = packet.GetDataSize();

					switch (packet.GetPacketType())
					{
					// STANDARD 패킷은 기본적인 Echo전용 패킷이다.
					case PACKET_TYPE_STANDARD:
					{
						// 1. 받은 내용을 출력한다.
						string command(charContent, size);
						cout << "STANDARD 패킷 내용 :: " << command << endl;
						
						// 2. 받은 내용을 그대로 클라이언트에 전송한다.
						GetInstance()->sendRequest(socket, charContent, type, size);
					}
					break;

					// LOGIN_REQ 패킷은 클라이언트의 로그인 요청이다.
					case PACKET_TYPE_LOGIN_REQ:
					{
						// 1. 결과와 그 값을 저장할 변수를 선언한다.
						bool bSuccess = false;
						char result[7];
						int resultSize = 0;

						// 2. 클라이언트로부터 받은 아이디와 비밀번호를 분리한다.
						string info(charContent, size);
						string id(info.substr(0, info.find('/')));
						string pw(info.substr(info.find('/') + 1));

						// 3. DB 모듈에 선언된 로그인 요청 함수를 호출한다.
						// 로그인이 성공한다면 LoginRequest()의 3번째 인수로 넣은 CClient*에
						// 해당 유저의 ID, 전적, 소지금이 저장된다.
						bSuccess = CDataBaseManager::GetInstance()
							->LoginRequest(id, pw, GetInstance()->m_ClientList[socket]);

						// 4. 성공하면 bSuccess에 성공 여부가 반환된다.
						// 이에 따라 클라이언트에 통지할 문자열을 설정한다.
						if (bSuccess) {
							resultSize = 7;
							memcpy(result, "success", resultSize);
						}
						else {
							resultSize = 6;
							memcpy(result, "failed", resultSize);
						}

						// 5. 로그인 요청 결과를 클라이언트에 통지한다.
						GetInstance()->sendRequest(socket, result, PACKET_TYPE_LOGIN_RES, resultSize);
					}
					break;

					// REGISTER_REQ 패킷은 클라이언트의 회원가입 요청이다.
					case PACKET_TYPE_REGISTER_REQ:
					{
						// 1. 결과와 그 값을 저장할 변수를 선언한다.
						bool bSuccess = false;
						char result[7];
						int resultSize = 0;

						// 2. 클라이언트로부터 받은 아이디와 비밀번호를 분리한다.
						string info(charContent, size);
						string id(info.substr(0, info.find('/')));
						string pw(info.substr(info.find('/') + 1));

						// 3. DB 모듈에 선언된 회원가입 요청 함수를 호출한다.
						if (id.length() >= 4 && pw.length() >= 4)
							bSuccess = CDataBaseManager::GetInstance()->RegisterRequest(id, pw);
						
						// 4. 성공하면 bSuccess에 성공 여부가 반환된다.
						// 이에 따라 클라이언트에 통지할 문자열을 설정한다.
						if (bSuccess) {
							resultSize = 7;
							memcpy(result, "success", resultSize);
						}
						else {
							resultSize = 6;
							memcpy(result, "failed", resultSize);
						}

						// 5. 회원가입 요청 결과를 클라이언트에 통지한다.
						GetInstance()->sendRequest(socket, result, PACKET_TYPE_REGISTER_RES, resultSize);
					}
					break;

					// LOBBY_REQ 패킷은 클라이언트의 로비 정보 요청이다.
					case PACKET_TYPE_ENTER_LOBBY_REQ:
					{
						// 1. 결과와 그 값을 저장할 변수를 선언한다.
						bool bSuccess = false;
						char result[2000] = "";
						int resultSize = 0;
						CClient* client = GetInstance()->m_ClientList[socket];

						// 2. 요청 클라이언트의 아이디를 저장한다.
						string id(charContent, size);

						// 3. 해당하는 아이디를 가진 회원의 정보를 DB에 요청한다.
						// 성공 시 UserInfoRequest()의 두 번째 인수에 지정한 CClient*에 정보가 저장된다.
						bSuccess = CDataBaseManager::GetInstance()->UserInfoRequest(id, client);

						// 4. 성공하면 bSuccess에 성공 여부가 반환된다.
						// 성공하면 해당 클라이언트의 정보를 구분자로 묶어서 result에 저장한다.
						if (bSuccess) {
							resultSize = sprintf(result, "%s/%d/%d/%d/%d",
								client->getID().c_str(), client->getWin(),
								client->getLose(), client->getMoney(), client->getRoom());
						}
						// 4.5. 오류 처리 - 만약 해당하는 아이디의 유저가 없다면 result에
						// "null" 이라고 저장한다.
						else {
							strcpy(result, "null");
							resultSize = 4;
						}

						// 5. 해당 유저의 정보를 전송한다.
						GetInstance()->sendRequest(socket, result, PACKET_TYPE_USER_INFO_RES, resultSize);
						
						// 6. 방 목록을 전송하기 위해 결과 저장용 변수들을 초기화해준다.
						bSuccess = false;
						memset(result, 0, sizeof(result));
						strcpy(result, "");
						resultSize = 0;

						// 7. 방 번호, 방 이름, 입장 인원수 순으로 구분자 '/'를 이용해 result에 저장한다.
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

						// 8. 최대 길이를 1 내려서 마지막 구분자는 가지 않게 한다.
						resultSize--;

						// 8.5. 오류처리 - 방이 없어서 리스트 사이즈가 0이면 "null"을 보낸다.
						if (GetInstance()->m_RoomList.size() == 0)
						{
							strcpy(result, "null");
							resultSize = 4;
						}

						// 9. 방 리스트를 전송한다.
						GetInstance()->sendRequest(socket, result, PACKET_TYPE_ROOM_LIST_RES, resultSize);
					}
					break;
					
					// ROOM_MAKE_REQ 패킷은 클라이언트의 방 만들기 요청이다.
					case PACKET_TYPE_ROOM_MAKE_REQ:
					{
						// 1. 결과와 그 값을 저장할 변수를 선언한다.
						char result[20];
						int resultSize = 0;

						// 2. 클라이언트로부터 받은 방 제목을 저장한다.
						string title(charContent, size);

						// 3. 새로운 방을 선언한다.
						// 새로운 방 번호는 인덱싱에 의해 겹칠 일이 절대로 없다.
						CGameRoom* room = new CGameRoom();
						CClient* client = GetInstance()->m_ClientList[socket];
						int roomIndex = GetInstance()->m_nLastRoomIndex;
						
						// 4. 클라이언트를 입장시키고 방 제목을 설정하며, 이를 해당 방에도 통지한다.
						client->OnEnterRoom(roomIndex);
						room->SetName(title);
						room->OnEnter(client);

						// 5. CRITICAL_SECTION ====================================
						// CIOCPServer의 공유 멤버인 방 리스트에 지금 만든 방을 추가한다.
						EnterCriticalSection(&GetInstance()->m_CS);
						GetInstance()->m_RoomList[roomIndex] = room;
						GetInstance()->m_nLastRoomIndex++;
						LeaveCriticalSection(&GetInstance()->m_CS);
						// CRITICAL_SECTION ====================================

						// 6. 들어간 방 번호를 클라이언트에 전달한다.
						resultSize = sprintf(result, "%d", roomIndex);
						GetInstance()->sendRequest(socket, result, PACKET_TYPE_ROOM_MAKE_RES, resultSize);
					}
					break;

					// ENTER_ROOM_REQ 패킷은 클라이언트의 방 입장 요청이다.
					case PACKET_TYPE_ENTER_ROOM_REQ:
					{
						// 1. 결과와 그 값을 저장할 변수를 선언한다.
						char result[20];
						int resultSize = 0;
						int ret = 0;

						// 2. 해당 클라이언트가 입장하고 싶어하는 방을 저장한다.
						int number = atoi(charContent);

						// 3. 해당하는 방이 존재하는지 검사한다.
						CGameRoom* room = GetInstance()->m_RoomList[number];
						CClient* client = GetInstance()->m_ClientList[socket];

						// 4. 방이 존재하지 않는다면 반환값을 -1로 셋팅한다.
						if (room == NULL) ret = -1;
						// 4.5. 존재한다면
						else
						{
							// 5. 해당 방의 상태가 READY상태인지 체크한다.
							if (room->GetRoomState() == GAME_STATE_READY)
								// READY 상태여도 인원이 가득 차 있다면 -1이 반환된다.
								ret = room->OnEnter(client);
							// 5.5. READY 상태가 아니면 이미 게임중이라는 뜻이므로 반환값을 -1로.
							else
								ret = -1;
						}

						// 6. 최종적으로 ret값을 검사하여 -1이 아니면 성공이다.
						if (ret != -1)
						{
							// 성공 시 서버 상의 클라이언트를 방에 입장시켜 준다.
							client->OnEnterRoom(number);
							strcpy(result, "success");
							resultSize = 7;
						}
						// 6.5. ret값이 -1이면 실패이다.
						else {
							// 실패 시 응답 패킷만 보낸다.
							strcpy(result, "failed");
							resultSize = 6;
						}
						
						// 7. 입장 요청에 대한 결과를 클라이언트에 통지한다.
						GetInstance()->sendRequest(socket, result, PACKET_TYPE_ENTER_ROOM_RES, resultSize);
					}
					break;

					// ROOM_INFO_REQ 패킷은 방에 새로 입장한 클라이언트의 정보 요청이다.
					case PACKET_TYPE_ROOM_INFO_REQ:
					{
						// 1. 해당 클라이언트가 입장한 방 번호를 받아 저장한다.
						int number = atoi(charContent);

						// 2. 해당하는 번호의 방을 방 리스트에서 꺼내온다.
						CGameRoom* room = GetInstance()->m_RoomList[number];

						// 3. 방 이름을 배열에 저장한다.
						char roomName[100];
						int nameSize = room->GetName().size();
						strcpy(roomName, room->GetName().c_str());

						// 4. 방 이름을 클라이언트에 전달하여 준다.
						GetInstance()->sendRequest(socket, roomName, PACKET_TYPE_ROOM_INFO_RES, nameSize);

						// 5. 또한 해당 방 안에 있는 유저 정보를 보내주기 위해 iterator를 셋팅한다.
						list<CClient*>::iterator iter;
						list<CClient*> list = room->GetUserList();

						// 6. 결과와 그 값을 저장할 변수를 선언한다.
						// result의 경우 strcat으로 이어붙이기 위해 초기값을 공백으로 초기화해준다.
						char result[500] = "";
						int resultSize = 0;

						// 7. 유저 정보를 구분자 '/'로 묶는다.
						// 형식은 "이름/돈/이름/돈....." 이다.
						for (iter=list.begin(); iter != list.end(); iter++)
						{
							char temp[50];
							resultSize += sprintf(temp, "%s/%d/",
								(*iter)->getID().c_str(), (*iter)->getMoney());

							strcat(result, temp);
						}
						
						// 8. 마지막 구분자를 제거하기 위해 resultSize를 1 줄여준다.
						resultSize--;

						// 9. 방 안에 있는 모든 클라이언트에 정보를 전송한다.
						for (iter = list.begin(); iter != list.end(); iter++)
						{
							GetInstance()->sendRequest((*iter)->getSocket(), result,
								PACKET_TYPE_ROOM_USER_RES, resultSize);
						}
					}
					break;

					// EXIT_ROOM_REQ 패킷은 클라이언트가 방에서 나갈 때 서버에 통지하는 패킷이다.
					case PACKET_TYPE_EXIT_ROOM_REQ:
					{
						// 1. 해당 클라이언트가 퇴장한 방 번호를 받아 저장한다. 
						int number = atoi(charContent);
						int roomSize = 0;

						// 2. 해당하는 번호의 방을 방 리스트에서 꺼내온다.
						CGameRoom* room = GetInstance()->m_RoomList[number];
						CClient* client = GetInstance()->m_ClientList[socket];

						// 3. 해당하는 클라이언트를 퇴장시킨다.
						roomSize = room->OnExit(client);
						client->OnExitRoom();

						// 4. 만약 방에 사람이 없다면 방을 폭파시키고 case 블록을 빠져나간다.
						if (roomSize == 0) {
							GetInstance()->m_RoomList.erase(number);
							delete room;
							break;
						}

						// 5. 해당 방 안에 있는 유저 정보를 보내주기 위해 iterator를 셋팅한다.
						list<CClient*>::iterator iter;
						list<CClient*> list = room->GetUserList();

						// 6. 결과와 그 값을 저장할 변수를 선언한다.
						// result의 경우 strcat으로 이어붙이기 위해 초기값을 공백으로 초기화해준다.
						char result[500] = "";
						int resultSize = 0;

						// 7. 유저 정보를 구분자 '/'로 묶는다.
						// 형식은 "이름/돈/이름/돈....." 이다.
						for (iter = list.begin(); iter != list.end(); iter++)
						{
							char temp[50];
							resultSize += sprintf(temp, "%s/%d/",
								(*iter)->getID().c_str(), (*iter)->getMoney());

							strcat(result, temp);
						}

						// 8. 마지막 구분자를 제거하기 위해 resultSize를 1 줄여준다.
						resultSize--;

						// 9. 방 안에 있는 모든 클라이언트에 정보를 전송한다.
						for (iter = list.begin(); iter != list.end(); iter++)
						{
							GetInstance()->sendRequest((*iter)->getSocket(), result, 
								PACKET_TYPE_ROOM_USER_RES, resultSize);
						}
					}
					break;

					// ROOM_CHAT_REQ는 클라이언트가 보낸 채팅 메세지이다.
					case PACKET_TYPE_ROOM_CHAT_REQ:
					{
						// 1. 클라이언트가 현재 있는 방을 알아낸다.
						CGameRoom* room = GetInstance()->m_RoomList
							[GetInstance()->m_ClientList[socket]->getRoom()];

						// 2. 해당 방 안에 있는 유저들에게 보내주기 위해 iterator를 셋팅한다.
						list<CClient*>::iterator iter;
						list<CClient*> list = room->GetUserList();

						// 3. 방 안에 있는 모든 클라이언트에 메세지를 전송한다.
						for (iter = list.begin(); iter != list.end(); iter++)
						{
							GetInstance()->sendRequest((*iter)->getSocket(), charContent,
								PACKET_TYPE_ROOM_CHAT_RES, size);
						}
					}
						break;

					// ROOM_START_REQ는 클라이언트의 게임 시작 요청이다.
					case PACKET_TYPE_ROOM_START_REQ:
					{
						// 1. 방 번호가 패킷으로 오므로 방을 얻는다.
						int number = atoi(charContent);
						CGameRoom* room = GetInstance()->m_RoomList[number];

						// 2. 방의 상태를 게임 시작으로 바꾼다.
						room->OnStart();

						// 2. 해당 방 안에 있는 유저들에게 알려주기 위해 iterator를 셋팅한다.
						list<CClient*>::iterator iter;
						list<CClient*> list = room->GetUserList();

						// 3. 방 안에 있는 모든 클라이언트에 메세지를 전송한다.
						for (iter = list.begin(); iter != list.end(); iter++)
						{
							GetInstance()->sendRequest((*iter)->getSocket(), charContent,
								PACKET_TYPE_ROOM_START_RES, size);
						}
					}
						break;

					// ENTER_GAME_REQ는 게임에 들어가는 클라이언트가 서버에 요청하는 패킷이다.
					case PACKET_TYPE_ENTER_GAME_REQ:
					{
						// 1. 클라이언트가 현재 있는 방을 알아낸다.
						int num = atoi(charContent);
						char result[50];
						int resultSize = 0;

						// 2. 모든 공통정보를 뿌린다.
						GetInstance()->RefreshGameRoomInfo(num);

						// 3. 카드 정보를 뿌린다.
						// 카드 정보는 game프래그먼트 최초 진입시에 한 번만 주므로..
						CClient* client = GetInstance()->m_ClientList[socket];
						CCard* one = client->getCardOne();
						CCard* two = client->getCardTwo();

						// 4. 보낼 때는 "숫자/광여부/숫자/광여부" 로 보낸다.
						resultSize = sprintf(result, "%d/%d/%d/%d", one->GetNumber(), one->GetSpecial(),
							two->GetNumber(), two->GetSpecial());

						// 5. 카드 정보를 요청자에게 뿌린다.
						GetInstance()->sendRequest(socket, result, PACKET_TYPE_GAME_CARD_RES, resultSize);
					}
						break;
					
					// GAME_BETTING_REQ는 클라이언트가 시도한 베팅 요청이다.
					case PACKET_TYPE_GAME_BETTING_REQ:
					{
						// 1. 클라이언트가 보낸 베팅 종류를 저장한다.
						int bet = atoi(charContent);

						// 2. 클라이언트와 클라이언트가 현재 있는 방을 알아낸다.
						CClient* client = GetInstance()->m_ClientList[socket];
						CGameRoom* room = GetInstance()->m_RoomList
							[client->getRoom()];

						// 3. 베팅 종류에 따라 클라이언트와 방의 상태를 조정한다.
						switch (bet)
						{
							// 죽었으면 죽었다고 설정해준다.
						case GAME_BETTING_DIE:
							client->setDie(true);
							break;
							// 콜이면 기준판돈만큼 클라이언트의 베팅머니를 올린다.
						case GAME_BETTING_CALL:
							client->setBetMoney(room->GetMoney());
							room->AddMoneySum(client->getBetMoney());
							break;
							// 더블이면 기준판돈의 2배로 베팅머니를 올린다.
						case GAME_BETTING_DOUBLE:
						{
							room->SetMoney(room->GetMoney() * 2);
							client->setBetMoney(room->GetMoney());
							room->AddMoneySum(client->getBetMoney());

							// 누군가 더블을 걸었으니 더 베팅하라는 사실을 모두에게 알린다.
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

						// 4. 게임이 끝나는 조건 :: 1명 빼고 다 죽거나
						// 살아있는 모두가 기준판돈만큼의 돈을 베팅했는지 확인한다.
						list<CClient*>::iterator iter;
						list<CClient*> list = room->GetUserList();
						int alive = 0;
						int called = 0;

						for (iter = list.begin(); iter != list.end(); iter++)
						{
							// 살아있는 놈 카운트 +
							if (!(*iter)->isDead()) 
								alive++;

							// 기준금액 이상 베팅한 사람 +
							if ((*iter)->getBetMoney() >= room->GetMoney()) 
								called++;
						}

						char result[5000];
						memset(result, 0, sizeof(result));
						int resultSize = 0;

						// 살아있는 사람이 한 명이다?
						if (alive == 1) {
							// 살아있는 사람(승자)에게 돈을 주고 게임 종료 패킷을 날린다.
							// 게임 종료 패킷에는 각자의 이름/족보코드/다이 여부가 가게 된다.
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

							// 마지막 구분자를 제외하기 위해 인덱스를 1 뺀다.
							resultSize--;

							for (iter = list.begin(); iter != list.end(); iter++)
							{
								// 클라이언트 안에 게임용 멤버들을 초기화 및 해제한다.
								(*iter)->OnEndGame();

								// 모두에게 게임이 끝났음을 알린다.
								GetInstance()->sendRequest((*iter)->getSocket(), result,
									PACKET_TYPE_GAME_SET_RES, resultSize);
							}
							// 방에 카드 리스트도 날리고
							room->ReleaseCardSet();
							room->SetRoomState(GAME_STATE_READY);
							// case 블럭을 빠져나간다.
							break;
						}

						int maxJokbo = 0;
						bool equal = false;

						// 살아있는 모든 사람이 기준금액 이상 콜베팅을 했다?
						if (called == alive) {
							// 족보를 비교하고 승패를 결정한 후
							// 승자에게 돈을 주고 게임 종료 패킷을 날린다.
							// 무승부면 그냥 끝내면 된다.
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

							// 마지막 구분자를 제외하기 위해 인덱스를 1 뺀다.
							resultSize--;

							i = 0;
							// 무승부면 아무것도 안 해줘도 되고
							// 무승부가 아니면 DB처리를 한다.
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

							// 마지막으로
							for (iter = list.begin(); iter != list.end(); iter++)
							{
								// 클라이언트 안에 게임용 멤버들을 초기화 및 해제한다.
								(*iter)->OnEndGame();

								// 모두에게 게임이 끝났음을 알린다.
								GetInstance()->sendRequest((*iter)->getSocket(), result,
									PACKET_TYPE_GAME_SET_RES, resultSize);
							}

							// 방에 카드 리스트도 날리고
							room->ReleaseCardSet();
							room->SetRoomState(GAME_STATE_READY);
							// 그리고 case 블럭을 빠져나간다.
							break;
						}

						// 5. 여기까지 왔으면 게임 끝난 것이 아니므로, 게임방 상태를 리프레쉬한다.
						GetInstance()->RefreshGameRoomInfo(client->getRoom());
					}
						break;

					default:
						break;
					}
				}

				// 응답과 요청이 모두 끝났다면 다음 패킷 수신을 위해 WSARecv를 실행한다.
				LPPER_IO_DATA newIoInfo;
				GetInstance()->initWSARecv(&newIoInfo);
				WSARecv(socket, &(newIoInfo->wsaBuf), 1, NULL, &flags, &(newIoInfo->overlapped), NULL);
			}
			// 쓰기 완료 시 
			else free(ioInfo);
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

void CIOCPServer::RefreshRoomInfo(int num)
{
	// 이 함수는 서버에서 다이렉트로 특정 방의 클라이언트들에게
	// 방 정보를 담은 패킷을 보내 새로고침해 주는 함수이다.
	CGameRoom* room = this->m_RoomList[num];

	// 해당 방 안에 있는 유저 정보를 보내주기 위해 iterator를 셋팅한다.
	list<CClient*>::iterator iter;
	list<CClient*> list = room->GetUserList();

	// 결과와 그 값을 저장할 변수를 선언한다.
	// result의 경우 strcat으로 이어붙이기 위해 초기값을 공백으로 초기화해준다.
	char result[500] = "";
	int resultSize = 0;
	
	// 유저 정보를 구분자 '/'로 묶는다.
	// 형식은 "이름/돈/이름/돈....." 이다.
	for (iter = list.begin(); iter != list.end(); iter++)
	{
		char temp[50];
		resultSize += sprintf(temp, "%s/%d/",
			(*iter)->getID().c_str(), (*iter)->getBetMoney());

		strcat(result, temp);
	}

	// 마지막 구분자를 제거하기 위해 resultSize를 1 줄여준다.
	resultSize--;

	// 방 안에 있는 모든 클라이언트에 정보를 전송한다.
	for (iter = list.begin(); iter != list.end(); iter++)
	{
		GetInstance()->sendRequest((*iter)->getSocket(), result,
			PACKET_TYPE_ROOM_USER_RES, resultSize);
	}
}

void CIOCPServer::RefreshGameRoomInfo(int num)
{
	// 이 함수는 서버에서 다이렉트로 게임 중인 특정 방의 클라이언트들에게
	// 방 정보를 담은 패킷을 보내 새로고침해 주는 함수이다.
	// 보내야 할 공통 정보는 다음과 같다.
	
	// 1. 유저 정보(이름/베팅한 돈)
	// 2. 판돈 정보(기준 판돈/전체 판돈)
	CGameRoom* room = this->m_RoomList[num];

	// 해당 방 안에 있는 유저 정보를 보내주기 위해 iterator를 셋팅한다.
	list<CClient*>::iterator iter;
	list<CClient*> list = room->GetUserList();

	// 결과와 그 값을 저장할 변수를 선언한다.
	// result의 경우 strcat으로 이어붙이기 위해 초기값을 공백으로 초기화해준다.
	char result[500] = "";
	int resultSize = 0;

	// 유저 정보를 구분자 '/'로 묶는다.
	// 형식은 "이름/베팅한 돈/이름/베팅한 돈....." 이다.
	for (iter = list.begin(); iter != list.end(); iter++)
	{
		char temp[50];
		resultSize += sprintf(temp, "%s/%d/",
			(*iter)->getID().c_str(), (*iter)->getBetMoney());

		strcat(result, temp);
	}

	// 마지막 구분자를 제거하기 위해 resultSize를 1 줄여준다.
	resultSize--;

	// 방 안에 있는 모든 클라이언트에 정보를 전송한다.
	for (iter = list.begin(); iter != list.end(); iter++)
	{
		GetInstance()->sendRequest((*iter)->getSocket(), result,
			PACKET_TYPE_GAME_USER_RES, resultSize);
	}

	// 결과 변수를 초기화한다.
	memset(result, 0, sizeof(result));
	resultSize = 0;

	resultSize = sprintf(result, "%d/%d", room->GetMoney(), room->GetMoneySum());

	// 방 안에 있는 모든 클라이언트에 정보를 전송한다.
	for (iter = list.begin(); iter != list.end(); iter++)
	{
		GetInstance()->sendRequest((*iter)->getSocket(), result,
			PACKET_TYPE_GAME_INFO_RES, resultSize);
	}
}

void CIOCPServer::sendRequest(SOCKET socket, char* data, unsigned short type, unsigned short size)
{
	// 이 함수에서는 RESPONSE TYPE의 패킷만 전송되며
	// 클라이언트에서 REQUEST가 도착해야만 응답하는 수동적 송신 패킷만을 취급한다.
	// 서버에서 직접 능동적으로 전송하는 패킷은 public 함수인 DirectSendRequest()에서 찾아볼 것.

	// 패킷 송신 순서
	// 모든 클래스, 구조체 생성 시 데이터는 깊은 복사가 된다.
	
	// 1. CPacket 생성
	// 보낼 패킷에 대한 타입과 데이터를 저장한다.
	// 지역 변수라서 함수 종료 시 자동 소멸된다.
	CPacket sendPacket(data, type, size);

	// 2. LPPER_IO_PACKET 생성 
	// CPacket의 Encode() 함수로 생성된, 통신 가능한 완전한 패킷이다.
	// char* 패킷 한 덩어리 / 패킷 사이즈가 저장된 구조체이다.
	// 동적 할당되었으며, 구조체이기 때문에 소멸하기 전 반드시 free()를 해줘야 한다.
	LPPER_IO_PACKET sendData = sendPacket.Encode();

	// 3. LPPER_IO_DATA 생성
	// 단방향(여기서는 송신)으로 가는 단위 입출력 패킷이다.
	// 동적 할당되었으며, 송신이 마무리되면 multiSendCount에 따라 
	// 불특정한 1개 스레드에서 자동 소멸된다.
	LPPER_IO_DATA sendIoInfo;

	// 4. WSASend 설정
	// ioInfo와 sendData를 WSASend에 이용하기 위해 초기화해 준다.
	GetInstance()->initWSASend(&sendIoInfo, sendData->data, sendData->size);

	// 5. WSASend
	// 모든 설정이 끝나면 드디어 응답 패킷을 해당하는 클라이언트로 보낸다.
	WSASend(socket, &(sendIoInfo->wsaBuf), 1, NULL, 0, &(sendIoInfo->overlapped), NULL);

	// 6. LPPER_IO_PACKET 해제
	// 2번에서 동적 할당된 패킷을 반드시 해제한다.
	free(sendData->data);
	free(sendData);
	
	// 모든 과정이 끝나면 함수가 종료된다.
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
	// 클라이언트가 접속 종료하기 전 임계 영역에서 처리를 한다.
	EnterCriticalSection(&GetInstance()->m_CS);

	CClient* client = this->m_ClientList[(*handleInfo)->hClientSocket];
	roomNumber = client->getRoom();
	// 클라이언트가 방에 있었을 경우 퇴장 및 종료 처리를 해 준다.
	if (client->getRoom() != -1) {
		CGameRoom* room = this->m_RoomList[roomNumber];
		room->OnExit(client);
		roomState = room->GetRoomState();

		// 만약 방에 사람이 없다면 방을 폭파시킨다.
		if (room->GetUserNumber() == 0) {
			GetInstance()->m_RoomList.erase(roomNumber);
			delete room;
			roomNumber = -1;
			roomState = 0;
		}
	}
	// 클라이언트 접속자 리스트에서도 해당 클라이언트를 빼 준다.
	this->m_ClientList.erase(client->getSocket());

	LeaveCriticalSection(&GetInstance()->m_CS);

	// 마지막으로 클라이언트를 해제하며
	delete client;
	
	// 방이 남아있는 경우 방에 있는 사람들에게 통지한다.
	if (roomNumber != -1)
	{
		if (roomState == 0)
			this->RefreshRoomInfo(roomNumber);
		else
			this->RefreshGameRoomInfo(roomNumber);
	}

	// 그리고 서버단에 남아있는 소켓과 그외 찌꺼기들을 마저 정리하고 끝낸다.
	closesocket((*handleInfo)->hClientSocket);
	printf("Client Disconnected :: %s/%d\n", inet_ntoa((*handleInfo)->clientAddress.sin_addr),
		(*handleInfo)->clientAddress.sin_port);
	delete (*handleInfo)->packetQueue;
	free(*handleInfo);
	free(*ioInfo);
}