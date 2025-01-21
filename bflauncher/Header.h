#pragma once

#define LOG_INFO	0
#define LOG_MSG		1
#define LOG_ERROR	2

// variables
std::string fPath;

int h = 0;
int m = 0;
int s = 0;

bool shouldStop = false;

// definitions
WNDCLASS NewWindowClass(HBRUSH bgColor, HCURSOR cursor, HINSTANCE hInst, HICON icon, LPCWSTR name,
	WNDPROC procedure);

LRESULT CALLBACK SoftwareMainProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

std::string wcharToString(LPWSTR text);

LPWSTR stringToWchar(std::string text);

LPCWSTR stringToLPCWSTR_ANSI(const std::string& str);

LPCWSTR stringToLPCWSTR_UTF(const std::string& str);

std::string getFolderPath();

void logMessage(const std::string& message);

void logMessage(const std::string& message, int type);

std::string getErrorString(DWORD error);

void processLine(const std::string& line);

int launchApplication(std::string appName);

void logic();

std::string getCurrentTime();
std::string getCurrentTime1();