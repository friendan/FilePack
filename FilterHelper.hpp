#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include "toml.hpp"
#include "AppUtil.hpp"
#include "PathUtil.hpp"

class FilterHelper {
private:
    std::vector<std::wstring> m_generalRules;
    bool m_loaded = false;

    void EnsureLoaded() {
        if (m_loaded) return;
        m_loaded = true;

        std::wstring filePath = PathUtil::GetExeDir() + L"\\filter.toml";
        std::string path = AppUtil::WStrToStr(filePath);

        try {
            auto tbl = toml::parse_file(path);

            // 加载通用规则
            auto* generalArr = tbl["filters"]["general"].as_array();
            if (generalArr) {
                for (auto& elem : *generalArr) {
                    std::string val = elem.value_or("");
                    if (!val.empty()) {
                        m_generalRules.push_back(AppUtil::StrToWStr(val));
                    }
                }
            }
        } catch (...) {
        }
    }

public:
    // 获取通用规则
    const std::vector<std::wstring>& GetGeneralRules() {
        EnsureLoaded();
        return m_generalRules;
    }

    // 获取指定文件夹的必需文件/子目录列表
    std::vector<std::wstring> GetRequiredItems(const std::wstring& folderPath) {
        EnsureLoaded();
        std::vector<std::wstring> items;
        // 提取文件夹名
        std::wstring folderName = folderPath;
        size_t pos = folderName.find_last_of(L"\\/");
        if (pos != std::wstring::npos) {
            folderName = folderName.substr(pos + 1);
        }
        std::string path = AppUtil::WStrToStr(PathUtil::GetExeDir() + L"\\filter.toml");
        try {
            auto tbl = toml::parse_file(path);
            std::string folderKey = std::string("folder.") + AppUtil::WStrToStr(folderName);
            // 配置格式示例：["folder.com"]，即 tbl["folder.com"] 为 table
            // 兼容两种格式：
            // 1. tbl["filters"]["folder.xxx"]（[filters] 下的子表）
            // 2. tbl["folder.xxx"]（顶层）
            const auto* arr = tbl["filters"][folderKey]["required"].as_array();
            if (!arr) {
                arr = tbl[folderKey]["required"].as_array();
            }
            if (arr) {
                for (auto& elem : *arr) {
                    std::string val = elem.value_or("");
                    if (!val.empty()) {
                        items.push_back(AppUtil::StrToWStr(val));
                    }
                }
            }
        } catch (...) {
        }
        return items;
    }

    // 验证文件夹是否满足必需条件
    // 返回所有缺少的项目，如果列表为空则表示验证通过
    static std::vector<std::wstring> ValidateRequiredItems(const std::wstring& folderPath, const std::vector<std::wstring>& requiredItems) {
        std::vector<std::wstring> missing;
        for (const auto& item : requiredItems) {
            std::wstring fullPath = folderPath + L"\\" + item;
            if (!std::filesystem::exists(fullPath)) {
                missing.push_back(item);
            }
        }
        return missing;
    }

    // 获取指定文件夹的过滤规则
    std::vector<std::wstring> GetFolderRules(const std::wstring& folderName) {
        EnsureLoaded();
        std::vector<std::wstring> rules;
        std::string path = AppUtil::WStrToStr(PathUtil::GetExeDir() + L"\\filter.toml");
        try {
            auto tbl = toml::parse_file(path);
            auto folderKey = std::string("folder.") + AppUtil::WStrToStr(folderName);
            const auto* arr = tbl["filters"][folderKey]["rules"].as_array();
            if (!arr) {
                arr = tbl[folderKey]["rules"].as_array();
            }
            if (arr) {
                for (auto& elem : *arr) {
                    std::string val = elem.value_or("");
                    if (!val.empty()) {
                        rules.push_back(AppUtil::StrToWStr(val));
                    }
                }
            }
        } catch (...) {
        }
        return rules;
    }

    // 判断路径是否匹配过滤规则（文件夹名 + 通用规则）
    bool IsFiltered(const std::wstring& folderName, const std::wstring& relativePath) {
        EnsureLoaded();

        // 检查通用规则
        for (const auto& rule : m_generalRules) {
            if (MatchesRule(relativePath, rule)) {
                return true;
            }
        }

        // 检查文件夹特定规则
        auto folderRules = GetFolderRules(folderName);
        for (const auto& rule : folderRules) {
            if (MatchesRule(relativePath, rule)) {
                return true;
            }
        }

        return false;
    }

private:
    // 规则匹配：支持精确匹配、前缀匹配、通配符（*号匹配任意内容）
    static bool MatchesRule(const std::wstring& path, const std::wstring& rule) {
        if (rule.empty()) return false;

        // 通配符匹配：*xxx, xxx*, *xxx*, xxx
        size_t starPos = rule.find(L'*');
        if (starPos != std::wstring::npos) {
            std::wstring prefix = rule.substr(0, starPos);
            std::wstring suffix = rule.substr(starPos + 1);

            if (starPos == 0 && suffix.empty()) return true; // 只有 *，匹配所有

            if (starPos == 0) {
                // *xxx：后缀匹配
                size_t p = path.rfind(suffix);
                return p != std::wstring::npos && p + suffix.length() == path.length();
            } else if (suffix.empty()) {
                // xxx*：前缀匹配
                return path.compare(0, prefix.length(), prefix) == 0;
            } else {
                // xxx*yyy：包含前缀和后缀
                if (path.compare(0, prefix.length(), prefix) != 0) return false;
                size_t p = path.rfind(suffix);
                return p != std::wstring::npos && p + suffix.length() == path.length();
            }
        }

        // 精确匹配或前缀匹配（路径以规则开头）
        // 精确匹配
        if (path == rule) return true;

        // 前缀匹配（路径以规则开头，且规则结尾是完整的一段）
        if (path.compare(0, rule.length(), rule) == 0) {
            if (path.length() == rule.length()) return true;
            wchar_t nextChar = path[rule.length()];
            if (nextChar == L'\\' || nextChar == L'/') return true;
        }

        return false;
    }
};
