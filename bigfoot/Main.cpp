#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <strsafe.h>
#include <filesystem>
#include <chrono>
#include <vector>
#include <shlobj.h>
#include <Windows.h>
#include "resource.h"
#include "Definitions.h"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow)
{
	//InitCommonControls();

	WNDCLASS mainClass = NewWindowClass((HBRUSH)COLOR_WINDOW, LoadCursor(NULL, IDC_ARROW), hInst, 
		LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)), L"MainWindowClass", SoftwareMainProc);

	WNDCLASS listContainer = NewWindowClass(brushGrey, LoadCursor(NULL, IDC_ARROW), hInst,
		NULL, L"ScheduleList", SoftwareListProc);

	WNDCLASS listItem = NewWindowClass(brushBlue, LoadCursor(NULL, IDC_ARROW), hInst,
		NULL, L"ScheduleItem", SoftwareItemProc);

	// class registration
	if(!RegisterClassW(&mainClass)) 
	{
		return -1;
	}

	if (!RegisterClassW(&listContainer))
	{
		return -1;
	}

	if (!RegisterClassW(&listItem))
	{
		return -1;
	}

	MSG SoftwareMainMsg = { 0 };

	mainContainer = CreateWindow(L"MainWindowClass", L"bigfoot",
		WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		100, 100, 700, 500, NULL, NULL, NULL, NULL);

	while (GetMessage(&SoftwareMainMsg, NULL, NULL, NULL))
	{
		TranslateMessage(&SoftwareMainMsg);
		DispatchMessage(&SoftwareMainMsg);
	}
	return 0;
}

WNDCLASS NewWindowClass(HBRUSH bgColor, HCURSOR cursor, HINSTANCE hInst, HICON icon, LPCWSTR name, 
	WNDPROC procedure)
{
	WNDCLASS NWC = { 0 };

	NWC.hCursor = cursor;
	NWC.hIcon = icon;
	NWC.hInstance = hInst;
	NWC.lpszClassName = name;
	NWC.hbrBackground = bgColor;
	NWC.lpfnWndProc = procedure;

	return NWC;
}

LRESULT CALLBACK SoftwareMainProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	int id;

	LPCWSTR lpwstr1AL;
	LPCWSTR lpwstr2AL;

	switch (msg)
	{
	case WM_CREATE:
	{
		CURRENT_STATE = S_ONLAUNCH;
		//createWithAdminRights();
		deserialization();

		MainWndAddMenu(hWnd);
		MainWndAddWidget(hWnd);

		if (scheduleInUse != "-") 
		{
			restoreScheduleInUse(hWnd);
		}
		CURRENT_STATE = S_INPROGRESS;

		break;
	}
	case WM_COMMAND:
		switch (wp)
		{
		// menu
		case OnMenuOpenSch:
		{
			openFileDialogOpen();

			break;
		}
		case OnMenuSaveSch:
		{
			if (itemsCount == 0) break;

			SendMessage(mainContainer, WM_COMMAND, OnButtonSort, 0);
			int res = openFileDialogSave();
			//if (res != 0)
			//{
			//	MessageBoxA(hWnd, "Error", "Something went wrong...", MB_OK | MB_ICONERROR);
			//}
			break;
		}
		case OnMenuCloseSch:
		{
			if (isScheduleOpened)
			{
				if (itemsCount == 0) break;

				SendMessage(mainContainer, WM_COMMAND, OnButtonSort, 0);
				openFileDialogSave();
				currentSchedule = "-";

				LPWSTR curSchW = stringToWchar_ANSI(currentSchedule);
				HWND curSch = GetDlgItem(mainContainer, WndCurSchId);
				SetWindowText(curSch, curSchW);
				CURRENT_RIGHTS = R_ABSOLUTE;
				SendMessage(mainContainer, WM_COMMAND, OnButtonClear, 0);
				CURRENT_RIGHTS = R_LIMITED;
			}
			isScheduleOpened = false;
			wasScheduleChanged = false;
			defaultFillClickedItems();

			break;
		}
		case OnMenuExit:
		{
			if (itemsCount > 0 && !isScheduleOpened ||
				isScheduleOpened && wasScheduleChanged)
			{
				int result = MessageBoxA(
					mainContainer,
					"Do you want to save current schedule?",
					"Confirmation box",
					MB_YESNO | MB_ICONQUESTION
				);

				if (result == IDYES)
				{
					SendMessage(mainContainer, WM_COMMAND, OnMenuSaveSch, 0);
				}
			}
			SendMessage(mainContainer, WM_DESTROY, 0, 0);
			break;
		}
		// buttons
		case OnButtonSort:
		{
			if (!isEditMode) break;

			normalizeItemsTime();
			sort();
			defaultFillClickedItems();

			break;
		}
		case OnButtonCheck:
		{
			if (!isEditMode) break;
			if (!isAnyOfItemsClicked()) break;

			checkItemExistence();

			//testBckgLauncherPerf();
			break;
		}
		case OnButtonLaunch:
		{
			if (!isEditMode) break;
			if (!isAnyOfItemsClicked()) break;

			launchApplication(getPathOfSelectedItem());

			break;
		}
		case OnButtonDelete:
		{
			if (!isEditMode) break;
			if (!isAnyOfItemsClicked()) break;

			id = (int)GetWindowLongPtr(selectedItem, GWLP_USERDATA);

			DestroyWindow(selectedItem);
			selectedItem = NULL;

			correctPosition(id);
			clickedItemsRestore();
			itemsCount--;

			if (isScheduleOpened)
			{
				wasScheduleChanged = true;
			}

			break;
		}
		case OnButtonAdd:
		{
			if (!isEditMode) break;
			if (openFileDialogAdd() == 0)
			{
				ListWndAddItem(listContainer);
			}
			break;
		}
		case OnButtonClear:
		{
			if (CURRENT_RIGHTS == R_LIMITED)
			{
				if (!isEditMode) break;
			}
			RemoveAllChildWindows(listContainer);

			listContainerHeight = mainClientRect.bottom - contTop;
			listHeight = 0;

			itemsCount = 0;

			SCROLLINFO scrInfo;
			scrInfo.cbSize = sizeof(SCROLLINFO);
			scrInfo.fMask = SIF_ALL;
			GetScrollInfo(listContainer, SB_VERT, &scrInfo);

			scrInfo.nMin = 0;
			scrInfo.nMax = listContainerHeight;
			scrInfo.nPos = 0;

			SetScrollInfo(listContainer, SB_VERT, &scrInfo, TRUE);

			defaultFillClickedItems();

			break;
		}
		case OnButtonEdit:
		{
			if (isEditMode)
			{
				SetWindowText(modeBtn, L"View mode");
				isEditMode = false;
			}
			else
			{
				SetWindowText(modeBtn, L"Edit mode");
				isEditMode = true;
			}
			break;
		}
		case OnButtonActLchr:
		{
			if (!launcherActivated)
			{
				if (addLauncherToStartup())
				{
					SetWindowText(launchMode, L"Deactivate launcher");
					launcherActivated = true;
				}
				 
				// config path
				// service name
				//lpwstr1AL = stringToLPCWSTR_UTF("bckglauncher");
				//lpwstr2AL = stringToLPCWSTR_ANSI(onActivateLauncher());
				//CreateMyService(lpwstr1AL, lpwstr2AL);

			}
			else
			{
				if (removeLauncherFromStartup())
				{
					SetWindowText(launchMode, L"Activate launcher");
					launcherActivated = false;
				}
				// service name
				//lpwstr1AL = stringToLPCWSTR_UTF("bckglauncher");
				//DeleteMyService(lpwstr1AL);

			}
			break;
		}
		case OnButtonClrSchU:
		{
			scheduleInUse = "-";

			LPWSTR useSchW = stringToWchar_ANSI(scheduleInUse);

			HWND useSch = GetDlgItem(mainContainer, WndUseSchId);
			SetWindowText(useSch, useSchW);
			break;
		}
		case OnButtonUseCurSch:
		{
			if (isScheduleOpened && itemsCount > 0)
			{
				scheduleInUse = currentSchedule;

				// string to wchar_t
				size_t pos = scheduleInUse.find_last_of("/\\");
				std::string filename = 
					(pos != std::string::npos) ? scheduleInUse.substr(pos + 1) : scheduleInUse;

				LPWSTR useSchW = stringToWchar_ANSI(filename);

				HWND useSch = GetDlgItem(mainContainer, WndUseSchId);
				SetWindowText(useSch, useSchW);

				useCurrentScheme();
			}
			break;
		}
		default:
			break;
		}
		break;
	case WM_DESTROY:
	{
		serialization();

		PostQuitMessage(0);
	}
	default:
		return DefWindowProc(hWnd, msg, wp, lp);
	}
}

