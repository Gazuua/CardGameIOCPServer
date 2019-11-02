#include<iostream>
#include<string>
#include<queue>

#include"CIOCPServer.h"
#include"CDataBaseManager.h"
#include"CPacket.h"
#include"CUtil.h"
using namespace std;

#pragma warning(disable:4996)

// 싱글톤 인스턴스를 main 함수에서 불러다 쓸 수 있도록 하는 조치
CIOCPServer* CIOCPServer::m_pInstance = nullptr;
CDataBaseManager* CDataBaseManager::m_pInstance = nullptr;

int main(int argc, char* argv[])
{
	if (!CDataBaseManager::GetInstance()->DBConnect())
		return -1;

	if (!CIOCPServer::GetInstance()->Init(55248))
		return -1;
	
	while (1)
	{
		char c;
		scanf("%c", &c);

		// 키보드 입력값이 z이면 프로그램 종료
		if (c == 'z')
		{
			break;
		}
	}
	
	CIOCPServer::GetInstance()->Release();
	CDataBaseManager::GetInstance()->Release();
}