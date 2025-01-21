#pragma once
// variables
SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hStatus;

LPWSTR stringToWchar(std::string text);

std::string fPath;

int h = 0;
int m = 0;
int s = 0;

bool shouldStop = false;

// definitions
void processLine(const std::string& line);

int launchApplication(std::string appName);

void ServiceMain(int argc, char** argv);

void ControlHandler(DWORD request);

void logMessage(const std::string& message);

std::string getFolderPath();

std::string getErrorString(DWORD error);

void EnablePrivilege(HANDLE hToken, LPCWSTR privilege);