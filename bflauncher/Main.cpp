#include <windows.h>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Header.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow)
{
	WNDCLASS mainClass = NewWindowClass((HBRUSH)COLOR_WINDOW, LoadCursor(NULL, IDC_ARROW), hInst,
		LoadIcon(NULL, IDI_SHIELD), L"MainWindowClass", SoftwareMainProc);

	MSG SoftwareMainMsg = { 0 };


	// class registration
	if (!RegisterClassW(&mainClass))
	{
		return -1;
	}

	HWND mainContainer = CreateWindow(L"MainWindowClass", L"bflauncher",
		WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		100, 100, 300, 200, NULL, NULL, NULL, NULL);

    ShowWindow(mainContainer, SW_SHOW);    
    ShowWindow(mainContainer, SW_MINIMIZE);

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

	switch (msg)
	{
	case WM_CREATE:
	{
        logic();
		
		PostQuitMessage(0);
		break;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
	}
	default:
		return DefWindowProc(hWnd, msg, wp, lp);
	}
}

//std::string wcharToString(LPWSTR text)
//{
//    int size_needed = WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
//    std::vector<char> tmpTextC(size_needed);
//    WideCharToMultiByte(CP_UTF8, 0, text, -1, &tmpTextC[0], size_needed, NULL, NULL);
//    std::string result(tmpTextC.begin(), tmpTextC.end() - 1);
//    return result;
//}
//
//LPWSTR stringToWchar(std::string text)
//{
//    int size_needed = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1,
//        nullptr, 0);
//    LPWSTR lpwstr = new wchar_t[size_needed];
//    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, lpwstr, size_needed);
//    return lpwstr;
//}

LPCWSTR stringToLPCWSTR_ANSI(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
    LPWSTR wstr = new WCHAR[size_needed];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wstr, size_needed);
    return wstr;
}

LPCWSTR stringToLPCWSTR_UTF(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    LPWSTR wstr = new WCHAR[size_needed];
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wstr, size_needed);
    return wstr;
}

void logic()
{
    logMessage("START SERVICE");
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
                processLine(line);
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
                    Sleep(100);
                }
            }
            schFile.close();
        }
        else
        {
            logMessage("couldn't open schedule file", LOG_ERROR);
        }
    }
    else
    {
        logMessage("couldn't open config file", LOG_ERROR);
    }
    logMessage("EXIT SERVICE");
}

void logMessage(const std::string& message, int type) {
    std::string logTime = "[" + getCurrentTime1() + "]";

    std::string folderPath = getFolderPath();

    std::string logPath = folderPath + "\\log.txt";

    std::ofstream logFile(logPath, std::ios::out | std::ios::app);

    if (logFile.is_open()) {
        switch (type)
        {
        case LOG_INFO:
            logFile << logTime << " " << "[INFO]: " << message << std::endl;
            break;
        case LOG_MSG:
            logFile << logTime << " " << "[MSG]: " << message << std::endl;
            break;
        case LOG_ERROR:
            logFile << logTime << " " << "[ERROR]: " << message << std::endl;
            break;
        default:
            logFile << logTime << " " << "[UNKNOWN]: " << message << std::endl;
            break;
        }
        logFile.close();
    }
}

void logMessage(const std::string& message) {
    std::string logTime ="[" + getCurrentTime1() + "]";
    
    std::string folderPath = getFolderPath();

    std::string logPath = folderPath + "\\log.txt";

    std::ofstream logFile(logPath, std::ios::out | std::ios::app);

    if (logFile.is_open()) {
        logFile << logTime << " " << message << std::endl;
        logFile.close();
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

int launchApplication(std::string appName)
{
    logMessage("launch started", LOG_INFO);
    logMessage("APP NAME : " + appName, LOG_MSG);

    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;

    int len = MultiByteToWideChar(CP_ACP, 0, appName.c_str(), -1, nullptr, 0);
    if (len == 0) {
        logMessage("MultiByteToWideChar", LOG_ERROR);
        logMessage("launch ended", LOG_INFO);
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
        logMessage("Create Process - " + getErrorString(error), LOG_ERROR);
        logMessage("launch ended", LOG_INFO);
        delete[] programW;
        return 1;
    }
    logMessage("APPLICATION SUCCESSFULLY LAUNCHED", LOG_MSG);
    logMessage("launch ended", LOG_INFO);
    delete[] programW;
    return 0;
}

void processLine(const std::string& line) {
    int hours = 0, minutes = 0, seconds = 0;
    std::string filePath;

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
}

std::string getErrorString(DWORD error)
{
    int number = static_cast<int>(error);

    std::stringstream ss;
    ss << number;
    std::string errorString = ss.str();
    return errorString;
}

// a little bit slower than getCurrentTime1 (2672692 < 2643508)
std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now(); // Текущее время
    auto in_time_t = std::chrono::system_clock::to_time_t(now); // Время в секундах
    auto ms = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000; // Микросекунды

    std::tm timeStruct;
    localtime_s(&timeStruct, &in_time_t); // Потокобезопасное преобразование

    std::ostringstream oss;
    oss << std::put_time(&timeStruct, "%Y-%m-%d %H:%M:%S") // Основная часть времени
        << "." << std::setfill('0') << std::setw(6) << ms.count(); // Микросекунды

    return oss.str();
}


std::string getCurrentTime1() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    // Перевод времени в стандартное время системы
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;
    ull.QuadPart -= 116444736000000000ULL; // Сдвиг к Unix epoch
    ull.QuadPart /= 10;                   // Преобразование в микросекунды

    std::time_t seconds = ull.QuadPart / 1000000;
    int microseconds = ull.QuadPart % 1000000;

    std::tm t{};
    if (gmtime_s(&t, &seconds) != 0) {
        throw std::runtime_error("Failed to convert time with gmtime_s");
    }

    std::ostringstream oss;
    oss << std::put_time(&t, "%Y-%m-%d %H:%M:%S") << "."
        << std::setfill('0') << std::setw(6) << microseconds;

    return oss.str();
}