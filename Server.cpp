#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <windows.h>

#define REQUEST_EVENT_NAME L"Global\\REQ"
#define RESPONSE_EVENT_NAME L"Global\\RESP"
#define IPC_REQ_MEMORY_NAME	L"TQER"
#define IPC_RESP_MEMORY_NAME L"PSER"

void Server()
{
	static int reqCount = 0;

	HANDLE hRequestEvent = CreateEvent(NULL, FALSE, FALSE, REQUEST_EVENT_NAME);
	if (hRequestEvent == nullptr)
	{
		wprintf(L"Server: CreateEvent() error. (%d)\n", GetLastError());
		return;
	}

	HANDLE hReqFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int), IPC_REQ_MEMORY_NAME);
	if (hReqFileMap == nullptr)
	{
		wprintf(L"Server: CreateFileMapping() <1> error. (%d)\n", GetLastError());
		return;
	}

	int* pMemReq = (int*)MapViewOfFile(hReqFileMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
	if (pMemReq == nullptr)
	{
		wprintf(L"Server: MapViewOfFile() error. <1> (%d)\n", GetLastError());
		return;
	}

	HANDLE hRespFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int), IPC_RESP_MEMORY_NAME);
	if (hRespFileMap == nullptr)
	{
		wprintf(L"Server: CreateFileMapping() <2> error. (%d)\n", GetLastError());
		return;
	}

	int* pMemResp = (int*)MapViewOfFile(hRespFileMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
	if (pMemResp == nullptr)
	{
		wprintf(L"Server: MapViewOfFile() error. <2> (%d)\n", GetLastError());
		return;
	}

	while (true)
	{
		switch (DWORD dwWait = WaitForSingleObject(hRequestEvent, INFINITE))
		{
			case WAIT_FAILED:
			{
				wprintf(L"Server: WAIT_FAILED. (%d)\n", GetLastError());
				break;
			}

			case WAIT_ABANDONED:
			{
				wprintf(L"Server: WAIT_ABANDONED. (%d)\n", GetLastError());
				break;
			}

			case WAIT_OBJECT_0:
			{
				int iData;
				int innerReqCount = ++reqCount;
				memcpy_s(&iData, sizeof(int), pMemReq, sizeof(int));
				wprintf(L"Server: Req event raised. (ClientData: 0x%08X)\n", iData);
				std::thread([&pMemResp, innerReqCount]()
				{
					static std::mutex g_mt;
					std::lock_guard<std::mutex> lock(g_mt);
					memcpy_s(pMemResp, sizeof(int), &innerReqCount, sizeof(int));
					
					HANDLE hRespEvent = OpenEvent(READ_CONTROL | EVENT_MODIFY_STATE, FALSE, RESPONSE_EVENT_NAME);
					if (hRespEvent == nullptr)
					{
						wprintf(L"Response: OpenEvent() error. (%d)\n", GetLastError());
						return;
					}

					if (!SetEvent(hRespEvent))
					{
						wprintf(L"Response: SetEvent() error. (%d)\n", GetLastError());
						return;
					}
					
					Sleep(200);
					CloseHandle(hRespEvent);

				}).detach();
				break;
			}

			default:
			{
				wprintf(L"Server: Unknown Value: %d\n", dwWait);
				break;
			}
		}
	}
}

int main()
{
	std::thread serverThread(Server); Sleep(1000);
	serverThread.join();
	return 0;
}