LRESULT CALLBACK SoftwareListProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_CREATE:
	{
		SCROLLINFO scrInfo;
		scrInfo.cbSize = sizeof(SCROLLINFO);
		scrInfo.fMask = SIF_RANGE | SIF_PAGE;
		scrInfo.nMin = 0;
		scrInfo.nMax = listContainerHeight;
		scrInfo.nPage = listContainerHeight + 1;
		scrInfo.nPos = 0;
		SetScrollInfo(hWnd, SB_VERT, &scrInfo, TRUE);
		break;
	}
	case WM_COMMAND:
		switch (wp)
		{
		
		default:
			break;
		}
		break;
	case WM_VSCROLL: 
	{
		SCROLLINFO scrInfo;
		scrInfo.cbSize = sizeof(SCROLLINFO);
		scrInfo.fMask = SIF_ALL; 
		GetScrollInfo(hWnd, SB_VERT, &scrInfo);

		int currentPos = scrInfo.nPos;

		switch (LOWORD(wp)) {
		case SB_LINEUP:
		{
			if (scrInfo.nPos > 5)
			{
				scrInfo.nPos -= 10;
			}
			else
			{
				scrInfo.nPos = scrInfo.nMin;
			}
			break;
		}
		case SB_LINEDOWN:
		{
			if (scrInfo.nPos + scrInfo.nPage < scrInfo.nMax - 5)
			{
				scrInfo.nPos += 10;
			}
			else
			{
				if (scrInfo.nPage > scrInfo.nMax)
					break;
				scrInfo.nPos = scrInfo.nMax - scrInfo.nPage;
			}
			break;
		}
		case SB_THUMBTRACK:
		{
			scrInfo.nPos = scrInfo.nTrackPos;
			break;
		}
		default:
			return 0;
		}

		scrInfo.fMask = SIF_POS; 
		SetScrollInfo(hWnd, SB_VERT, &scrInfo, TRUE); 

		int yScroll = currentPos - scrInfo.nPos;
		ScrollWindow(hWnd, 0, yScroll, NULL, NULL); 

		//InvalidateRect(hWnd, NULL, TRUE); // redraw window
		//UpdateWindow(hWnd); // update window

		break;
	}
	case WM_MOUSEWHEEL:
	{
		int delta = GET_WHEEL_DELTA_WPARAM(wp);
		if (delta > 0) {
			SendMessage(hWnd, WM_VSCROLL, SB_LINEUP, 0);
		}
		else if (delta < 0) {
			SendMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, 0);
		}
		break;
	}
	default:
		return DefWindowProc(hWnd, msg, wp, lp);
	}
}

LRESULT CALLBACK SoftwareItemProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	int id = (int)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	int controlId = static_cast<int>(LOWORD(wp));
	int notificationCode = static_cast<int>(HIWORD(wp));

	TRACKMOUSEEVENT tme;
	PAINTSTRUCT ps;
	HDC hdc;
	HPEN hPen;
	HPEN hOldPen;

	switch (msg)
	{
	case WM_CREATE:
	{
		break;
	}
	case WM_COMMAND:
	{
		switch (controlId)
		{
		case ItemHoursId:
		case ItemMinutesId:
		case ItemSecondsId:
			switch (notificationCode)
			{
			case EN_KILLFOCUS:
			{
				if (!isEditMode) break;
				if (isScheduleOpened) 
				{
					wasScheduleChanged = true;
				}
			}
			break;
			default:
				break;
			}
			break;

		default:
			break;
		}
		break;
	}
	case WM_CTLCOLORSTATIC: 
	{
		HDC hdcStatic = (HDC)wp;
		HWND hStatic = (HWND)lp;

		int ctrlID = GetDlgCtrlID(hStatic);

		if (ctrlID == ItemLaunchId || ctrlID == ItemNumberId) {
			SetTextColor(hdcStatic, RGB(255, 255, 255));
			SetBkMode(hdcStatic, TRANSPARENT);

			return (LRESULT)brushBlue;
		}
		return DefWindowProc(hWnd, msg, wp, lp);
	}
	case WM_MOUSEMOVE:
	{
		if (!isItemHovered)
		{
			isItemHovered = true;
			tme.cbSize = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = hWnd;
			TrackMouseEvent(&tme);
			InvalidateRect(hWnd, NULL, TRUE);
		}
		break;
	}
	case WM_MOUSELEAVE:
	{
		if (isItemHovered)
		{
			isItemHovered = false;
			InvalidateRect(hWnd, NULL, TRUE);
		}
		break;
	}
	case WM_LBUTTONDOWN:
	{
		int id = (int)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (!isItemClicked[id])
		{
			//int xPos = LOWORD(lp);
			//int yPos = HIWORD(lp);
			clickedItemsRestore();
			isItemClicked[id] = true;
			selectedItem = hWnd;
			InvalidateRect(hWnd, NULL, TRUE);
			// check if cursor in right area
			//if (xPos >= 0 && xPos <= itemWidth && yPos >= 0 && yPos <= itemHeight) {
			//	
			//	
			//}
			//else {
			//	
			//	
			//}
		}
		else 
		{
			isItemClicked[id] = false;
			selectedItem = NULL;
			InvalidateRect(hWnd, NULL, TRUE);
		}
		break;
	}
	case WM_PAINT:
	{
		if (isItemClicked[id])
		{
			hdc = BeginPaint(hWnd, &ps);

			//Rectangle(ps.hdc, paintRect.left, paintRect.top, paintRect.right, paintRect.bottom);
			//FillRect(ps.hdc, &paintRect, brushGrey);

			// red pen
			hPen = CreatePen(PS_SOLID, 6, RGB(255, 188, 17));
			hOldPen = (HPEN)SelectObject(hdc, hPen);

			// rect
			drawRect(hdc, 0, 0, itemWidth, itemHeight);

			// restore old pen
			SelectObject(hdc, hOldPen);
			DeleteObject(hPen);

			EndPaint(hWnd, &ps);
		}
		else if (isItemHovered)
		{
			hdc = BeginPaint(hWnd, &ps);

			// green pen
			hPen = CreatePen(PS_SOLID, 6, RGB(0, 196, 41));
			hOldPen = (HPEN)SelectObject(hdc, hPen);

			// rect
			drawRect(hdc, 0, 0, itemWidth, itemHeight);

			// restore old pen
			SelectObject(hdc, hOldPen);
			DeleteObject(hPen);

			EndPaint(hWnd, &ps);
		}
		else
		{
			return DefWindowProc(hWnd, msg, wp, lp);
		}
		//RedrawWindow(hWnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE);
		break;
	}
	default:
		return DefWindowProc(hWnd, msg, wp, lp);
	}
	
}

