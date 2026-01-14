#pragma once
#include "log_types.hpp"
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <set>
#include <algorithm>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "user32.lib")

// colecteaza doar aplicatiile cu ferestre (Apps din Task Manager)


class ProcessCollector {
public:
    static std::string get_hostname() {
        char hostname[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = sizeof(hostname);
        if (GetComputerNameA(hostname, &size)) {
            return std::string(hostname);
        }
        return "unknown";
    }

private:
    static std::string get_username_from_process(HANDLE hProcess) {
        HANDLE hToken = NULL;
        if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
            return "SYSTEM";
        }

        DWORD dwSize = 0;
        GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);

        if (dwSize == 0) {
            CloseHandle(hToken);
            return "SYSTEM";
        }

        PTOKEN_USER pTokenUser = static_cast<PTOKEN_USER>(malloc(dwSize));
        if (!pTokenUser) {
            CloseHandle(hToken);
            return "SYSTEM";
        }

        if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize)) {
            free(pTokenUser);
            CloseHandle(hToken);
            return "SYSTEM";
        }

        char name[256];
        char domain[256];
        DWORD nameSize = sizeof(name);
        DWORD domainSize = sizeof(domain);
        SID_NAME_USE sidType;

        std::string result = "SYSTEM";
        if (LookupAccountSidA(NULL, pTokenUser->User.Sid, name, &nameSize,
            domain, &domainSize, &sidType)) {
            result = std::string(name);
        }

        free(pTokenUser);
        CloseHandle(hToken);
        return result;
    }

    static std::wstring string_to_wstring(const std::string& str) {
        if (str.empty()) return std::wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()), &wstrTo[0], size_needed);
        return wstrTo;
    }

    static std::string wstring_to_string(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }

    struct CPUTimes {
        ULONGLONG kernelTime;
        ULONGLONG userTime;
        ULONGLONG timestamp;
    };

    static std::map<DWORD, CPUTimes> previousCPUTimes;

    static double calculate_cpu_percent(DWORD pid, HANDLE hProcess) {
        FILETIME creationTime, exitTime, kernelTime, userTime;

        if (!GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime)) {
            return 0.0;
        }

        ULONGLONG kernel = (static_cast<ULONGLONG>(kernelTime.dwHighDateTime) << 32) | kernelTime.dwLowDateTime;
        ULONGLONG user = (static_cast<ULONGLONG>(userTime.dwHighDateTime) << 32) | userTime.dwLowDateTime;
        ULONGLONG currentTime = GetTickCount64();

        auto it = previousCPUTimes.find(pid);
        if (it == previousCPUTimes.end()) {
            previousCPUTimes[pid] = { kernel, user, currentTime };
            return 0.0;
        }

        ULONGLONG kernelDiff = kernel - it->second.kernelTime;
        ULONGLONG userDiff = user - it->second.userTime;
        ULONGLONG timeDiff = currentTime - it->second.timestamp;

        it->second = { kernel, user, currentTime };

        if (timeDiff == 0) return 0.0;

        double cpuPercent = ((kernelDiff + userDiff) / 10000.0) / timeDiff * 100.0;

        if (cpuPercent > 100.0) cpuPercent = 100.0;

        return cpuPercent;
    }

    static bool has_visible_window(DWORD pid) {
        struct EnumData {
            DWORD pid;
            bool found;
        };

        EnumData data = { pid, false };

        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            EnumData* pData = reinterpret_cast<EnumData*>(lParam);

            DWORD windowPid = 0;
            GetWindowThreadProcessId(hwnd, &windowPid);

            if (windowPid != pData->pid) {
                return TRUE; 
            }

            if (!IsWindowVisible(hwnd)) {
                return TRUE;
            }

            if (IsIconic(hwnd)) {
                return TRUE;
            }

            LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
            if (exStyle & WS_EX_TOOLWINDOW) {
                return TRUE;
            }

            char title[256];
            if (GetWindowTextA(hwnd, title, sizeof(title)) > 0) {
                pData->found = true;
                return FALSE; 
            }

            return TRUE;
            }, reinterpret_cast<LPARAM>(&data));

        return data.found;
    }

    static bool is_uwp_app(HANDLE hProcess) {
        HANDLE hToken = NULL;
        if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
            return false;
        }

        DWORD dwSize = 0;
        BOOL isAppContainer = FALSE;

        if (GetTokenInformation(hToken, TokenIsAppContainer, &isAppContainer,
            sizeof(isAppContainer), &dwSize)) {
            CloseHandle(hToken);
            return isAppContainer == TRUE;
        }

        CloseHandle(hToken);
        return false;
    }

