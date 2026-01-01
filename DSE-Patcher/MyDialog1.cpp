
// DSE-Patcher - Patch DSE (Driver Signature Enforcement)
// Copyright (C) 2022 Kai Schtrom
//
// This file is part of DSE-Patcher.
//
// DSE-Patcher is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DSE-Patcher is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with DSE-Patcher. If not, see <http://www.gnu.org/licenses/>.

// disable lint warnings for complete source code file
//lint -e459  Warning  459: Function 'MyDlg1DlgProc' whose address was taken has an unprotected access to variable 'g'
//lint -e744  Warning  744: switch statement has no default
//lint -e747  Warning  747: Significant prototype coercion -> This is only used here, because SendMessage needs a lot of type conversions otherwise.
//lint -e750  Warning  750: local macro '_CRT_SECURE_NO_DEPRECATE' not referenced
//lint -e818  Warning  818: Pointer parameter could be declared as pointing to const --- Eff. C++ 3rd Ed. item 3
//lint -e952  Warning  952: Parameter could be declared const --- Eff. C++ 3rd Ed. item 3
//lint -e953  Warning  953: Variable could be declared as const --- Eff. C++ 3rd Ed. item 3
//lint -e1924 Warning 1924: C-style cast -- More Effective C++ #2

// deprecate unsafe function warnings e.g. strcpy, sprintf
#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>
// CreateStatusWindow
#include <commctrl.h>
// printf, freopen_s
#include <stdio.h>
// _open_osfhandle, _fdopen, _O_TEXT
#include <io.h>
#include <fcntl.h>
#include "resource.h"
#include "MyFunctions.h"

// CreateStatusWindow
#pragma comment(lib,"comctl32.lib")

extern GLOBALS g;

// Store the font handle to delete it on exit
static HFONT g_hFont = NULL;

// Global console output handle for CLI mode (used by MyExecuteCLI)
HANDLE g_hConsoleOut = NULL;

//------------------------------------------------------------------------------
// create tooltip window and associate the tooltip with the control
//------------------------------------------------------------------------------
int MyDlg1CreateTooltip(HMODULE hInstance,HWND hDialog,HWND hControl)
{
	// create tooltip window
	HWND hwndTip = CreateWindowEx(NULL,TOOLTIPS_CLASS,NULL,WS_POPUP | TTS_ALWAYSTIP,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,hDialog,NULL,hInstance,NULL);
	if(hwndTip == NULL)
	{
		return 1;
	}

	// associate the tooltip with the control
	TOOLINFO toolInfo;
	memset(&toolInfo,0,sizeof(TOOLINFO));
	toolInfo.cbSize = sizeof(TOOLINFO);
	toolInfo.hwnd = hDialog;
	toolInfo.uFlags = TTF_CENTERTIP | TTF_IDISHWND | TTF_SUBCLASS;
	toolInfo.uId = (UINT_PTR)hControl;
	// if lpszText is set to LPSTR_TEXTCALLBACK, the control sends the TTN_GETDISPINFO notification code to the owner window to retrieve the text
	toolInfo.lpszText = LPSTR_TEXTCALLBACK;
	SendMessage(hwndTip,TTM_ADDTOOL,0,(LPARAM)&toolInfo);

	// set the visible duration of the tooltip before it closes to 30 seconds
	SendMessage(hwndTip,TTM_SETDELAYTIME,TTDT_AUTOPOP,30000);

	return 0;
}


