#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <windows.h>

#define REQUEST_EVENT_NAME L"Global\\REQ"
#define RESPONSE_EVENT_NAME L"Global\\RESP"
#define IPC_REQ_MEMORY_NAME	L"TQER"
#define IPC_RESP_MEMORY_NAME L"PSER"

void Client()
{
	static int reqCount = 0;

	HANDLE hResponseEvent = CreateEvent(NULL, NULL, FALSE, RESPONSE_EVENT_NAME);
	if (hResponseEvent == nullptr)
	{
		wprintf(L"Client: CreateEvent() error. (%d)\n", GetLastError());
		return;
	}

	HANDLE hRequestEvent = OpenEvent(READ_CONTROL | EVENT_MODIFY_STATE, FALSE, REQUEST_EVENT_NAME);
	if (hRequestEvent == nullptr)
	{
		wprintf(L"Client: OpenEvent() error. (%d)\n", GetLastError());
		return;
	}

	HANDLE hReqFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int), IPC_REQ_MEMORY_NAME);
	if (hReqFileMap == nullptr)
	{
		wprintf(L"Client: CreateFileMapping() <1> error. (%d)\n", GetLastError());
		return;
	}

	int* pMemReq = (int*)MapViewOfFile(hReqFileMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
	if (pMemReq == nullptr)
	{
		wprintf(L"Client: MapViewOfFile() error. <1> (%d)\n", GetLastError());
		return;
	}

	HANDLE hRespFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int), IPC_RESP_MEMORY_NAME);
	if (hRespFileMap == nullptr)
	{
		wprintf(L"Client: CreateFileMapping() <2> error. (%d)\n", GetLastError());
		return;
	}

	int* pMemResp = (int*)MapViewOfFile(hRespFileMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
	if (pMemResp == nullptr)
	{
		wprintf(L"Client: MapViewOfFile() error. <2> (%d)\n", GetLastError());
		return;
	}

	int iThreadId = GetCurrentThreadId();
	memcpy_s(pMemReq, sizeof(int), &iThreadId, sizeof(int));
	if (!SetEvent(hRequestEvent))
	{
		wprintf(L"Client: SetEvent() error. (%d)\n", GetLastError());
		return;
	}

	while (true)
	{
		switch (DWORD dwWait = WaitForSingleObject(hResponseEvent, INFINITE))
		{
			case WAIT_FAILED:
			{
				wprintf(L"Client: WAIT_FAILED. (%d)\n", GetLastError());
				break;
			}

			case WAIT_ABANDONED:
			{
				wprintf(L"Client: WAIT_ABANDONED. (%d)\n", GetLastError());
				break;
			}

			case WAIT_OBJECT_0:
			{
				int iData;
				memcpy_s(&iData, sizeof(int), pMemResp, sizeof(int));
				wprintf(L"Client: Resp event raised. (ServerData : 0x%08X)\n", iData);
				return;
			}
		}
	}
}


int main()
{
	Sleep(500);
	while (true)
	{
		std::vector<std::thread> threads;
		for (int i = 0; i < 100; i++)
		{
			threads.push_back(std::thread(Client));
			Sleep(10);
		}
		for (std::thread& t : threads)
			t.join();
		system("pause");
	}

	return 0;
}