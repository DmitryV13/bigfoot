#pragma once

#define OnMenuOpenSch		1
#define OnMenuSaveSch		2
#define OnMenuCloseSch		3

#define OnMenuExit			40

#define OnButtonAdd			60
#define OnButtonClear		61
#define OnButtonEdit		62
#define OnButtonCheck		63
#define OnButtonSort		64
#define OnButtonDelete		65
#define OnButtonLaunch		66
#define OnButtonActLchr		67
#define OnButtonClrSchU		68
#define OnButtonUseCurSch	69

#define ItemLaunchId		100
#define ItemNumberId		101
#define ItemHoursId			102
#define ItemMinutesId		103
#define ItemSecondsId		104
#define ItemAppNameId		105

#define WndCurSchId			130
#define WndUseSchId			131

#define R_ABSOLUTE			150
#define R_LIMITED			151

#define S_ONLAUNCH			160
#define S_INPROGRESS		161


HWND modeBtn;
HWND launchMode;
HWND mainContainer;
HWND listContainer;

HBRUSH brushGrey = CreateSolidBrush(RGB(169, 169, 169));
HBRUSH brushBlue = CreateSolidBrush(RGB(26, 56, 96));
HBRUSH brushBackground;

RECT paintRect;
PAINTSTRUCT ps;

// flags
bool isEditMode = false;

bool isItemHovered = false;
bool isItemClicked[100] = {};

int CURRENT_RIGHTS = R_LIMITED;
int CURRENT_STATE;

// other
int maxItems = 99;
int itemsCount = 0;

std::string tmpAppName;

HWND selectedItem;

bool wasScheduleChanged = false;
bool isScheduleOpened = false;
std::string currentSchedule = "-";

struct SortTimeItem {
	int id;
	int time;
};

// boundaries
RECT mainClientRect;
RECT listClientRect;

int contLeft;
int contTop;
int itemWidth;
int itemHeight;
int listHeight = 0;
int listContainerHeight = 0;

// serialization
int launchesCount;
std::string scheduleInUse;
bool launcherActivated;

// definitions
LRESULT CALLBACK SoftwareMainProc(HWND hWnd, UINT msg, WPARAM, LPARAM lp);

LRESULT CALLBACK SoftwareListProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

LRESULT CALLBACK SoftwareItemProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

WNDCLASS NewWindowClass(HBRUSH bgColor, HCURSOR cursor, HINSTANCE hInst, HICON icon, LPCWSTR name,
	WNDPROC procedure);

BOOL CALLBACK DestroyChildWindowsProc(HWND hwndChild, LPARAM lParam);

bool CreateMyService(const std::wstring& serviceName, const std::wstring& exePath);

bool DeleteMyService(const std::wstring& serviceName);

std::string onActivateLauncher();

void RemoveAllChildWindows(HWND hParent);

void MainWndAddMenu(HWND hWnd);

void MainWndAddWidget(HWND hWnd);

void ListWndAddItem(HWND hWnd);

void ListWndRestoreItem(HWND hWnd, std::string appN, std::string h, std::string m, std::string s);

int launchApplication(std::string appName);

int launchApplication(LPWSTR appName);

int openFileDialogAdd();

int openFileDialogOpen();

int openFileDialogSave();

void drawRect(HDC hdc, int x, int y, int width, int height);

void clickedItemsRestore();

void correctPosition(int id);

void normalizeItemsTime();

void processLine(const std::string& line);

void serialization();

void deserialization();

std::string wcharToString_UTF(LPWSTR text);
std::string wcharToString_ANSI(LPWSTR text);

LPWSTR stringToWchar_UTF(std::string text);
LPWSTR stringToWchar_ANSI(std::string text);

LPCWSTR stringToLPCWSTR_UTF(const std::string& str);
LPCWSTR stringToLPCWSTR_ANSI(const std::string& str);

void RequestAdminRights();

void createWithAdminRights();

bool ServiceExists(const std::wstring& serviceName);

void testBckgLauncherPerf();

std::string getFolderPath();

bool doesExeExist(const std::string& filePath);

bool isAnyOfItemsClicked();

void checkItemExistence();

std::string getPathOfSelectedItem();

void useCurrentScheme();

void restoreScheduleInUse(HWND hWnd);

bool addLauncherToStartup();

bool removeLauncherFromStartup();

void CreateToolTip(HWND hwnd, std::string tip);

void sort();

void defaultFillClickedItems();

//-------------