//------------------------------------------------------------------------------
// tooltip set multiline text
//------------------------------------------------------------------------------
int MyDlg1TooltipSetMultilineText(LPARAM lParam)
{
	LPNMTTDISPINFO pInfo = (LPNMTTDISPINFO)lParam;

	// enable multiline tooltip by setting the display rectangle to 500 pixels
	// we never use the full width of 500 pixels, because we use newlines for long tooltip text
	SendMessage(pInfo->hdr.hwndFrom,TTM_SETMAXTIPWIDTH,0,500);

	// set tooltip text
	if((HWND)pInfo->hdr.idFrom == g.Dlg1.hButton1)
	{
		pInfo->lpszText = "Disable \"Driver Signature Enforcement\":\nSets the variable to \"DSE Disable Value\".";
	}
	else if((HWND)pInfo->hdr.idFrom == g.Dlg1.hButton2)
	{
		pInfo->lpszText = "Enable \"Driver Signature Enforcement\":\nSets the variable to \"DSE Enable Value\".";
	}
	else if((HWND)pInfo->hdr.idFrom == g.Dlg1.hButton3)
	{
		pInfo->lpszText = "Restore \"Driver Signature Enforcement\":\nSets the variable to \"DSE Original Value\".\n\n"
						  "Attention:\nThe \"DSE Original Value\" is retrieved\nonly one time on startup of !";
	}
	else if((HWND)pInfo->hdr.idFrom == g.Dlg1.hCombo1)
	{
		// check vulnerable driver combo box selection
		int sel = (int)SendMessage(g.Dlg1.hCombo1,CB_GETCURSEL,0,0);
		if(sel != CB_ERR)
		{
			// show corresponding tool tip text
			// the tool tip text is initialized in the function MyInitVulnerableDrivers
			//lint -e{1773} Warning 1773: Attempt to cast away const (or volatile)
			pInfo->lpszText = (LPSTR)g.vd[sel].szToolTipText;
		}
	}

	return 0;
}


//------------------------------------------------------------------------------
// dialog on init
//------------------------------------------------------------------------------
int MyDlg1OnInitDialog(HWND hwnd)
{
	// get control window handles
	g.Dlg1.hDialog1 = hwnd;
	g.Dlg1.hButton1 = GetDlgItem(hwnd,IDC_BUTTON1);
	g.Dlg1.hButton2 = GetDlgItem(hwnd,IDC_BUTTON2);
	g.Dlg1.hButton3 = GetDlgItem(hwnd,IDC_BUTTON3);
	g.Dlg1.hCombo1 = GetDlgItem(hwnd,IDC_COMBO1);
	g.Dlg1.hStatic1 = GetDlgItem(hwnd,IDC_STATIC1);

	// set dialog icons
	HICON hIcon1 = LoadIcon(g.hInstance,MAKEINTRESOURCE(IDI_ICON1));
	HICON hIcon2 = LoadIcon(g.hInstance,MAKEINTRESOURCE(IDI_ICON2));
	SendMessage(hwnd,WM_SETICON,ICON_BIG,(LPARAM)hIcon1);
	SendMessage(hwnd,WM_SETICON,ICON_SMALL,(LPARAM)hIcon2);

	// set dialog title
	SendMessage(hwnd,WM_SETTEXT,0,(LPARAM)APPNAME);

	// create status bar with two parts
	RECT rect;
	GetClientRect(hwnd,&rect);
	g.Dlg1.hStatusBar1 = CreateStatusWindow(WS_CHILD|WS_VISIBLE,0,hwnd,IDC_STATUS_BAR1);
	int widths[2] = {rect.right-50,-1};
	SendMessage(g.Dlg1.hStatusBar1,SB_SETPARTS,2,(LPARAM)widths);

	// set font type for static control
	// create font from installed font type
	LOGFONT lf;
	memset(&lf,0,sizeof(LOGFONT));
	// retrieve handle to device context for client area
	HDC hdc = GetDC(hwnd);
	// set font size to 8
	lf.lfHeight = -MulDiv(8,GetDeviceCaps(hdc,LOGPIXELSY),72);
	// release device context
	ReleaseDC(hwnd,hdc);
	// use "Lucida Console" because it is a monospaced font present on all target OSs
	strcpy(lf.lfFaceName,"Lucida Console");
	// create logical font
	g_hFont = CreateFontIndirect(&lf);
	// set font of static control
	SendMessage(g.Dlg1.hStatic1,WM_SETFONT,(WPARAM)g_hFont,FALSE);

	// initialize vulnerable driver structures
	//lint -e{534} Warning 534: Ignoring return value of function
	MyInitVulnerableDrivers(g.vd,MAX_VULNERABLE_DRIVERS);

	// do this for all vulnerable drivers
	for(unsigned int i = 0; i < MAX_VULNERABLE_DRIVERS; i++)
	{
		// add valid vulnerable driver to combo box
		if(g.vd[i].szProvider[0] != 0) SendMessage(g.Dlg1.hCombo1,CB_ADDSTRING,0,(LPARAM)g.vd[i].szProvider);
	}

	// select first vulnerable driver in combo box
	SendMessage(g.Dlg1.hCombo1,CB_SETCURSEL,0,0);

	// set focus to button 1
	SetFocus(g.Dlg1.hButton1);

	// create tooltip window and associate the tooltip with button 1, 2, 3 and combo box
	//lint -e{534} Warning 534: Ignoring return value of function
	MyDlg1CreateTooltip(g.hInstance,hwnd,g.Dlg1.hButton1);
	//lint -e{534} Warning 534: Ignoring return value of function
	MyDlg1CreateTooltip(g.hInstance,hwnd,g.Dlg1.hButton2);
	//lint -e{534} Warning 534: Ignoring return value of function
	MyDlg1CreateTooltip(g.hInstance,hwnd,g.Dlg1.hButton3);
	//lint -e{534} Warning 534: Ignoring return value of function
	MyDlg1CreateTooltip(g.hInstance,hwnd,g.Dlg1.hCombo1);

	// run initialization thread
	g.ucRunning = 1;
	g.ThreadParams.ttno = ThreadTaskReadDSEOnFirstRun;
	HANDLE hThread = CreateThread(NULL,0,MyThreadProc1,(LPVOID)&g.ThreadParams,0,NULL);
	// close the thread handle as we don't need it
	if(hThread)
	{
		CloseHandle(hThread);
	}

	return 0;
}


