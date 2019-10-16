#include<iostream>
#include<string>
#include<vector>

#include"CIOCPServer.h"
using namespace std;

// 싱글톤 객체를 main 함수에서 불러다 쓸 수 있도록 하는 조치
CIOCPServer* CIOCPServer::m_pInstance = nullptr;

int main(int argc, char* argv[])
{
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
}