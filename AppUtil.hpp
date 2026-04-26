#pragma once
#include <Windows.h>
#include <cstdint>  // 必须包含，用于uint8_t
#include <string>
#include <string_view>
#include <sstream>

class AppUtil
{
public:
	static std::string WStrToStr(const std::wstring& wstr);
	static std::wstring StrToWStr(const std::string& str);

	static std::string GetTimeStr();
    static void SaveLog(const std::string& msg);
    static void SaveLog(const std::wstring& msg);

    // template <typename T>
    // void SaveLog(const T& value) {
    //     std::stringstream ss;
    //     ss << value;
    //     SaveLog(ss.str());
    // }

    // 变参模板：支持任意数量、任意类型的参数
    template <typename... Args>
    static void SaveLog(const Args&... args) {
        std::stringstream ss;
        // 折叠表达式：依次将所有参数写入stringstream（C++17+）
        (ss << ... << args);
        // 调用基础重载保存日志
        SaveLog(ss.str());
    }
    
    // 宽字符变参模板（显式指定宽字符参数）
    // 模板参数后缀W，明确区分宽字符版本
    template <typename... Args>
    static void SaveLogW(const Args&... args) {
        std::wstringstream wss; // 改用宽字符流
        (wss << ... << args);   // 折叠表达式支持wstring/int等
        SaveLog(wss.str());     // 调用宽字符基础重载
    }
};