//------------------------------------------------------------------------------
// enable or disable the dialog controls
//------------------------------------------------------------------------------
int MyDlg1EnableControls(unsigned char ucEnable)
{
	if(ucEnable == 1)
	{
		EnableWindow(g.Dlg1.hButton1,TRUE);
		EnableWindow(g.Dlg1.hButton2,TRUE);
		EnableWindow(g.Dlg1.hButton3,TRUE);
		EnableWindow(g.Dlg1.hCombo1,TRUE);
		SetFocus(g.Dlg1.hButton1);
	}
	else
	{
		EnableWindow(g.Dlg1.hButton1,FALSE);
		EnableWindow(g.Dlg1.hButton2,FALSE);
		EnableWindow(g.Dlg1.hButton3,FALSE);
		EnableWindow(g.Dlg1.hCombo1,FALSE);
		SetFocus(g.Dlg1.hButton1);
	}

	return 0;
}


//------------------------------------------------------------------------------
// button 1 "DSE Disable" clicked
//------------------------------------------------------------------------------
int MyDlg1Button1OnClick()
{
	// disable controls to prevent starting another thread
	MyDlg1EnableControls(FALSE);
	// run DSE disable thread
	g.ucRunning = 1;
	g.ThreadParams.ttno = ThreadTaskDisableDSE;
	HANDLE hThread = CreateThread(NULL,0,MyThreadProc1,(LPVOID)&g.ThreadParams,0,NULL);
	if(hThread)
	{
		CloseHandle(hThread);
	}

	return 0;
}


//------------------------------------------------------------------------------
// button 2 "DSE Enable" clicked
//------------------------------------------------------------------------------
int MyDlg1Button2OnClick()
{
	// disable controls to prevent starting another thread
	MyDlg1EnableControls(FALSE);
	// run DSE enable thread
	g.ucRunning = 1;
	g.ThreadParams.ttno = ThreadTaskEnableDSE;
	HANDLE hThread = CreateThread(NULL,0,MyThreadProc1,(LPVOID)&g.ThreadParams,0,NULL);
	if(hThread)
	{
		CloseHandle(hThread);
	}

	return 0;
}


