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
    VScrollBar m_scrollBar;
    int m_scrollOffset = 0;
    
public:
    // 双击空白区域的回调函数
    std::function<void()> OnDoubleClickEmpty = nullptr;
    
    // 日志回调函数
    std::function<void(const std::wstring&)> OnLog = nullptr;
    
    void SetFolderPath(const std::wstring& path) {
        m_folderPath = path;
        LoadFilesFromFolder(path);
    }
    
public:
    FileListView(Object* parentObject = NULL) : VLayout(parentObject) {
        Init();
    }
    
    ~FileListView() {
    }
    
    void Init() {
        // 设置自身为垂直布局
        this->SetDockStyle(DockStyle::Fill);
        
        // 直接使用this作为内容布局，不再创建m_contentLayout
        // 添加表头
        HLayout* headerLayout = new HLayout(this);
        this->Add(headerLayout);
        headerLayout->SetFixedHeight(30);
        headerLayout->Style.BackColor = Color(240, 240, 240);  // 浅灰色背景
        
        Label* pathHeader = new Label(headerLayout);
        headerLayout->Add(pathHeader);
        pathHeader->SetText(L"文件路径");
        pathHeader->SetFixedWidth(500);  // 固定宽度
        pathHeader->Style.FontSize = 12;
        pathHeader->Style.ForeColor = Color(0, 0, 0);
        
        Label* sizeHeader = new Label(headerLayout);
        headerLayout->Add(sizeHeader);
        sizeHeader->SetText(L"大小");
        sizeHeader->SetFixedWidth(100);
        sizeHeader->Style.FontSize = 12;
        sizeHeader->Style.ForeColor = Color(0, 0, 0);
        
        Label* timeHeader = new Label(headerLayout);
        headerLayout->Add(timeHeader);
        timeHeader->SetText(L"修改时间");
        timeHeader->SetFixedWidth(150);
        timeHeader->Style.FontSize = 12;
        timeHeader->Style.ForeColor = Color(0, 0, 0);
        
        // 强制刷新布局
        this->RefreshLayout();
        
        if (OnLog) {
            OnLog(L"[FileListView] Init completed, controls count: " + std::to_wstring(this->GetControls().size()));
        }
    }
    
protected:
    // 重写鼠标双击事件
    virtual void OnMouseDoubleClick(const MouseEventArgs& arg) override {
        Control::OnMouseDoubleClick(arg);
        OnItemDoubleClick(arg.Location);
    }
    
    // 重写布局方法，如果不可见则跳过布局以提高性能
    virtual void OnLayout() override {
        // 如果控件不可见或大小为0，跳过布局
        if (!this->IsVisible() || this->Width() == 0 || this->Height() == 0) {
            return;
        }
        VLayout::OnLayout();
    }
    
    // 外部可以调用的选择文件夹方法
    void TriggerSelectFolder() {
        AppUtil::SaveLog("[FileListView] TriggerSelectFolder called");
        if (!m_folderPath.empty()) {
            // 如果已经有文件夹，重新加载
            LoadFilesFromFolder(m_folderPath);
        } else {
            // 否则需要外部调用SelectFolderAndLoad
            AppUtil::SaveLog("[FileListView] No folder path set, waiting for external call");
        }
    }
    
    // LoadFilesFromFolder的实现
    void LoadFilesFromFolder(const std::wstring& folderPath) {
        if (OnLog) OnLog(L"[FileListView] Loading files from: " + folderPath);
        
        // 清除旧内容（保留表头）
        while (this->GetControls().size() > 1) {
            Control* ctl = this->GetControl(1);
            if (ctl) {
                this->Remove(ctl, true);
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
            
            if (OnLog) OnLog(L"[FileListView] Found " + std::to_wstring(fileCount) + L" files");
            
            // 按修改时间从大到小排序
            std::sort(m_files.begin(), m_files.end(), 
                [](const FileInfo& a, const FileInfo& b) {
                    return a.modifyTime > b.modifyTime;
                });
            
            // 添加到列表
            for (const auto& file : m_files) {
                HLayout* itemLayout = new HLayout(this);
                this->Add(itemLayout);
                itemLayout->SetFixedHeight(25);
                itemLayout->Style.BackColor = Color(255, 255, 255);  // 白色背景
                
                Label* pathLabel = new Label(itemLayout);
                itemLayout->Add(pathLabel);
                pathLabel->SetText(file.fullPath.c_str());
                pathLabel->SetFixedWidth(500);  // 固定宽度
                pathLabel->Style.FontSize = 11;
                pathLabel->Style.ForeColor = Color(0, 0, 0);
                
                Label* sizeLabel = new Label(itemLayout);
                itemLayout->Add(sizeLabel);
                sizeLabel->SetText(AppUtil::FormatFileSize(file.fileSize).c_str());
                sizeLabel->SetFixedWidth(100);
                sizeLabel->Style.FontSize = 11;
                sizeLabel->Style.ForeColor = Color(0, 0, 0);
                
                Label* timeLabel = new Label(itemLayout);
                itemLayout->Add(timeLabel);
                timeLabel->SetText(AppUtil::FormatFileTime(file.modifyTime).c_str());
                timeLabel->SetFixedWidth(150);
                timeLabel->Style.FontSize = 11;
                timeLabel->Style.ForeColor = Color(0, 0, 0);
            }
            
            if (OnLog) OnLog(L"[FileListView] Added " + std::to_wstring(m_files.size()) + L" items to list");
            
            // 强制刷新布局
            this->RefreshLayout();
            
            if (OnLog) {
                OnLog(L"[FileListView] This controls count: " + std::to_wstring(this->GetControls().size()));
                OnLog(L"[FileListView] Size: " + std::to_wstring(this->Width()) + L"x" + std::to_wstring(this->Height()));
            }
        }
        catch (const std::exception& e) {
            if (OnLog) OnLog(L"[FileListView] Error loading files: " + std::wstring(e.what(), e.what() + strlen(e.what())));
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
                AppUtil::SaveLog("[FileListView] Double clicked file: ", AppUtil::WStrToStr(file.fullPath));
                
                // 打开文件
                ShellExecuteW(NULL, L"open", file.fullPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
            } else {
                // 双击空白区域，触发回调
                AppUtil::SaveLog("[FileListView] Double clicked empty area");
                if (OnDoubleClickEmpty) {
                    OnDoubleClickEmpty();
                }
            }
        } else {
            // 双击表头或上方空白区域
            AppUtil::SaveLog("[FileListView] Double clicked header or above");
            if (OnDoubleClickEmpty) {
                OnDoubleClickEmpty();
            }
        }
    }
    
    const std::wstring& GetFolderPath() const {
        return m_folderPath;
    }
};