void MainWndAddMenu(HWND hWnd) 
{
	HMENU rootMenu = CreateMenu();
	HMENU subMenu1 = CreateMenu();

	AppendMenu(subMenu1, MF_STRING, OnMenuOpenSch, L"Open schedule");
	AppendMenu(subMenu1, MF_STRING, OnMenuSaveSch, L"Save schedule");
	AppendMenu(subMenu1, MF_STRING, OnMenuCloseSch, L"Close schedule");
	AppendMenu(subMenu1, MF_STRING, OnMenuExit, L"Exit");

	AppendMenu(rootMenu, MF_POPUP, (UINT_PTR)subMenu1, L"File");

	SetMenu(hWnd, rootMenu);
}

void MainWndAddWidget(HWND hWnd)
{
	// client's window size
	GetClientRect(hWnd, &mainClientRect);

	// left widget 
	// text
	CreateWindowA("static", "Schedule in use:", WS_VISIBLE | WS_CHILD | ES_CENTER,
		mainClientRect.left, mainClientRect.top + 53, 200, 20,
		hWnd, NULL, NULL, NULL);

	CreateWindowA("static", "Current Schedule:", WS_VISIBLE | WS_CHILD | ES_CENTER,
		mainClientRect.left, mainClientRect.top + 110, 200, 20,
		hWnd, NULL, NULL, NULL);

	if (scheduleInUse != "-")
	{
		size_t pos = scheduleInUse.find_last_of("/\\");
		std::string filename =
			(pos != std::string::npos) ? scheduleInUse.substr(pos + 1) : scheduleInUse;

		CreateWindowA("static", filename.c_str(), WS_VISIBLE | WS_CHILD | ES_CENTER,
			mainClientRect.left, mainClientRect.top + 80, 200, 20,
			hWnd, (HMENU)WndUseSchId, NULL, NULL);

		CreateWindowA("static", filename.c_str(), WS_VISIBLE | WS_CHILD | ES_CENTER,
			mainClientRect.left, mainClientRect.top + 137, 200, 20,
			hWnd, (HMENU)WndCurSchId, NULL, NULL);
	}
	else
	{
		CreateWindowA("static", scheduleInUse.c_str(), WS_VISIBLE | WS_CHILD | ES_CENTER,
			mainClientRect.left, mainClientRect.top + 80, 200, 20,
			hWnd, (HMENU)WndUseSchId, NULL, NULL);

		CreateWindowA("static", currentSchedule.c_str(), WS_VISIBLE | WS_CHILD | ES_CENTER,
			mainClientRect.left, mainClientRect.top + 137, 200, 20,
			hWnd, (HMENU)WndCurSchId, NULL, NULL);
	}

	// buttons
	CreateWindowA("button", "Use current Schedule", WS_VISIBLE | WS_CHILD,
		2, mainClientRect.bottom - 120, 198, 30,
		hWnd, (HMENU)OnButtonUseCurSch, NULL, NULL);

	CreateWindowA("button", "Clear Schedule in use", WS_VISIBLE | WS_CHILD,
		2, mainClientRect.bottom - 80, 198, 30,
		hWnd, (HMENU)OnButtonClrSchU, NULL, NULL);

	if (launcherActivated)
	{
		launchMode = CreateWindowA("button", "Deactivate launcher", WS_VISIBLE | WS_CHILD,
			2, mainClientRect.bottom - 40, 198, 30,
			hWnd, (HMENU)OnButtonActLchr, NULL, NULL);
	}
	else
	{
		launchMode = CreateWindowA("button", "Activate launcher", WS_VISIBLE | WS_CHILD,
			2, mainClientRect.bottom - 40, 198, 30,
			hWnd, (HMENU)OnButtonActLchr, NULL, NULL);
	}

	// top widget
	// control buttons
	CreateWindowA("button", "Sort", WS_VISIBLE | WS_CHILD,
		2, mainClientRect.top, 100, 30,
		hWnd, (HMENU)OnButtonSort, NULL, NULL);

	CreateWindowA("button", "Check", WS_VISIBLE | WS_CHILD,
		102, mainClientRect.top, 100, 30,
		hWnd, (HMENU)OnButtonCheck, NULL, NULL);

	CreateWindowA("button", "Launch", WS_VISIBLE | WS_CHILD,
		202, mainClientRect.top, 100, 30,
		hWnd, (HMENU)OnButtonLaunch, NULL, NULL);

	CreateWindowA("button", "Add", WS_VISIBLE | WS_CHILD,
		302, mainClientRect.top, 100, 30,
		hWnd, (HMENU)OnButtonAdd, NULL, NULL);

	CreateWindowA("button", "Delete", WS_VISIBLE | WS_CHILD,
		402, mainClientRect.top, 80, 30,
		hWnd, (HMENU)OnButtonDelete, NULL, NULL);

	CreateWindowA("button", "Clear", WS_VISIBLE | WS_CHILD, 
		482, mainClientRect.top, 100, 30,
		hWnd, (HMENU)OnButtonClear, NULL, NULL);

	modeBtn = CreateWindowA("button", "View mode", WS_VISIBLE | WS_CHILD, 
		582, mainClientRect.top, 100, 30,
		hWnd, (HMENU)OnButtonEdit, NULL, NULL);

	// delimeters
	CreateWindowA("static", "", WS_VISIBLE | WS_CHILD | SS_ETCHEDHORZ,
		mainClientRect.left , mainClientRect.top + 31, mainClientRect.right - mainClientRect.left, 2,
		hWnd, NULL, NULL, NULL);

	CreateWindowA("static", "", WS_VISIBLE | WS_CHILD | SS_ETCHEDVERT,
		mainClientRect.left + 200, mainClientRect.top + 33, 2, mainClientRect.bottom - mainClientRect.top,
		hWnd, NULL, NULL, NULL);

	// central widget
	// schedule list
	contLeft = mainClientRect.left + 203;
	contTop = mainClientRect.top + 33;
	listContainerHeight = mainClientRect.bottom - contTop;

	listContainer = CreateWindowA("ScheduleList", "", WS_VISIBLE | WS_CHILD | WS_VSCROLL,
		contLeft, contTop, mainClientRect.right - contLeft, listContainerHeight,
		hWnd, NULL, NULL, NULL);

}

