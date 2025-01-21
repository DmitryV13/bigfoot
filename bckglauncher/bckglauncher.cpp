#include <windows.h>
#include <WtsApi32.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include "definitions.h"
#pragma comment(lib, "wtsapi32.lib")

int main() {    
    logMessage("START SERVICE");
    //logFile << "opened\n";

    std::string tmp("bckglauncher");
    LPWSTR serv = stringToWchar(tmp);

    SERVICE_TABLE_ENTRY ServiceTable[] = {
        {serv, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL}
    };

    


    if (!StartServiceCtrlDispatcher(ServiceTable)) {
        DWORD error = GetLastError();
        logMessage("--error : dispatcher");
        //logFile << "error"<< error << " \n";
        //logFile.close();
        return 1;
    }

    //logFile.close();
    return 0;
}

void ServiceMain(int argc, char** argv) {
    ServiceStatus.dwServiceType = SERVICE_WIN32;
    ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 0;
    ServiceStatus.dwWaitHint = 0;

    std::string tmp("bckglauncher");
    LPWSTR serv = stringToWchar(tmp);
    hStatus = RegisterServiceCtrlHandler(serv, (LPHANDLER_FUNCTION)ControlHandler);
    if (hStatus == (SERVICE_STATUS_HANDLE)0) {
        return;
    }

    ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(hStatus, &ServiceStatus);

    auto start_time = std::chrono::system_clock::now();

    // main service
    while (ServiceStatus.dwCurrentState == SERVICE_RUNNING) {  

        // get the folder path where this .exe exists
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
                        Sleep(1000);
                    }
                }
                schFile.close();
            }
        }
        logMessage("EXIT SERVICE");
        ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(hStatus, &ServiceStatus);
    }

    return;
}

void ControlHandler(DWORD request) {
    switch (request) {
    case SERVICE_CONTROL_STOP:
        ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(hStatus, &ServiceStatus);
        return;
    case SERVICE_CONTROL_SHUTDOWN:
        ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(hStatus, &ServiceStatus);
        return;
    default:
        break;
    }

    SetServiceStatus(hStatus, &ServiceStatus);
    return;
}


LPWSTR stringToWchar(std::string text)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1,
        nullptr, 0);
    LPWSTR lpwstr = new wchar_t[size_needed];
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, lpwstr, size_needed);
    return lpwstr;
}

int launchApplication(std::string appName)
{
    logMessage("--launch started");
    logMessage("--app name : " + appName);

    HANDLE hToken = NULL;

    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;

    // Получаем токен текущего пользователя
    if (!WTSQueryUserToken(WTSGetActiveConsoleSessionId(), &hToken)) {
        DWORD error = GetLastError();

        logMessage("--error : token : " + getErrorString(error));
        return false;
    }

    EnablePrivilege(hToken, SE_ASSIGNPRIMARYTOKEN_NAME);
    EnablePrivilege(hToken, SE_INCREASE_QUOTA_NAME);
  
    int len = MultiByteToWideChar(CP_ACP, 0, appName.c_str(), -1, nullptr, 0);
    if (len == 0) {
        logMessage("--error 1 : len = 0");
        return 1;
    }

    LPWSTR programW = new WCHAR[len];
    MultiByteToWideChar(CP_ACP, 0, appName.c_str(), -1, programW, len);


    if (!CreateProcessAsUser(
        hToken,                 // Токен пользователя
        programW,               // Имя приложения
        NULL,                   // Аргументы
        NULL,                   // Атрибуты безопасности процесса
        NULL,                   // Атрибуты безопасности потока
        FALSE,                  // Унаследовать дескрипторы
        CREATE_NEW_CONSOLE,     // Флаги создания
        NULL,                   // Среда
        NULL,                   // Текущий каталог
        &si,                    // Информация о старте
        &pi                     // Информация о процессе
    ))
    {
    //if (CreateProcess(
    //    nullptr, 
    //    programW, 
    //    nullptr, 
    //    nullptr, 
    //    FALSE, 
    //    0, 
    //    nullptr, 
    //    nullptr, 
    //    &si, 
    //    &pi)) 
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else {
        DWORD error = GetLastError();

        logMessage("--error2 : " + getErrorString(error));
        logMessage("--launch ended");

        delete[] programW;
        return 1;
    }
    DWORD error = GetLastError();

    logMessage("--error3 : " + getErrorString(error));
    logMessage("--launch ended");

    delete[] programW;
    return 0;
}

void EnablePrivilege(HANDLE hToken, LPCWSTR privilege) {
    TOKEN_PRIVILEGES tp;
    LUID luid;
    if (LookupPrivilegeValue(nullptr, privilege, &luid)) {
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr);
    }
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

void logMessage(const std::string& message) {
    std::string folderPath = getFolderPath();
    
    std::string logPath = folderPath + "\\log.txt";

    std::ofstream logFile(logPath, std::ios::out | std::ios::app);

    if (logFile.is_open()) {
        logFile << message << std::endl;
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

std::string getErrorString(DWORD error)
{
    int number = static_cast<int>(error); 

    std::stringstream ss;
    ss << number;
    std::string errorString = ss.str();
    return errorString;
}