public:
    static std::vector<ProcessInfo> collect_app_processes() {
        std::vector<ProcessInfo> app_processes;
        std::time_t current_timestamp = std::time(nullptr);

        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Nu se poate crea snapshot procese");
        }

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (!Process32FirstW(hSnapshot, &pe32)) {
            CloseHandle(hSnapshot);
            throw std::runtime_error("Nu se poate obtine primul proces");
        }

        do {
            DWORD pid = pe32.th32ProcessID;

            if (pid == 0 || pid == 4) {
                continue;
            }

            if (!has_visible_window(pid)) {
                continue;
            }

            ProcessInfo proc_info;
            proc_info.pid = static_cast<int>(pid);
            proc_info.name = wstring_to_string(pe32.szExeFile);
            proc_info.timestamp = current_timestamp;

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                FALSE, pid);

            if (hProcess != NULL) {
                PROCESS_MEMORY_COUNTERS_EX pmc;
                if (GetProcessMemoryInfo(hProcess,
                    reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
                    proc_info.memory_kb = pmc.WorkingSetSize / 1024;
                }

                proc_info.cpu_percent = calculate_cpu_percent(pid, hProcess);
                proc_info.user = get_username_from_process(hProcess);
                proc_info.status = ProcessStatus::RUNNING;

                CloseHandle(hProcess);
            }
            else {
                proc_info.memory_kb = 0;
                proc_info.cpu_percent = 0.0;
                proc_info.user = "SYSTEM";
                proc_info.status = ProcessStatus::RUNNING;
            }

            proc_info.log_level = proc_info.get_log_level();
            app_processes.push_back(proc_info);

        } while (Process32NextW(hSnapshot, &pe32));

        CloseHandle(hSnapshot);
        return app_processes;
    }

    // functie de colectat procese
    static std::vector<ProcessInfo> collect_processes() {
        std::vector<ProcessInfo> processes;
        std::time_t current_timestamp = std::time(nullptr);

        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Nu se poate crea snapshot procese");
        }

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (!Process32FirstW(hSnapshot, &pe32)) {
            CloseHandle(hSnapshot);
            throw std::runtime_error("Nu se poate obtine primul proces");
        }

        do {
            ProcessInfo proc_info;
            proc_info.pid = static_cast<int>(pe32.th32ProcessID);
            proc_info.name = wstring_to_string(pe32.szExeFile);
            proc_info.timestamp = current_timestamp;

            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                FALSE, pe32.th32ProcessID);

            if (hProcess != NULL) {
                PROCESS_MEMORY_COUNTERS_EX pmc;
                if (GetProcessMemoryInfo(hProcess,
                    reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
                    proc_info.memory_kb = pmc.WorkingSetSize / 1024;
                }

                proc_info.cpu_percent = calculate_cpu_percent(pe32.th32ProcessID, hProcess);
                proc_info.user = get_username_from_process(hProcess);
                proc_info.status = ProcessStatus::RUNNING;

                CloseHandle(hProcess);
            }
            else {
                proc_info.memory_kb = 0;
                proc_info.cpu_percent = 0.0;
                proc_info.user = "SYSTEM";
                proc_info.status = ProcessStatus::RUNNING;
            }

            proc_info.log_level = proc_info.get_log_level();
            processes.push_back(proc_info);

        } while (Process32NextW(hSnapshot, &pe32));

        CloseHandle(hSnapshot);
        return processes;
    }

    static ProcessSnapshot collect_snapshot() {
        ProcessSnapshot snapshot;
        snapshot.hostname = get_hostname();
        snapshot.timestamp = std::time(nullptr);
        snapshot.processes = collect_processes();
        return snapshot;
    }

    static void cleanup_cpu_cache(const std::vector<ProcessInfo>& current_processes) {
        std::set<DWORD> current_pids;
        for (const auto& proc : current_processes) {
            current_pids.insert(static_cast<DWORD>(proc.pid));
        }

        auto it = previousCPUTimes.begin();
        while (it != previousCPUTimes.end()) {
            if (current_pids.find(it->first) == current_pids.end()) {
                it = previousCPUTimes.erase(it);
            }
            else {
                ++it;
            }
        }
    }
};

std::map<DWORD, ProcessCollector::CPUTimes> ProcessCollector::previousCPUTimes;