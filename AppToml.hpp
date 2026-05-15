#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <fstream>
#include "toml.hpp"
#include "AppUtil.hpp"
#include "PathUtil.hpp"

class AppToml {
private:
    std::wstring m_filePath;

    toml::table LoadTable() {
        std::string path = AppUtil::WStrToStr(m_filePath);
        try {
            return toml::parse_file(path);
        } catch (...) {
            return toml::table{};
        }
    }

    void SaveTable(const toml::table& tbl) {
        std::ofstream file(AppUtil::WStrToStr(m_filePath));
        if (file.is_open()) {
            file << tbl << std::endl;
        }
    }

public:
    AppToml() {
        m_filePath = PathUtil::GetExeDir() + L"\\App.toml";
    }

    // 获取所有保存的文件夹路径
    std::vector<std::wstring> GetFolders() {
        auto tbl = LoadTable();
        std::vector<std::wstring> folders;
        auto* arr = tbl["folders"].as_array();
        if (arr) {
            for (auto& elem : *arr) {
                std::string val = elem.value_or("");
                if (!val.empty()) {
                    folders.push_back(AppUtil::StrToWStr(val));
                }
            }
        }
        return folders;
    }

    // 添加文件夹（确保唯一）
    void AddFolder(const std::wstring& path) {
        std::string key = AppUtil::WStrToStr(path);
        auto tbl = LoadTable();
        toml::array arr = tbl["folders"].as_array() ? *tbl["folders"].as_array() : toml::array{};
        // 检查是否已存在
        for (auto& elem : arr) {
            std::string val = elem.value_or("");
            if (val == key) return;
        }
        arr.push_back(key);
        tbl.insert_or_assign("folders", arr);
        SaveTable(tbl);
    }

    // 移除文件夹
    void RemoveFolder(const std::wstring& path) {
        std::string key = AppUtil::WStrToStr(path);
        auto tbl = LoadTable();
        auto* arr = tbl["folders"].as_array();
        if (!arr) return;
        toml::array newArr;
        for (auto& elem : *arr) {
            std::string val = elem.value_or("");
            if (val != key) {
                newArr.push_back(val);
            }
        }
        tbl.insert_or_assign("folders", newArr);
        SaveTable(tbl);
    }
};