void ListWndAddItem(HWND hWnd) {
	if (itemsCount == maxItems)
	{
		return;
	}

	if (isScheduleOpened)
	{
		wasScheduleChanged = true;
	}

	SCROLLINFO scrInfo;
	scrInfo.cbSize = sizeof(SCROLLINFO);
	scrInfo.fMask = SIF_ALL;
	GetScrollInfo(hWnd, SB_VERT, &scrInfo);

	itemWidth = mainClientRect.right - contLeft - 22;
	itemHeight = 40;

	itemsCount++;

	HWND item = CreateWindowA("ScheduleItem", "", WS_VISIBLE | WS_CHILD,
		2, 2 + listHeight - scrInfo.nPos, itemWidth, itemHeight,
		hWnd, (HMENU)itemsCount, NULL, NULL);

	SetWindowLongPtr(item, GWLP_USERDATA, (LONG_PTR)itemsCount);

	listHeight += 42;

	// number
	HWND number = CreateWindowA("static", ("№"+std::to_string(itemsCount)).c_str(), 
		WS_VISIBLE | WS_CHILD | ES_CENTER,
		10, 10, 30, 20,
		item, (HMENU)ItemNumberId, NULL, NULL);

	// application name
	size_t pos = tmpAppName.find_last_of("/\\");
	std::string appFilename =
		(pos != std::string::npos) ? tmpAppName.substr(pos + 1) : tmpAppName;

	HWND appName = CreateWindowA("static", (" " + appFilename).c_str(),
		WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP,
		45, 10, 150, 20,
		item, (HMENU)ItemAppNameId, NULL, NULL);

	std::string* tmpAppName1 = new std::string(tmpAppName);
	SetWindowLongPtr(appName, GWLP_USERDATA, (LONG_PTR)tmpAppName1);

	// popup tip
	CreateToolTip(item, tmpAppName);

	// delimiter
	HWND delimiter = CreateWindowA("static", "", WS_VISIBLE | WS_CHILD | SS_ETCHEDVERT,
		mainClientRect.right - contLeft - 270, 3, 2, 36,
		item, NULL, NULL, NULL);

	// time control text
	HWND textLaunch = CreateWindowA("static", "Launch delay:", WS_VISIBLE | WS_CHILD | ES_CENTER,
		mainClientRect.right - contLeft - 265, 10, 100, 20,
		item, (HMENU)ItemLaunchId, NULL, NULL);

	// time control hms
	HWND textHours = CreateWindowA("static", "h", WS_VISIBLE | WS_CHILD | ES_CENTER,
		mainClientRect.right - contLeft - 160, 10, 20, 20,
		item, NULL, NULL, NULL);

	HWND hours = CreateWindowA("edit", "0", WS_VISIBLE | WS_CHILD | ES_NUMBER | ES_CENTER,
		mainClientRect.right - contLeft - 140, 10, 20, 20,
		item, (HMENU)ItemHoursId, NULL, NULL);

	HWND textMinutes = CreateWindowA("static", "m", WS_VISIBLE | WS_CHILD | ES_CENTER,
		mainClientRect.right - contLeft - 115, 10, 20, 20,
		item, NULL, NULL, NULL);

	HWND minutes = CreateWindowA("edit", "0", WS_VISIBLE | WS_CHILD | ES_NUMBER | ES_CENTER,
		mainClientRect.right - contLeft - 95, 10, 20, 20,
		item, (HMENU)ItemMinutesId, NULL, NULL);

	HWND textSeconds = CreateWindowA("static", "s", WS_VISIBLE | WS_CHILD | ES_CENTER,
		mainClientRect.right - contLeft - 70, 10, 20, 20,
		item, NULL, NULL, NULL);

	HWND seconds = CreateWindowA("edit", "0", WS_VISIBLE | WS_CHILD | ES_NUMBER | ES_CENTER,
		mainClientRect.right - contLeft - 50, 10, 20, 20,
		item, (HMENU)ItemSecondsId, NULL, NULL);

	if (listHeight > listContainerHeight) {
		listContainerHeight = listHeight + 2;
 
		scrInfo.nMax = listContainerHeight; 

		SetScrollInfo(hWnd, SB_VERT, &scrInfo, TRUE);
	}
}

void ListWndRestoreItem(HWND hWnd, std::string appN, std::string h, std::string m, std::string s)
{
	if (itemsCount == maxItems)
	{
		return;
	}

	SCROLLINFO scrInfo;
	scrInfo.cbSize = sizeof(SCROLLINFO);
	scrInfo.fMask = SIF_ALL;
	GetScrollInfo(hWnd, SB_VERT, &scrInfo);

	itemWidth = mainClientRect.right - contLeft - 22;
	itemHeight = 40;

	itemsCount++;

	HWND item = CreateWindowA("ScheduleItem", "", WS_VISIBLE | WS_CHILD,
		2, 2 + listHeight - scrInfo.nPos, itemWidth, itemHeight,
		hWnd, (HMENU)itemsCount, NULL, NULL);

	SetWindowLongPtr(item, GWLP_USERDATA, (LONG_PTR)itemsCount);

	listHeight += 42;

	//paintRect = {0, 0 + listHeight - scrInfo.nPos, mainClientRect.right - contLeft - 22, 40};

	// number
	HWND number = CreateWindowA("static", ("№" + std::to_string(itemsCount)).c_str(),
		WS_VISIBLE | WS_CHILD | ES_CENTER,
		10, 10, 30, 20,
		item, (HMENU)ItemNumberId, NULL, NULL);

	// application name
	size_t pos = appN.find_last_of("/\\");
	std::string appFilename =
		(pos != std::string::npos) ? appN.substr(pos + 1) : appN;

	HWND appName = CreateWindowA("static", (" " + appFilename).c_str(),
		WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP,
		45, 10, 150, 20,
		item, (HMENU)ItemAppNameId, NULL, NULL);

	std::string* appN1 = new std::string(appN);
	SetWindowLongPtr(appName, GWLP_USERDATA, (LONG_PTR)appN1);

	// popup tip
	CreateToolTip(item, appN);

	// delimiter
	HWND delimiter = CreateWindowA("static", "", WS_VISIBLE | WS_CHILD | SS_ETCHEDVERT,
		mainClientRect.right - contLeft - 270, 3, 2, 36,
		item, NULL, NULL, NULL);

	// time control text
	HWND textLaunch = CreateWindowA("static", "Launch delay:", WS_VISIBLE | WS_CHILD | ES_CENTER,
		mainClientRect.right - contLeft - 265, 10, 100, 20,
		item, (HMENU)ItemLaunchId, NULL, NULL);

	// time control hms
	HWND textHours = CreateWindowA("static", "h", WS_VISIBLE | WS_CHILD | ES_CENTER,
		mainClientRect.right - contLeft - 160, 10, 20, 20,
		item, NULL, NULL, NULL);

	HWND hours = CreateWindowA("edit", h.c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | ES_CENTER,
		mainClientRect.right - contLeft - 140, 10, 20, 20,
		item, (HMENU)ItemHoursId, NULL, NULL);

	HWND textMinutes = CreateWindowA("static", "m", WS_VISIBLE | WS_CHILD | ES_CENTER,
		mainClientRect.right - contLeft - 115, 10, 20, 20,
		item, NULL, NULL, NULL);

	HWND minutes = CreateWindowA("edit", m.c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | ES_CENTER,
		mainClientRect.right - contLeft - 95, 10, 20, 20,
		item, (HMENU)ItemMinutesId, NULL, NULL);

	HWND textSeconds = CreateWindowA("static", "s", WS_VISIBLE | WS_CHILD | ES_CENTER,
		mainClientRect.right - contLeft - 70, 10, 20, 20,
		item, NULL, NULL, NULL);

	HWND seconds = CreateWindowA("edit", s.c_str(), WS_VISIBLE | WS_CHILD | ES_NUMBER | ES_CENTER,
		mainClientRect.right - contLeft - 50, 10, 20, 20,
		item, (HMENU)ItemSecondsId, NULL, NULL);

	if (listHeight > listContainerHeight) {
		listContainerHeight = listHeight + 2;

		scrInfo.nMax = listContainerHeight;

		SetScrollInfo(hWnd, SB_VERT, &scrInfo, TRUE);
	}
}

BOOL CALLBACK DestroyChildWindowsProc(HWND hwndChild, LPARAM lParam)
{
	HWND itemAppName = GetDlgItem(hwndChild, ItemAppNameId);
	std::string* strAN = (std::string*)GetWindowLongPtr(itemAppName, GWLP_USERDATA);
	delete strAN;

	DestroyWindow(hwndChild); 
	return TRUE; 
}

void RemoveAllChildWindows(HWND hParent)
{
	EnumChildWindows(hParent, DestroyChildWindowsProc, 0);
}

