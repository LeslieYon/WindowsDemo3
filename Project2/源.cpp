#include <Windows.h>
#include <cstdio>
#include"tools.h"
#include "resource.h"

TCHAR OutputBuffer1[10]{}, OutputBuffer2[10]{}, OutputBuffer3[10]{};
TCHAR* OutputBuffer[3]{ OutputBuffer1 ,OutputBuffer2 ,OutputBuffer3 };
TCHAR InputBuffer1[10]{}, InputBuffer2[10]{};
TCHAR* InputBuffer[2]{ InputBuffer1 ,InputBuffer2 };

HWND hINPUT[2]{};
HWND hOUTPUT[3]{};

HANDLE isInputBufferEmpyt;
HANDLE isInputBufferFull;
HANDLE hEater[3];

CRITICAL_SECTION InputBuffer_CS;

DWORD WINAPI putInBuffer(_In_ LPVOID lpParameter) {
	TCHAR buffer[10]{};
	GetWindowText((HWND)lpParameter, buffer, 10);
	for (TCHAR x : buffer)
	{
		WaitForSingleObject(isInputBufferEmpyt, INFINITE); //等待缓冲区为空
		EnterCriticalSection(&InputBuffer_CS);
		GetWindowText(hINPUT[0], InputBuffer1, 10);
		GetWindowText(hINPUT[1], InputBuffer2, 10);
		DbgPrintf("Thread %d enter SC...", GetCurrentThreadId());
		if (!lstrlen(InputBuffer1)) {
			memset(InputBuffer1, x, 1);
			SetWindowText(hINPUT[0], InputBuffer1);
			Sleep(200);
			LeaveCriticalSection(&InputBuffer_CS);
			DbgPrintf("Thread %d leave SC(%c)...", GetCurrentThreadId(),x);
			ReleaseSemaphore(isInputBufferFull, 1, NULL);
		} else {
			memset(InputBuffer2, x, 1);
			SetWindowText(hINPUT[1], InputBuffer2);
			Sleep(200);
			LeaveCriticalSection(&InputBuffer_CS);
			DbgPrintf("Thread %d leave SC(%c)...", GetCurrentThreadId(),x);
			ReleaseSemaphore(isInputBufferFull, 1, NULL);
		}
	}
	return true;
}

DWORD WINAPI Eater(_In_ LPVOID lpParameter) {
	while (WaitForSingleObject(isInputBufferFull, 10000) != WAIT_TIMEOUT) //等待缓冲区有资源
	{
		EnterCriticalSection(&InputBuffer_CS);
		GetWindowText(hINPUT[0], InputBuffer1, 10);
		GetWindowText(hINPUT[1], InputBuffer2, 10);
		DbgPrintf("(%d)EAT Thread %d enter SC...", (DWORD)lpParameter+1,GetCurrentThreadId());
		if (lstrlen(InputBuffer1)) {
			lstrcat(OutputBuffer[(DWORD)lpParameter], InputBuffer1);
			SetWindowText(hOUTPUT[(DWORD)lpParameter], OutputBuffer[(DWORD)lpParameter]);
			SetWindowText(hINPUT[0], TEXT(""));
			LeaveCriticalSection(&InputBuffer_CS);
			DbgPrintf("(%d)EAT Thread %d leave SC(%c)...", (DWORD)lpParameter+1, GetCurrentThreadId(), *InputBuffer1);
			ReleaseSemaphore(isInputBufferEmpyt, 1, NULL);
		} else {
			lstrcat(OutputBuffer[(DWORD)lpParameter], InputBuffer2);
			SetWindowText(hOUTPUT[(DWORD)lpParameter], OutputBuffer[(DWORD)lpParameter]);
			SetWindowText(hINPUT[1], TEXT(""));
			LeaveCriticalSection(&InputBuffer_CS);
			DbgPrintf("(%d)EAT Thread %d leave SC(%c)...", (DWORD)lpParameter+1, GetCurrentThreadId(), *InputBuffer2);
			ReleaseSemaphore(isInputBufferEmpyt, 1, NULL);
		}
		Sleep(20);
	}
	return true;
}

DWORD WINAPI MainThread(_In_ LPVOID lpParameter) {

	InitializeCriticalSection(&InputBuffer_CS);

	isInputBufferEmpyt = CreateSemaphore(NULL, 2, 2, NULL); //有空闲
	isInputBufferFull = CreateSemaphore(NULL, 0, 2, NULL); //有资源

	HANDLE hThread = CreateThread(NULL, 0, putInBuffer, lpParameter, 0, NULL);

	hEater[0] = CreateThread(NULL, 0, Eater, (LPVOID)0, 0, NULL);
	hEater[1] = CreateThread(NULL, 0, Eater, (LPVOID)1, 0, NULL);
	hEater[2] = CreateThread(NULL, 0, Eater, (LPVOID)2, 0, NULL);

	if (hThread) {
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
	}
	WaitForMultipleObjects(3, hEater, true, INFINITE);
	for (int i = 0; i < 3; i++) {
		if (hEater[i]) CloseHandle(hEater[i]);
	}
	DeleteCriticalSection(&InputBuffer_CS);
	return true;
}

INT_PTR Dlgproc(
	HWND hwndDlg,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
) {
	HWND hEDITInput = GetDlgItem(hwndDlg, IDC_INPUT);
	hINPUT[0] = GetDlgItem(hwndDlg, IDC_BUFF1);
	hINPUT[1] = GetDlgItem(hwndDlg, IDC_BUFF2);
	hOUTPUT[0] = GetDlgItem(hwndDlg, IDC_OUTPUT1);
	hOUTPUT[1] = GetDlgItem(hwndDlg, IDC_OUTPUT2);
	hOUTPUT[2] = GetDlgItem(hwndDlg, IDC_OUTPUT3);
	switch (uMsg)
	{
	case WM_CLOSE:
	{
		DestroyWindow(hwndDlg);
		return true;
	}
	case WM_INITDIALOG:
	{
		SetWindowText(hEDITInput, TEXT("ABCDEFGHI"));
		return true;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
			case IDC_BUTTON_START:
			{
				HANDLE hThread = CreateThread(NULL, 0, MainThread, hEDITInput, 0, NULL);
				return true;
			}
			default:
				break;
		}
	}
	default:
		break;
	}
	return false;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPreInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
	DialogBox(hInstance, (LPCWSTR)IDD_DIALOG_MAIN, nullptr, Dlgproc);
}