//------------------------------------------------------------------------------
// button 3 "DSE Restore" clicked
//------------------------------------------------------------------------------
int MyDlg1Button3OnClick()
{
	// disable controls to prevent starting another thread
	MyDlg1EnableControls(FALSE);
	// run DSE restore thread
	g.ucRunning = 1;
	g.ThreadParams.ttno = ThreadTaskRestoreDSE;
	HANDLE hThread = CreateThread(NULL,0,MyThreadProc1,(LPVOID)&g.ThreadParams,0,NULL);
	if(hThread)
	{
		CloseHandle(hThread);
	}

	return 0;
}


//------------------------------------------------------------------------------
// WM_TIMER message processing
//------------------------------------------------------------------------------
int MyDlg1OnTimer(WPARAM wParam)
{
	UNREFERENCED_PARAMETER(wParam);

	// increment seconds
	g.Dlg1.uiTimerSeconds++;

	// change minutes every 60 seconds
	if(g.Dlg1.uiTimerSeconds == 60)
	{
		g.Dlg1.uiTimerMinutes++;
		g.Dlg1.uiTimerSeconds = 0;
	}
	
	// change hours every 60 minutes
	if(g.Dlg1.uiTimerMinutes == 60)
	{
		g.Dlg1.uiTimerHours++;
		g.Dlg1.uiTimerMinutes = 0;
		g.Dlg1.uiTimerSeconds = 0;
	}
	
	// build time string in the format 00:00:00
	char szTime[9];
	sprintf_s(szTime, sizeof(szTime), "%.2u:%.2u:%.2u", g.Dlg1.uiTimerHours, g.Dlg1.uiTimerMinutes, g.Dlg1.uiTimerSeconds);
		
	// set pane 1 status bar text
	SendMessage(g.Dlg1.hStatusBar1,SB_SETTEXT,1,(LPARAM)szTime);

	return 0;
}


//------------------------------------------------------------------------------
// dialog procedure callback
//------------------------------------------------------------------------------
INT_PTR CALLBACK MyDlg1DlgProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_TIMER:
		//lint -e{534} Warning 534: Ignoring return value of function
		MyDlg1OnTimer(wParam);
		return 1;
	case WM_INITDIALOG:
		//lint -e{534} Warning 534: Ignoring return value of function
		MyDlg1OnInitDialog(hwnd);
		// return FALSE, otherwise the keyboard focus is not set correctly by SetFocus
		return 0;
	case WM_CLOSE:
		// check if thread is running before closing the dialog
		if(g.ucRunning == 0)
		{
			EndDialog(hwnd,0);
		}
		return 1;
	case WM_DESTROY:
		// delete GDI objects
		if(g_hFont)
		{
			DeleteObject(g_hFont);
		}
		return 1;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_BUTTON1:
			switch(HIWORD(wParam))
			{
			case BN_CLICKED:
				//lint -e{534} Warning 534: Ignoring return value of function
				MyDlg1Button1OnClick();
				return 1;
			}
			break;
		case IDC_BUTTON2:
			switch(HIWORD(wParam))
			{
			case BN_CLICKED:
				//lint -e{534} Warning 534: Ignoring return value of function
				MyDlg1Button2OnClick();
				return 1;
			}
			break;
		case IDC_BUTTON3:
			switch(HIWORD(wParam))
			{
			case BN_CLICKED:
				//lint -e{534} Warning 534: Ignoring return value of function
				MyDlg1Button3OnClick();
				return 1;
			}
			break;
		}
		break;
	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->code)
        {
		// this is only triggered if we hover with the mouse over the control
		// for the combo box this is only triggered for the button of the control and not the item list
		//lint -e{835} Warning 835: A zero has been given as right argument to operator '-'
		case TTN_GETDISPINFO:
			// tooltip set multiline text
			//lint -e{534} Warning 534: Ignoring return value of function
			MyDlg1TooltipSetMultilineText(lParam);
			return 1;
		}
		break;
	}
	
	return 0;
}