int launchApplication(std::string appName)
{
	STARTUPINFO si = { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION pi;

	int len = MultiByteToWideChar(CP_ACP, 0, appName.c_str(), -1, nullptr, 0);
	if (len == 0) {
		return 1;
	}

	LPWSTR programW = new WCHAR[len];
	MultiByteToWideChar(CP_ACP, 0, appName.c_str(), -1, programW, len);

	if (CreateProcess(nullptr, programW, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	else {
		DWORD error = GetLastError();
		delete[] programW;
		return 1;
	}
	delete[] programW;
	return 0;
}

int launchApplication(LPWSTR appName)
{
	STARTUPINFO si = { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION pi;

	if (CreateProcess(nullptr, appName, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	else {
		return 1;
	}
	return 0;
}

int openFileDialogAdd() {

	wchar_t filePath[MAX_PATH] = { 0 };

	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = nullptr; 
	ofn.lpstrFilter = L"Executable Files (*.exe)\0*.exe\0";
	ofn.lpstrFile = filePath; 
	ofn.nMaxFile = MAX_PATH; 
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST; 
	ofn.lpstrTitle = L"Choose .exe file"; 

	if (GetOpenFileName(&ofn)) {
		std::string tmp = wcharToString_ANSI(filePath);

		size_t pos = tmp.find_last_of("/\\");
		std::string filename =
			(pos != std::string::npos) ? tmp.substr(pos + 1) : tmp;

		//verification if chosen file has extension .exe
		size_t extPos = filename.find_last_of('.');
		if (extPos == std::string::npos || filename.substr(extPos) != ".exe") {
			MessageBoxA(nullptr, "Please select a valid .exe file.",
				"Invalid File", MB_OK | MB_ICONERROR);
			return 1;
		}

		//verification if chosen file is not this program or its launcher
		if (filename == "bigfoot.exe" || filename == "bflauncher.exe")
		{
			MessageBoxA(mainContainer, "Files bigfoot.exe and bflauncher.exe could not be added", 
				"Error", MB_OK | MB_ICONERROR);
			return 1;
		}
		
		tmpAppName = tmp;
		return 0;
	}
	else {
		return 1;
	}
}

int openFileDialogOpen() {

	if (itemsCount > 0 && !isScheduleOpened || 
		itemsCount > 0 && isScheduleOpened && wasScheduleChanged)
	{
		int result = MessageBoxA(
			mainContainer,                      
			"Do you want to save current schedule?",    
			"Confirmation box",   
			MB_YESNO | MB_ICONQUESTION  
		);

		if (result == IDYES)
		{
			SendMessage(mainContainer, WM_COMMAND, OnMenuSaveSch, 0);
		}
	}

	wchar_t filePath[MAX_PATH] = { 0 };

	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = nullptr;
	ofn.lpstrFilter = L"Schdl Files (*.schdl)\0*.schdl\0";
	ofn.lpstrFile = filePath;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.lpstrTitle = L"Select file";

	if (GetOpenFileName(&ofn)) {
		CURRENT_RIGHTS = R_ABSOLUTE;
		SendMessage(mainContainer, WM_COMMAND, OnButtonClear, 0);
		CURRENT_RIGHTS = R_LIMITED;

		std::ifstream inFile(filePath);
		if (inFile.is_open())
		{
			currentSchedule = wcharToString_ANSI(filePath);

			size_t pos = currentSchedule.find_last_of("/\\");
			std::string filename =
				(pos != std::string::npos) ? currentSchedule.substr(pos + 1) : currentSchedule;

			LPWSTR curSchW = stringToWchar_ANSI(filename);

			HWND curSch = GetDlgItem(mainContainer, WndCurSchId);
			SetWindowText(curSch, curSchW);

			std::string line;
			while (std::getline(inFile, line)) {
				processLine(line);
			}

			isScheduleOpened = true;
			wasScheduleChanged = false;
			inFile.close();
		}
		else 
		{
			MessageBoxA(mainContainer, "Couldn't open file", "Error", MB_OK | MB_ICONERROR);
		}

		return 0;
	}
	else {
		return 1;
	}
}

int openFileDialogSave()
{
	if (isScheduleOpened)
	{
		if (wasScheduleChanged) {
			std::ofstream outFile(currentSchedule);

			if (outFile.is_open())
			{
				for (int i = 1; i < itemsCount + 1; i++)
				{
					HWND item = GetDlgItem(listContainer, i);

					HWND itemHours = GetDlgItem(item, ItemHoursId);
					HWND itemMinutes = GetDlgItem(item, ItemMinutesId);
					HWND itemSeconds = GetDlgItem(item, ItemSecondsId);
					HWND itemAppName = GetDlgItem(item, ItemAppNameId);

					char bufferH[256];
					GetWindowTextA(itemHours, bufferH, sizeof(bufferH));
					std::string strH(bufferH);

					char bufferM[256];
					GetWindowTextA(itemMinutes, bufferM, sizeof(bufferM));
					std::string strM(bufferM);

					char bufferS[256];
					GetWindowTextA(itemSeconds, bufferS, sizeof(bufferS));
					std::string strS(bufferS);

					std::string* strAN = (std::string*)GetWindowLongPtr(itemAppName, GWLP_USERDATA);
					//char bufferAN[256];
					//GetWindowTextA(itemAppName, bufferAN, sizeof(bufferAN));
					//std::string strAN(bufferAN);

					outFile << strH + ":" + strM + ":" + strS + "-" + *strAN;
					outFile << "\n";
				}

				wasScheduleChanged = false;
				isScheduleOpened = true;

				outFile.close();
				return 0;
			}
			else {
				MessageBoxA(mainContainer, "Couldn't edit created file", "Error", MB_OK | MB_ICONERROR);
				return 1;
			}
		}
	}
	else
	{
		OPENFILENAME ofn;
		wchar_t filePath[MAX_PATH] = { 0 };

		// extension
		int size = MultiByteToWideChar(CP_UTF8, 0, ".schdl", -1, NULL, 0);
		LPWSTR lpwstrExt = new wchar_t[size];
		MultiByteToWideChar(CP_UTF8, 0, ".schdl", -1, lpwstrExt, size);

		// filestruct
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = NULL;
		ofn.lpstrFile = filePath;
		ofn.nMaxFile = sizeof(filePath);
		ofn.lpstrFilter = L"Files .schdl (*.schdl)\0*.schdl\0";
		ofn.lpstrDefExt = lpwstrExt;
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_OVERWRITEPROMPT;

		if (GetSaveFileName(&ofn)) {

			std::ofstream outFile(filePath);

			if (outFile.is_open())
			{
				currentSchedule = wcharToString_ANSI(filePath);
				size_t pos = currentSchedule.find_last_of("/\\");
				std::string filename =
					(pos != std::string::npos) ? currentSchedule.substr(pos + 1) : currentSchedule;
				LPWSTR curSchW = stringToWchar_ANSI(filename);

				HWND curSch = GetDlgItem(mainContainer, WndCurSchId);
				SetWindowText(curSch, curSchW);

				for (int i = 1; i < itemsCount + 1; i++)
				{
					HWND item = GetDlgItem(listContainer, i);

					HWND itemHours = GetDlgItem(item, ItemHoursId);
					HWND itemMinutes = GetDlgItem(item, ItemMinutesId);
					HWND itemSeconds = GetDlgItem(item, ItemSecondsId);
					HWND itemAppName = GetDlgItem(item, ItemAppNameId);

					char bufferH[256];
					GetWindowTextA(itemHours, bufferH, sizeof(bufferH));
					std::string strH(bufferH);

					char bufferM[256];
					GetWindowTextA(itemMinutes, bufferM, sizeof(bufferM));
					std::string strM(bufferM);

					char bufferS[256];
					GetWindowTextA(itemSeconds, bufferS, sizeof(bufferS));
					std::string strS(bufferS);

					std::string* strAN = (std::string*)GetWindowLongPtr(itemAppName, GWLP_USERDATA);

					outFile << strH + ":" + strM + ":" + strS + "-" + *strAN;
					outFile << "\n";
				}

				wasScheduleChanged = false;
				isScheduleOpened = true;

				outFile.close();
			}
			else {
				MessageBoxA(mainContainer, "Couldn't edit created file", "Error", MB_OK | MB_ICONERROR);
				return 1;
			}

			return 0;
		}
		else {
			return 1;
		}
	}
}

void drawRect(HDC hdc, int x, int y, int width, int height)
{
	// lines
	MoveToEx(hdc, x, y, NULL);
	LineTo(hdc, width, y);

	MoveToEx(hdc, width, y, NULL);
	LineTo(hdc, width, height);

	MoveToEx(hdc, width, height, NULL);
	LineTo(hdc, x, height);

	MoveToEx(hdc, x, height, NULL);
	LineTo(hdc, x, y);
}

// reset flag isItemClicked to false for all items
void clickedItemsRestore()
{
	for (size_t i = 0; i < 100; i++)
	{
		if (isItemClicked[i] == true) {
			isItemClicked[i] = false;
		}
	}
}

// correcting position for all items after deleting an item
void correctPosition(int id)
{
	SCROLLINFO scrInfo;
	scrInfo.cbSize = sizeof(SCROLLINFO);
	scrInfo.fMask = SIF_ALL;
	GetScrollInfo(listContainer, SB_VERT, &scrInfo);

	int x = 2;  
	int y = (id - 1) * 42 + 2 - scrInfo.nPos;  

	for (int i = id + 1; i < itemsCount + 1; i++)
	{
		HWND item = GetDlgItem(listContainer, i);

		SetWindowPos(item, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		SetWindowLongPtr(item, GWLP_ID, i - 1);
		SetWindowLongPtr(item, GWLP_USERDATA, i - 1);

		HWND itemNumber = GetDlgItem(item, ItemNumberId);
		SetWindowTextA(itemNumber, ("№" + std::to_string(i - 1)).c_str());

		y += 42;
	}
	listHeight = 42 * (itemsCount - 1);
}

// correcting time 
void normalizeItemsTime()
{
	for (int i = 1; i < itemsCount + 1; i++)
	{
		HWND item = GetDlgItem(listContainer, i);

		HWND itemHours = GetDlgItem(item, ItemHoursId);
		HWND itemMinutes = GetDlgItem(item, ItemMinutesId);
		HWND itemSeconds = GetDlgItem(item, ItemSecondsId);

		char bufferH[256];
		GetWindowTextA(itemHours, bufferH, sizeof(bufferH));
		std::string strH(bufferH);
		int h;
		if (strH == "")
		{
			h = 0;
		}
		else
		{
			h = std::stoi(strH);
		}

		char bufferM[256];
		GetWindowTextA(itemMinutes, bufferM, sizeof(bufferM));
		std::string strM(bufferM);
		int m;
		if (strM == "")
		{
			m = 0;
		}
		else
		{
			m = std::stoi(strM);
		}
		if (m > 59) {
			m = 59;
		}

		char bufferS[256];
		GetWindowTextA(itemSeconds, bufferS, sizeof(bufferS));
		std::string strS(bufferS);
		int s;
		if (strS == "")
		{
			s = 0;
		}
		else
		{
			s = std::stoi(strS);
		}
		if (s > 59) {
			s = 59;
		}

		SetWindowTextA(itemHours, std::to_string(h).c_str());
		SetWindowTextA(itemMinutes, std::to_string(m).c_str());
		SetWindowTextA(itemSeconds, std::to_string(s).c_str());
	}
}

// line parsing and item restoring
void processLine(const std::string& line) {
	int hours = 0, minutes = 0, seconds = 0;
	std::string filePath;

	std::istringstream stream(line);

	std::string fH;
	std::string fM;
	std::string fS;
	std::string fPath;

	std::getline(stream, fH, ':');
	std::getline(stream, fM, ':');
	std::getline(stream, fS, '-');
	std::getline(stream, fPath);

	ListWndRestoreItem(listContainer, fPath, fH, fM, fS);
}

void serialization()
{
	// get the folder path where this .exe exists
	std::string folderPath = getFolderPath();
	std::string confName = folderPath + "\\config.config";
	std::ofstream oFile(confName);
	if (oFile.is_open())
	{
		
		oFile << std::to_string(launchesCount) + "\n";
		oFile << scheduleInUse + "\n";
		oFile << launcherActivated;

		oFile.close();
	}
	else
	{
		MessageBoxA(mainContainer, "Couldn't find config file. It will be created automatically", 
			"Error", MB_OK | MB_ICONERROR);
	}
}

void deserialization()
{
	// get the folder path where this .exe exists
	std::string folderPath = getFolderPath();

	std::string confName = folderPath + "\\config.config";
	std::ifstream iFile(confName);
	if (iFile.is_open())
	{
		std::string value;

		// amount of launches
		std::getline(iFile, value);
		launchesCount = std::stoi(value);
		launchesCount++;

		// schedule in use
		std::getline(iFile, value);
		scheduleInUse = value;
		currentSchedule = scheduleInUse;

		// is launcher activated or not
		std::getline(iFile, value);
		if (std::stoi(value) == 0)
		{
			launcherActivated = false;
		}
		else
		{
			launcherActivated = true;
		}

		iFile.close();
	}
	else
	{
		MessageBoxA(mainContainer, "Couldn't find config file. It will be created automatically", 
			"Error", MB_OK | MB_ICONERROR);

		launchesCount = 1;
		scheduleInUse = "-";

		std::ofstream oFile(confName);
		if (oFile.is_open())
		{
			oFile << std::to_string(launchesCount) + "\n";
			oFile << scheduleInUse;

			oFile.close();
		}
	}
}

//std::string wcharToString_UTF(LPWSTR text)
//{
//	int size_needed = WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
//	std::vector<char> tmpTextC(size_needed);
//	WideCharToMultiByte(CP_UTF8, 0, text, -1, &tmpTextC[0], size_needed, NULL, NULL);
//	std::string result(tmpTextC.begin(), tmpTextC.end() - 1);
//	return result;
//}

std::string wcharToString_ANSI(LPWSTR text)
{
	int size_needed = WideCharToMultiByte(CP_ACP, 0, text, -1, NULL, 0, NULL, NULL);
	std::vector<char> tmpTextC(size_needed);
	WideCharToMultiByte(CP_ACP, 0, text, -1, &tmpTextC[0], size_needed, NULL, NULL);
	std::string result(tmpTextC.begin(), tmpTextC.end() - 1);
	return result;
}

LPWSTR stringToWchar_ANSI(std::string text)
{
	int size_needed = MultiByteToWideChar(CP_ACP, 0, text.c_str(), -1,
		nullptr, 0);
	LPWSTR lpwstr = new wchar_t[size_needed];
	MultiByteToWideChar(CP_ACP, 0, text.c_str(), -1, lpwstr, size_needed);
	return lpwstr;
}

//LPWSTR stringToWchar_UTF(std::string text)
//{
//	int size_needed = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1,
//		nullptr, 0);
//	LPWSTR lpwstr = new wchar_t[size_needed];
//	MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, lpwstr, size_needed);
//	return lpwstr;
//}

LPCWSTR stringToLPCWSTR_ANSI(const std::string& str) {
	int size_needed = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
	LPWSTR wstr = new WCHAR[size_needed];	
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wstr, size_needed);
	return wstr; 
}

//LPCWSTR stringToLPCWSTR_UTF(const std::string& str) {
//	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
//	LPWSTR wstr = new WCHAR[size_needed];
//	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wstr, size_needed);
//	return wstr;
//}

bool CreateMyService(const std::wstring& serviceName, const std::wstring& exePath) {
	if (ServiceExists(serviceName)) {
		return false; 
	}

	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (hSCManager == NULL) {
		DWORD dwError = GetLastError();
		RequestAdminRights();
		return false;
	}

	SC_HANDLE hService = CreateService(
		hSCManager,                 
		serviceName.c_str(),        
		serviceName.c_str(),        
		SERVICE_ALL_ACCESS,         
		SERVICE_WIN32_OWN_PROCESS,  
		SERVICE_AUTO_START,         
		SERVICE_ERROR_NORMAL,       
		exePath.c_str(),            
		NULL,                       
		NULL,                       
		NULL,                       
		NULL,                       
		NULL                        
	);

	if (hService == NULL) {
		CloseServiceHandle(hSCManager);
		DWORD dwError = GetLastError();
		return false;
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);

	return true;
}

bool DeleteMyService(const std::wstring& serviceName) {
	if (!ServiceExists(serviceName)) {
		return false;
	}

	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (hSCManager == NULL) {
		DWORD dwError = GetLastError();
		RequestAdminRights();
		return false;
	}

	SC_HANDLE hService = OpenService(hSCManager, serviceName.c_str(), DELETE);
	if (hService == NULL) {
		CloseServiceHandle(hSCManager);
		DWORD dwError = GetLastError();
		return false;
	}

	BOOL success = DeleteService(hService);
	if (!success) {
		CloseServiceHandle(hService);
		CloseServiceHandle(hSCManager);
		DWORD dwError = GetLastError();
		return false;
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);

	return true;
}

// writes in config file for launcher schedule in use, return service name
std::string onActivateLauncher()
{
	std::string folderPath = getFolderPath();
	std::string confName = folderPath + "\\configlauncher.config";
	std::string servName = folderPath + "\\bckglauncher.exe";

	std::ifstream iFile(confName);
	if (!iFile.is_open()) {
		std::ofstream oFile(confName);
	}
	else
	{
		iFile.close();
	}
	std::ofstream oFile(confName);
	if (oFile.is_open())
	{
		if (scheduleInUse != "-")
		{
			oFile << scheduleInUse;
		}
		else
		{
			oFile << "";
		}
		oFile.close();
	}

	return servName;
}

void useCurrentScheme()
{
	std::string folderPath = getFolderPath();
	std::string confName = folderPath + "\\configlauncher.config";

	std::ifstream iFile(confName);
	if (!iFile.is_open()) {
		std::ofstream oFile(confName);
	}
	else
	{
		iFile.close();
	}
	std::ofstream oFile(confName);
	if (oFile.is_open())
	{
		if (scheduleInUse != "-")
		{
			oFile << scheduleInUse;
		}
		else
		{
			oFile << "";
		}
		oFile.close();
	}
}

void RequestAdminRights() {
	char exePath[MAX_PATH];
	GetModuleFileNameA(NULL, exePath, MAX_PATH);
	std::string folderPath = std::string(exePath);

	// SHELLEXECUTEINFO
	SHELLEXECUTEINFO shExecInfo;
	ZeroMemory(&shExecInfo, sizeof(SHELLEXECUTEINFO));
	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = SEE_MASK_DEFAULT;
	shExecInfo.hwnd = NULL;
	shExecInfo.lpVerb = L"runas";  
	shExecInfo.lpFile = stringToLPCWSTR_ANSI(folderPath);  
	shExecInfo.lpParameters = NULL;  
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_NORMAL;


	if (!ShellExecuteEx(&shExecInfo)) {
		MessageBox(NULL, L"Couldn't launch bigfoot.exe with administrator rights.", L"Error", MB_OK);
	}
	ExitProcess(0);
}

void createWithAdminRights()
{
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (hSCManager == NULL) {
		DWORD dwError = GetLastError();
		RequestAdminRights();
	}
}

bool ServiceExists(const std::wstring& serviceName) {
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (hSCManager == NULL) {
		return false;
	}

	SC_HANDLE hService = OpenService(hSCManager, serviceName.c_str(), SERVICE_QUERY_STATUS);
	bool exists = (hService != NULL);

	if (hService != NULL) {
		CloseServiceHandle(hService);
	}

	CloseServiceHandle(hSCManager);
	return exists;
}

void testBckgLauncherPerf()
{
	bool shouldStop = false;
	std::string fPath;

	int h = 0;
	int m = 0;
	int s = 0;

	auto start_time = std::chrono::system_clock::now();

	std::string folderPath = getFolderPath();
	std::string confName = folderPath + "\\configlauncher.config";

	// conf file
	std::ifstream confFile(confName);
	if (confFile.is_open())
	{
		std::string schFilePath;
		std::getline(confFile, schFilePath);

		//schedule file
		std::ifstream schFile(schFilePath);
		if (schFile.is_open())
		{
			std::string line;
			while (std::getline(schFile, line)) {
				
				std::istringstream stream(line);

				std::string fH;
				std::string fM;
				std::string fS;

				std::getline(stream, fH, ':');
				std::getline(stream, fM, ':');
				std::getline(stream, fS, '-');
				std::getline(stream, fPath);

				h = std::stoi(fH);
				m = std::stoi(fM);
				s = std::stoi(fS);

				shouldStop = false;
				while (!shouldStop) {
					// current time
					auto current_time = std::chrono::system_clock::now();

					auto target_time =
						start_time +
						std::chrono::seconds(s) +
						std::chrono::minutes(m) +
						std::chrono::hours(h);

					if (current_time >= target_time) {
						launchApplication(fPath);
						shouldStop = true;
					}
				}
			}
			schFile.close();
		}
	}
}

std::string getFolderPath() {
	char exePath[MAX_PATH];
	GetModuleFileNameA(NULL, exePath, MAX_PATH);
	std::string folderPath = std::string(exePath);
	size_t pos = folderPath.find_last_of("\\/");
	if (pos != std::string::npos) {
		folderPath = folderPath.substr(0, pos);
	}
	return folderPath;
}

bool doesExeExist(const std::string& filePath) {
	std::filesystem::path path(filePath);
	return std::filesystem::exists(path) && path.extension() == ".exe";
}

bool isAnyOfItemsClicked()
{
	for (short i = 0; i < 100; i++)
	{
		if (isItemClicked[i] == true) return true;
	}
	return false;
}

void defaultFillClickedItems()
{
	for (short i = 0; i < 100; i++)
	{
		isItemClicked[i] = false;
	}
}

std::string getPathOfSelectedItem()
{
	int id = (int)GetWindowLongPtr(selectedItem, GWLP_USERDATA);
	HWND item = GetDlgItem(listContainer, id);
	HWND itemName = GetDlgItem(item, ItemAppNameId);

	std::string* path = (std::string*)GetWindowLongPtr(itemName, GWLP_USERDATA);

	//char buffer[256];
	//GetWindowTextA(itemName, buffer, sizeof(buffer));
	//std::string path(buffer);
	return *path;
}

void checkItemExistence()
{
	std::string path = getPathOfSelectedItem();

	bool result = doesExeExist(path);
	if (result)
	{
		MessageBoxA(mainContainer, "File with extension .exe was found", "Confirmation box",
			MB_OK);
	}
	else
	{
		MessageBoxA(mainContainer, "Couldn't find .exe file with this path", "Error",
			MB_OK | MB_ICONERROR);
	}
}

void restoreScheduleInUse(HWND hWnd)
{
	LPWSTR filePath = stringToWchar_ANSI(scheduleInUse);

	std::ifstream inFile(filePath);
	if (inFile.is_open())
	{
		std::string line;
		while (std::getline(inFile, line)) {
			processLine(line);
		}

		isScheduleOpened = true;
		wasScheduleChanged = false;
		inFile.close();
	}
	else
	{
		currentSchedule = "-";
		LPWSTR curSchW = stringToWchar_ANSI(currentSchedule);
		HWND curSch = GetDlgItem(hWnd, WndCurSchId);
		SetWindowText(curSch, curSchW);
		MessageBoxA(mainContainer, "Couldn't find file of the schedule in use.", "Error", MB_OK | MB_ICONERROR);
	}
}

bool addLauncherToStartup()
{
	std::string launcherPath = getFolderPath() + "\\bflauncher.exe";

	if (!doesExeExist(launcherPath))
	{
		MessageBoxA(mainContainer, "Couldn't find bflauncher", "Error", MB_OK | MB_ICONERROR);
		return false;
	}

	std::string launcherName = "bflauncher";

	HKEY hKey;
	const std::string regPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

	LONG openRes = RegOpenKeyExA(HKEY_CURRENT_USER, regPath.c_str(), 0, KEY_WRITE, &hKey);
	if (openRes != ERROR_SUCCESS) {
		//std::cerr << "Failed to open registry key!" << std::endl;
		return false;
	}

	LONG setRes = RegSetValueExA(hKey, launcherName.c_str(), 0, REG_SZ, 
		(const BYTE*)launcherPath.c_str(), launcherPath.size() + 1);
	if (setRes != ERROR_SUCCESS) {
		//std::cerr << "Failed to add application to startup!" << std::endl;
		return false;
	}

	return true;
	RegCloseKey(hKey);
}

bool removeLauncherFromStartup()
{
	std::string launcherName = "bflauncher"; 

	HKEY hKey;
	const std::string regPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

	LONG openRes = RegOpenKeyExA(HKEY_CURRENT_USER, regPath.c_str(), 0, KEY_WRITE, &hKey);
	if (openRes != ERROR_SUCCESS) {
		//std::cerr << "Failed to open registry key!" << std::endl;
		return true;
	}

	LONG deleteRes = RegDeleteValueA(hKey, launcherName.c_str());
	if (deleteRes != ERROR_SUCCESS) {
		//std::cerr << "Failed to remove application from startup!" << std::endl;
		return false;
	}

	return true;
	RegCloseKey(hKey);
}

void CreateToolTip(HWND hwnd, std::string tip)
{
	LPWSTR tipText = stringToWchar_ANSI(tip);
	INITCOMMONCONTROLSEX iccex;
	iccex.dwICC = ICC_WIN95_CLASSES;
	iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	InitCommonControlsEx(&iccex);

	HWND hwndTT = CreateWindowExW(WS_EX_TOPMOST,
		L"tooltips_class32",
		NULL,
		WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
		0,
		0,
		0,
		0,
		hwnd,
		NULL,
		NULL,
		NULL);

	SetWindowPos(hwndTT,
		HWND_TOPMOST,
		0,
		0,
		0,
		0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	RECT rect;
	GetClientRect(hwnd, &rect);

	TOOLINFOW ti;
	memset(&ti, 0, sizeof(ti));
	ti.cbSize = sizeof(ti);
	ti.uFlags = TTF_SUBCLASS;
	ti.hwnd = hwnd;
	ti.lpszText = tipText;
	::CopyRect(&ti.rect, &rect);

	if (!SendMessageW(hwndTT, TTM_ADDTOOLW, 0, (LPARAM)(LPTOOLINFOW)&ti))
	{
		DWORD error = GetLastError(); // Получить код ошибки
		int a = 9;
		a++;
	}
}

void sort()
{
	// preparation for comfort sorting
	std::vector<SortTimeItem>sortV(itemsCount);
	for (size_t i = 1; i < itemsCount + 1; ++i) {
		char buffer[256];
		std::string fullTime;

		HWND item = GetDlgItem(listContainer, i);
		HWND itemH = GetDlgItem(item, ItemHoursId);
		HWND itemM = GetDlgItem(item, ItemMinutesId);
		HWND itemS = GetDlgItem(item, ItemSecondsId);
		
		GetWindowTextA(itemS, buffer, sizeof(buffer));
		std::string strS(buffer);
		fullTime += strS.size() == 1 ? "0" + strS : strS;

		GetWindowTextA(itemM, buffer, sizeof(buffer));
		std::string strM(buffer);
		fullTime = (strM.size() == 1 ? "0" + strM : strM) + fullTime;

		GetWindowTextA(itemH, buffer, sizeof(buffer));
		std::string strH(buffer);
		fullTime = (strH.size() == 1 ? "0" + strH : strH) + fullTime;

		SortTimeItem tmp;
		tmp.id = i;
		tmp.time = std::stoi(fullTime);

		sortV[i - 1] = tmp;
	}

	// sort
	for (size_t i = 1; i < sortV.size(); ++i) {
		SortTimeItem key = sortV[i];  
		int j = i - 1;

		while (j >= 0 && sortV[j].time > key.time) {
			sortV[j + 1] = sortV[j];
			--j;
		}

		sortV[j + 1]  = key;
	}

	int top = 2;
	// change items' position
	for (int i = 0; i < sortV.size(); i++)
	{
		HWND item = GetDlgItem(listContainer, sortV[i].id);
		SetWindowPos(item, NULL, 2, top, 0, 0,
			SWP_NOSIZE | SWP_NOZORDER);
		top += 42;

		HWND itemNumber = GetDlgItem(item, ItemNumberId);
		SetWindowTextA(itemNumber, ("№" + std::to_string(i + 1)).c_str());

		// normalize items id
		if (sortV[i].id != i + 1)
		{
			HWND corrItem = GetDlgItem(listContainer, i + 1);
			//SetWindowLongPtr(corrItem, GWLP_ID, (LONG_PTR)(-1));
			SetWindowLongPtr(item, GWLP_ID, (LONG_PTR)(i + 1));
			SetWindowLongPtr(corrItem, GWLP_ID, (LONG_PTR)(sortV[i].id));

			//SetWindowLongPtr(corrItem, GWLP_USERDATA, (LONG_PTR)(-1));
			SetWindowLongPtr(item, GWLP_USERDATA, (LONG_PTR)(i + 1));
			SetWindowLongPtr(corrItem, GWLP_USERDATA, (LONG_PTR)(sortV[i].id));

			// normalize array ids
			SortTimeItem tmpSwap = sortV[i];
			for (int j = 0; j < sortV.size(); j++)
			{
				SortTimeItem tmpSwap1 = sortV[j];
				if (sortV[j].id == i + 1)
				{
					int tmp = sortV[j].id;
					sortV[j].id = sortV[i].id;
					sortV[i].id = tmp;
					break;
				}
			}
		}

		std::string fullTime;
		char buffer[256];

		HWND item1 = GetDlgItem(listContainer, i + 1);
		HWND itemH = GetDlgItem(item1, ItemHoursId);
		HWND itemM = GetDlgItem(item1, ItemMinutesId);
		HWND itemS = GetDlgItem(item1, ItemSecondsId);

		GetWindowTextA(itemS, buffer, sizeof(buffer));
		std::string strS(buffer);
		fullTime += strS.size() == 1 ? "0" + strS : strS;

		GetWindowTextA(itemM, buffer, sizeof(buffer));
		std::string strM(buffer);
		fullTime = (strM.size() == 1 ? "0" + strM : strM) + fullTime;

		GetWindowTextA(itemH, buffer, sizeof(buffer));
		std::string strH(buffer);
		fullTime = (strH.size() == 1 ? "0" + strH : strH) + fullTime;
	}
}