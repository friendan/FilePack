#pragma once
#include <Windows.h>
#include <VLayout.h>
#include <Label.h>
#include <HLayout.h>
#include <VScrollBar.h>
#include "AppUtil.hpp"
#include <algorithm>
#include <functional>
#include <shellapi.h>

using namespace ezui;

class FileListView : public VLayout {
private:
    std::vector<FileInfo> m_files;
    std::wstring m_folderPath;
    VLayout* m_contentLayout;
    VScrollBar m_scrollBar;
    int m_scrollOffset = 0;
    
public:
    FileListView(Object* parentObject = NULL) : VLayout(parentObject) {
        Init();
    }
    
    ~FileListView() {
    }
    
    void Init() {
        // 设置自身为垂直布局
        this->SetDockStyle(DockStyle::Fill);
        
        // 创建内容布局
        m_contentLayout = new VLayout(this);
        m_contentLayout->SetDockStyle(DockStyle::Fill);
        
        // 添加表头
        HLayout* headerLayout = new HLayout(m_contentLayout);
        headerLayout->SetFixedHeight(30);
        headerLayout->Style.BackColor = Color(240, 240, 240);  // 浅灰色背景
        
        Label* pathHeader = new Label(headerLayout);
        pathHeader->SetText(L"文件路径");
        pathHeader->SetRateWidth(1.0f);
        pathHeader->Style.FontSize = 12;
        pathHeader->Style.ForeColor = Color(0, 0, 0);
        headerLayout->Add(pathHeader);
        
        Label* sizeHeader = new Label(headerLayout);
        sizeHeader->SetText(L"大小");
        sizeHeader->SetFixedWidth(100);
        sizeHeader->Style.FontSize = 12;
        sizeHeader->Style.ForeColor = Color(0, 0, 0);
        headerLayout->Add(sizeHeader);
        
        Label* timeHeader = new Label(headerLayout);
        timeHeader->SetText(L"修改时间");
        timeHeader->SetFixedWidth(150);
        timeHeader->Style.FontSize = 12;
        timeHeader->Style.ForeColor = Color(0, 0, 0);
        headerLayout->Add(timeHeader);
        
        // 启用双击事件
        this->EventHandler = [this](Control* sender, EventArgs& args) {
            if (args.EventType == Event::OnMouseDoubleClick) {
                MouseEventArgs& mouseArgs = static_cast<MouseEventArgs&>(args);
                OnItemDoubleClick(mouseArgs.Location);
            }
        };
        
        // 强制刷新布局
        this->RefreshLayout();
    }
    
    void SetFolderPath(const std::wstring& path) {
        m_folderPath = path;
        LoadFilesFromFolder(path);
    }
    
    void LoadFilesFromFolder(const std::wstring& folderPath) {
        AppUtil::SaveLog("[FileListView] Loading files from: ", AppUtil::WStrToStr(folderPath));
        
        // 清除旧内容（保留表头）
        while (m_contentLayout->GetControls().size() > 1) {
            Control* ctl = m_contentLayout->GetControl(1);
            if (ctl) {
                m_contentLayout->Remove(ctl, true);
            }
        }
        m_files.clear();
        
        try {
            // 递归遍历文件夹
            int fileCount = 0;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(folderPath)) {
                if (entry.is_regular_file()) {
                    FileInfo info;
                    info.fullPath = entry.path().wstring();
                    info.fileSize = entry.file_size();
                    info.modifyTime = entry.last_write_time();
                    m_files.push_back(info);
                    fileCount++;
                }
            }
            
            AppUtil::SaveLog("[FileListView] Found ", fileCount, " files");
            
            // 按修改时间从大到小排序
            std::sort(m_files.begin(), m_files.end(), 
                [](const FileInfo& a, const FileInfo& b) {
                    return a.modifyTime > b.modifyTime;
                });
            
            // 添加到列表
            for (const auto& file : m_files) {
                HLayout* itemLayout = new HLayout(m_contentLayout);
                itemLayout->SetFixedHeight(25);
                itemLayout->Style.BackColor = Color(255, 255, 255);  // 白色背景
                
                Label* pathLabel = new Label(itemLayout);
                pathLabel->SetText(file.fullPath.c_str());
                pathLabel->SetRateWidth(1.0f);
                pathLabel->Style.FontSize = 11;
                pathLabel->Style.ForeColor = Color(0, 0, 0);
                itemLayout->Add(pathLabel);
                
                Label* sizeLabel = new Label(itemLayout);
                sizeLabel->SetText(AppUtil::FormatFileSize(file.fileSize).c_str());
                sizeLabel->SetFixedWidth(100);
                sizeLabel->Style.FontSize = 11;
                sizeLabel->Style.ForeColor = Color(0, 0, 0);
                itemLayout->Add(sizeLabel);
                
                Label* timeLabel = new Label(itemLayout);
                timeLabel->SetText(AppUtil::FormatFileTime(file.modifyTime).c_str());
                timeLabel->SetFixedWidth(150);
                timeLabel->Style.FontSize = 11;
                timeLabel->Style.ForeColor = Color(0, 0, 0);
                itemLayout->Add(timeLabel);
                
                m_contentLayout->Add(itemLayout);
            }
            
            AppUtil::SaveLog("[FileListView] Added ", m_files.size(), " items to list");
            
            // 强制刷新布局
            m_contentLayout->RefreshLayout();
            this->RefreshLayout();
        }
        catch (const std::exception& e) {
            AppUtil::SaveLog("[FileListView] Error loading files: ", e.what());
        }
    }
    
    void OnItemDoubleClick(const Point& point) {
        // 处理双击事件，可以打开文件或执行其他操作
        // 简单实现：计算点击的是哪一行
        int headerHeight = 30;
        int itemHeight = 25;
        int relativeY = point.Y - headerHeight + m_scrollOffset;
        
        if (relativeY >= 0) {
            int itemIndex = relativeY / itemHeight;
            if (itemIndex >= 0 && itemIndex < (int)m_files.size()) {
                const FileInfo& file = m_files[itemIndex];
                AppUtil::SaveLog("[FileListView] Double clicked: ", AppUtil::WStrToStr(file.fullPath));
                
                // 打开文件
                ShellExecuteW(NULL, L"open", file.fullPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
            }
        }
    }
    
    const std::wstring& GetFolderPath() const {
        return m_folderPath;
    }
};
