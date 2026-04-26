#include "AppUtil.hpp"
#include "AppConst.hpp"
#include <cuchar>
#include <stdexcept>
#include <clocale>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <filesystem>
namespace fs = std::filesystem;

std::string AppUtil::WStrToStr(const std::wstring& wstr){
	if (wstr.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], len, nullptr, nullptr);
    if (!result.empty()) result.pop_back();
    return result;
}

std::wstring AppUtil::StrToWStr(const std::string& str){
	if (str.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring result(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], len);
    if (!result.empty()) result.pop_back();
    return result;
}

std::string AppUtil::GetTimeStr(){
    time_t now = time(nullptr);
    tm t{};
    localtime_s(&t, &now); // Windows下安全的本地时间函数
    char buf[32] = {0};
    sprintf_s(buf, "[%04d-%02d-%02d %02d:%02d:%02d]", 
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
             t.tm_hour, t.tm_min, t.tm_sec);
    return buf;
}

static std::mutex g_log_mutex;
static std::string g_log_filename;

// 获取日志文件名（基于exe文件名）
static std::string get_log_filename() {
    if (!g_log_filename.empty()) {
        return g_log_filename;
    }
    
    // 获取exe路径
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    
    // 提取exe目录
    std::wstring wExePath(exePath);
    size_t lastSlash = wExePath.find_last_of(L"/\\");
    std::wstring wExeDir = wExePath.substr(0, lastSlash + 1);
    
    // 提取exe文件名（不含扩展名）
    std::wstring wFileName = wExePath.substr(lastSlash + 1);
    std::wstring wNameWithoutExt = wFileName.substr(0, wFileName.find_last_of(L"."));
    
    // 构建完整的日志文件路径：exe目录 + exe名.log
    std::wstring wLogPath = wExeDir + wNameWithoutExt + L".log";
    
    // 转换为窄字符
    g_log_filename = AppUtil::WStrToStr(wLogPath);
    return g_log_filename;
}

void write_log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_log_mutex); // 自动加锁/解锁
    std::string log_file_name = get_log_filename();
    std::ofstream log_file(log_file_name, std::ios::app | std::ios::out);
    if (log_file.is_open()) {
        log_file << AppUtil::GetTimeStr() << " " << msg << std::endl;
        log_file.close();
    }
}

void AppUtil::SaveLog(const std::string& msg){
    write_log(msg);
}

void AppUtil::SaveLog(const std::wstring& msg){
    write_log(WStrToStr(msg));
}