//------------------------------------------------------------------------------
// print CLI help message
//------------------------------------------------------------------------------
void PrintCLIHelp()
{
	printf("DSE-Patcher - Patch Driver Signature Enforcement\n");
	printf("=================================================\n\n");
	printf("Usage: DSE-Patcher.exe [options]\n\n");
	printf("Options:\n");
	printf("  -disable   Disable Driver Signature Enforcement\n");
	printf("  -enable    Enable Driver Signature Enforcement\n");
	printf("  -restore   Restore DSE to the value captured at CLI startup\n");
	printf("  -help      Show this help message\n\n");
	printf("If no arguments are provided, the GUI will be launched.\n\n");
	printf("Note: This tool requires Administrator privileges.\n");
	printf("      Uses RTCore64 driver for kernel memory access.\n");
}


//------------------------------------------------------------------------------
// WinMain
//------------------------------------------------------------------------------
int __stdcall WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(nCmdShow);

	// zero all global vars
	memset(&g,0,sizeof(GLOBALS));
	g.hInstance = hInstance;

	// check for command line arguments
	if(lpCmdLine != NULL && lpCmdLine[0] != '\0')
	{
		// Try to open log file for debugging - try multiple locations
		char szLogPath[MAX_PATH];
		FILE* fpLog = NULL;
		
		// Try 1: Same directory as executable
		if(GetModuleFileNameA(NULL, szLogPath, MAX_PATH) != 0)
		{
			char* pExt = strrchr(szLogPath, '.');
			if(pExt != NULL)
			{
				strcpy_s(pExt, MAX_PATH - (pExt - szLogPath), ".log");
			}
			else
			{
				strcat_s(szLogPath, MAX_PATH, ".log");
			}
			fopen_s(&fpLog, szLogPath, "w");
		}
		
		// Try 2: TEMP directory if first attempt failed
		if(fpLog == NULL)
		{
			char szTempPath[MAX_PATH];
			if(GetTempPathA(MAX_PATH, szTempPath) != 0)
			{
				strcat_s(szTempPath, MAX_PATH, "DSE-Patcher.log");
				fopen_s(&fpLog, szTempPath, "w");
			}
		}
		
		// Set unbuffered mode for immediate writes
		if(fpLog) setvbuf(fpLog, NULL, _IONBF, 0);
		
		if(fpLog) fprintf(fpLog, "=== DSE-Patcher CLI Log ===\n");
		if(fpLog) fprintf(fpLog, "CLI started with args: '%s'\n", lpCmdLine);
		
		// Try to attach to parent console first (e.g., cmd.exe or powershell)
		BOOL bAttachedToParent = AttachConsole(ATTACH_PARENT_PROCESS);
		BOOL bAllocatedConsole = FALSE;
		if(fpLog) fprintf(fpLog, "AttachConsole(ATTACH_PARENT_PROCESS) result: %d, LastError: %lu\n", bAttachedToParent, GetLastError());
		
		if(!bAttachedToParent)
		{
			// No parent console, allocate a new one
			bAllocatedConsole = AllocConsole();
			DWORD dwAllocError = GetLastError();
			if(fpLog) fprintf(fpLog, "AllocConsole result: %d, LastError: %lu\n", bAllocatedConsole, dwAllocError);
			
			if(!bAllocatedConsole)
			{
				if(fpLog) { fprintf(fpLog, "FATAL: Failed to allocate console\n"); fclose(fpLog); }
				MessageBox(NULL, "Failed to allocate console.", "Error", MB_OK | MB_ICONERROR);
				return 1;
			}
			
			// Set console title only for our own console
			SetConsoleTitleA("DSE-Patcher CLI");
			if(fpLog) fprintf(fpLog, "Console title set\n");
		}
		else
		{
			if(fpLog) fprintf(fpLog, "Attached to parent console\n");
		}
		
		// Get console handles
		HANDLE hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
		HANDLE hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);
		if(fpLog) fprintf(fpLog, "Console handles: out=%p, in=%p\n", (void*)hConsoleOut, (void*)hConsoleIn);
		
		// Write startup message directly to console first
		if(hConsoleOut != INVALID_HANDLE_VALUE && hConsoleOut != NULL)
		{
			DWORD written = 0;
			// Print newline first if attached to parent console (to separate from prompt)
			if(bAttachedToParent)
			{
				WriteConsoleA(hConsoleOut, "\r\n", 2, &written, NULL);
			}
			const char* msg = "DSE-Patcher CLI\r\n================\r\n";
			WriteConsoleA(hConsoleOut, msg, (DWORD)strlen(msg), &written, NULL);
			if(fpLog) fprintf(fpLog, "Initial WriteConsole wrote %lu bytes\n", written);
		}
		
		// Redirect stdio using _fdopen
		if(fpLog) fprintf(fpLog, "Attempting stdio redirection...\n");
		
		FILE* fpStdout = NULL;
		FILE* fpStdin = NULL;
		BOOL bStdioOK = FALSE;
		
		// Method: Use _fdopen with console file descriptor
		int hConOut = _open_osfhandle((intptr_t)hConsoleOut, _O_TEXT);
		int hConIn = _open_osfhandle((intptr_t)hConsoleIn, _O_TEXT);
		if(fpLog) fprintf(fpLog, "_open_osfhandle results: out=%d, in=%d\n", hConOut, hConIn);
		
		if(hConOut != -1)
		{
			fpStdout = _fdopen(hConOut, "w");
			if(fpLog) fprintf(fpLog, "_fdopen stdout: %p\n", (void*)fpStdout);
			
			if(fpStdout != NULL)
			{
				// Don't try to replace stdout - it crashes with DDK CRT
				// Just use fpStdout directly for our output
				setvbuf(fpStdout, NULL, _IONBF, 0);
				bStdioOK = TRUE;
				if(fpLog) fprintf(fpLog, "fpStdout ready (not replacing stdout)\n");
			}
		}
		
		if(hConIn != -1)
		{
			fpStdin = _fdopen(hConIn, "r");
			if(fpLog) fprintf(fpLog, "_fdopen stdin: %p\n", (void*)fpStdin);
		}
		
		// Test output using fpStdout
		if(bStdioOK && fpStdout != NULL)
		{
			fprintf(fpStdout, "Console ready.\n");
			fflush(fpStdout);
			if(fpLog) fprintf(fpLog, "fprintf test done\n");
		}
		else
		{
			if(fpLog) fprintf(fpLog, "stdio redirection failed, will use WriteConsole\n");
		}

		int result = 0;
		char msgBuf[1024];
		DWORD written = 0;
		
		// Helper macro for console output
		#define CLI_PRINT(msg) \
			do { \
				if(fpStdout) { fprintf(fpStdout, "%s", msg); fflush(fpStdout); } \
				else { WriteConsoleA(hConsoleOut, msg, (DWORD)strlen(msg), &written, NULL); } \
			} while(0)

		// parse command line arguments
		if(_stricmp(lpCmdLine, "-disable") == 0)
		{
			if(fpLog) fprintf(fpLog, "Calling MyExecuteCLI(ThreadTaskDisableDSE)\n");
			CLI_PRINT("Executing: Disable DSE...\r\n\r\n");
			result = MyExecuteCLI(ThreadTaskDisableDSE);
			sprintf_s(msgBuf, sizeof(msgBuf), "\r\nResult: %s (code %d)\r\n", result == 0 ? "SUCCESS" : "FAILED", result);
			CLI_PRINT(msgBuf);
			if(fpLog) fprintf(fpLog, "MyExecuteCLI returned: %d\n", result);
		}
		else if(_stricmp(lpCmdLine, "-enable") == 0)
		{
			if(fpLog) fprintf(fpLog, "Calling MyExecuteCLI(ThreadTaskEnableDSE)\n");
			CLI_PRINT("Executing: Enable DSE...\r\n\r\n");
			result = MyExecuteCLI(ThreadTaskEnableDSE);
			sprintf_s(msgBuf, sizeof(msgBuf), "\r\nResult: %s (code %d)\r\n", result == 0 ? "SUCCESS" : "FAILED", result);
			CLI_PRINT(msgBuf);
			if(fpLog) fprintf(fpLog, "MyExecuteCLI returned: %d\n", result);
		}
		else if(_stricmp(lpCmdLine, "-restore") == 0)
		{
			if(fpLog) fprintf(fpLog, "Calling MyExecuteCLI(ThreadTaskRestoreDSE)\n");
			CLI_PRINT("Executing: Restore DSE...\r\n\r\n");
			result = MyExecuteCLI(ThreadTaskRestoreDSE);
			sprintf_s(msgBuf, sizeof(msgBuf), "\r\nResult: %s (code %d)\r\n", result == 0 ? "SUCCESS" : "FAILED", result);
			CLI_PRINT(msgBuf);
			if(fpLog) fprintf(fpLog, "MyExecuteCLI returned: %d\n", result);
		}
		else if(_stricmp(lpCmdLine, "-help") == 0 || _stricmp(lpCmdLine, "--help") == 0 || _stricmp(lpCmdLine, "/?") == 0)
		{
			if(fpLog) fprintf(fpLog, "Showing help\n");
			CLI_PRINT("DSE-Patcher CLI Help\r\n");
			CLI_PRINT("====================\r\n\r\n");
			CLI_PRINT("Usage: DSE-Patcher.exe [options]\r\n\r\n");
			CLI_PRINT("Options:\r\n");
			CLI_PRINT("  -disable   Disable Driver Signature Enforcement\r\n");
			CLI_PRINT("  -enable    Enable Driver Signature Enforcement\r\n");
			CLI_PRINT("  -restore   Restore DSE to original value\r\n");
			CLI_PRINT("  -help      Show this help message\r\n\r\n");
			CLI_PRINT("Note: Requires Administrator privileges.\r\n");
			result = 0;
		}
		else
		{
			if(fpLog) fprintf(fpLog, "Unknown argument: '%s'\n", lpCmdLine);
			sprintf_s(msgBuf, sizeof(msgBuf), "Error: Unknown argument '%s'\r\n\r\nUse -help for usage information.\r\n", lpCmdLine);
			CLI_PRINT(msgBuf);
			result = 1;
		}

		if(fpLog) fprintf(fpLog, "Command completed, result=%d\n", result);

		// Only wait for Enter if we allocated our own console (not attached to parent)
		if(bAllocatedConsole)
		{
			CLI_PRINT("\r\nPress Enter to exit...\r\n");
			
			if(fpLog) fprintf(fpLog, "Waiting for Enter key...\n");
			
			// Read from console
			if(fpStdin != NULL)
			{
				if(fpLog) fprintf(fpLog, "Using fgetc(fpStdin)\n");
				fgetc(fpStdin);
			}
			else if(hConsoleIn != INVALID_HANDLE_VALUE && hConsoleIn != NULL)
			{
				if(fpLog) fprintf(fpLog, "Using ReadConsoleA()\n");
				char inputBuf[16];
				DWORD readCount = 0;
				ReadConsoleA(hConsoleIn, inputBuf, 1, &readCount, NULL);
			}
			else
			{
				if(fpLog) fprintf(fpLog, "Using MessageBox fallback\n");
				MessageBox(NULL, "CLI completed. Click OK to close.", "DSE-Patcher", MB_OK);
			}
		}
		else
		{
			// Attached to parent console - just print a newline and let the shell continue
			CLI_PRINT("\r\n");
			if(fpLog) fprintf(fpLog, "Attached to parent, not waiting for input\n");
		}
		
		#undef CLI_PRINT
		
		if(fpLog) { fprintf(fpLog, "CLI completed successfully.\n"); fclose(fpLog); }
		FreeConsole();

		return result;
	}

	// no command line arguments, launch GUI
	// create dialog box from resource
	DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_DIALOG1),0,MyDlg1DlgProc,0);

	return 0;
}
