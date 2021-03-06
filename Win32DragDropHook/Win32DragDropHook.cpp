#include "stdafx.h"

#include "Shellapi.h"
#include <vector>
#include <string>



extern HMODULE g_HMODULE;

extern "C"
{

	typedef void(__stdcall * DragDropCallback)(int);

	DragDropCallback g_Callback = nullptr;

	std::vector<std::wstring> filenames;


	/*
	 * This handler gets registered via SetWindowsHookEx(WH_GETMESSAGE, MsgProcHandler, ...)
	 */
	__declspec(dllexport) LRESULT CALLBACK MsgProcHandler(int message, WPARAM wParam, LPARAM lParam)
	{
		MSG * pData = (MSG *)lParam;

		switch (pData->message)
		{
			case WM_COPYDATA:
				//OutputDebugString(L"WM_COPYDATA\n");
				break;


			case WM_DROPFILES:
				//OutputDebugString(L"FILE DROPPED\n");
				HDROP hDrop = (HDROP)pData->wParam;
				
				POINT drop_point;
				if (DragQueryPoint(hDrop, &drop_point) == FALSE) {
					drop_point.x = -1;
					drop_point.y = -1;
				}

				int nFiles = DragQueryFile(hDrop, 0xFFFFFFFF, 0, 0);
				filenames.clear();
				for (int i = 0; i < nFiles; ++i) {
					int nCharacters = DragQueryFile(hDrop, i, nullptr, 0);
					if (nCharacters > 0) {
						TCHAR * buffer = new TCHAR[nCharacters+1];
						DragQueryFile(hDrop, i, buffer, nCharacters+1);
						OutputDebugString(buffer);
						std::wstring filename(buffer);
						filenames.push_back(filename);
					}
				}

				DragFinish(hDrop);

				if (g_Callback != nullptr && nFiles > 0)
					g_Callback(nFiles);
				break;
		}

		return CallNextHookEx(0, message, wParam, lParam);
	}



	__declspec(dllexport) int CALLBACK GetLastDropFilenameLength(int i)
	{
		if (i < filenames.size())
			return (int)filenames[i].length();
		else
			return -1;
	}

	__declspec(dllexport) int CALLBACK GetLastDropFilename(int i, void * pBuffer, int buflen)
	{
		if (i >= filenames.size())
			return -1;
		int required_bytes = (int)(filenames[i].size()+1) * sizeof(wchar_t);
		if (required_bytes > buflen)
			return -2;

		memcpy_s(pBuffer, buflen, filenames[i].c_str(), required_bytes);
		((unsigned char *)pBuffer)[required_bytes] = 0; // null-terminate
		((unsigned char *)pBuffer)[required_bytes+1] = 0; // null-terminate
		return required_bytes;
	}



	__declspec(dllexport) void CALLBACK SetActiveCallback(DragDropCallback callback_func)
	{
		g_Callback = callback_func;
	}


	HWND installed_hwnd = 0;
	HHOOK installed_hook = 0;


	/// install our hook in the hWnd
	__declspec(dllexport) int CALLBACK InstallDragDropHook(HWND hWnd)
	{
		if (installed_hook != 0)
			return -1;

		// [RMS] enable dragdrop
		LONG_PTR style = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
		style |= WS_EX_ACCEPTFILES;
		SetWindowLongPtr(hWnd, GWL_EXSTYLE, style);
		DragAcceptFiles(hWnd, true);
		installed_hwnd = hWnd;

		DWORD thread_id = GetWindowThreadProcessId(hWnd, NULL);  // or 0 ?
		installed_hook = SetWindowsHookEx(WH_GETMESSAGE, MsgProcHandler, g_HMODULE, thread_id);
		return 1;
	}


	/// remove our hook
	__declspec(dllexport) void CALLBACK RemoveDragDropHook()
	{
		if (installed_hook > 0) {
			UnhookWindowsHookEx(installed_hook);
			installed_hook = 0;

			DragAcceptFiles(installed_hwnd, false);
			installed_hwnd = 0;
		}
	}

};