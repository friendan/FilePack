#pragma once
#include <Windows.h>
#include <VLayout.h>
#include <Label.h>
#include <HLayout.h>
#include <VScrollBar.h>
#include <UIManager.h>
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
    int m_headerHeight = 30;
    int m_itemHeight = 25;
    std::vector<HLayout*> m_itemLayouts;
    HLayout* m_headerLayout = nullptr;
    VLayout* m_contentLayout = nullptr;
    UIManager m_ui;
    Size m_cachedContentSize;
    
public:
    std::function<void()> OnDoubleClickEmpty = nullptr;
    std::function<void(const std::wstring&)> OnLog = nullptr;

    virtual const Size& GetContentSize() override {
        m_cachedContentSize.Width = Width();
        m_cachedContentSize.Height = (int)m_itemLayouts.size() * m_itemHeight;
        return m_cachedContentSize;
    }

    virtual ScrollBar* GetScrollBar() override {
        return &m_scrollBar;
    }
    
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
        this->SetDockStyle(DockStyle::Fill);
        this->EventPassThrough = Event::OnMouseDoubleClick;
        
        m_scrollBar.Parent = this;
        m_scrollBar.SetFixedWidth(14);
        m_scrollBar.OffsetCallback = [this](int offset) {
            m_scrollOffset = offset;
            OffsetItems(offset);
            this->Invalidate();
        };
        
        LoadXmlLayout();
        
        m_headerLayout = (HLayout*)this->FindControl("header");
        m_contentLayout = (VLayout*)this->FindControl("content");
        
        if (m_contentLayout) {
            m_contentLayout->EventPassThrough = Event::OnMouseDoubleClick;
            m_contentLayout->EventHandler = [this](Control* sender, EventArgs& args) {
                if (args.EventType == Event::OnMouseDoubleClick) {
                    MouseEventArgs& mouseArgs = (MouseEventArgs&)args;
                    OnItemDoubleClick(mouseArgs.Location);
                }
            };
        }
        
        this->Invalidate();
    }
    
    void LoadXmlLayout() {
        HRSRC hRsrc = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_FILELISTVIEW_LAYOUT), RT_HTML);
        if (!hRsrc) {
            hRsrc = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_FILELISTVIEW_LAYOUT), L"HTML");
        }
        
        if (hRsrc) {
            HGLOBAL hGlobal = LoadResource(NULL, hRsrc);
            if (hGlobal) {
                DWORD size = SizeofResource(NULL, hRsrc);
                const char* xmlData = (const char*)LockResource(hGlobal);
                if (xmlData && size > 0) {
                    std::wstring xmlContent;
                    int wideLen = MultiByteToWideChar(CP_UTF8, 0, xmlData, size, NULL, 0);
                    if (wideLen > 0) {
                        xmlContent.resize(wideLen);
                        MultiByteToWideChar(CP_UTF8, 0, xmlData, size, &xmlContent[0], wideLen);
                    }
                    
                    m_ui.LoadXmlData(xmlContent.c_str());
                    m_ui.SetupUI(this);
                }
            }
        }
    }
    
    void OffsetItems(int offset) {
        for (size_t i = 0; i < m_itemLayouts.size(); i++) {
            int y = i * m_itemHeight + offset;
            m_itemLayouts[i]->SetY(y);
        }
        this->Invalidate();
    }
    
protected:
    virtual void OnChildPaint(PaintEventArgs& args) override {
        VLayout::OnChildPaint(args);
    }
    
    virtual void OnMouseDoubleClick(const MouseEventArgs& arg) override {
        OnItemDoubleClick(arg.Location);
    }
    
    virtual void OnLayout() override {
        VLayout::OnLayout();
        m_scrollBar.RefreshScroll();
    }
    
public:
    void TriggerSelectFolder() {
        if (!m_folderPath.empty()) {
            LoadFilesFromFolder(m_folderPath);
        }
    }
    
    void LoadFilesFromFolder(const std::wstring& folderPath) {
        if (OnLog) OnLog(L"[FileListView] Loading: " + folderPath);
        
        for (auto item : m_itemLayouts) {
            m_contentLayout->Remove(item, true);
        }
        m_itemLayouts.clear();
        m_files.clear();
        
        try {
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
            
            if (OnLog) OnLog(L"[FileListView] Found: " + std::to_wstring(fileCount) + L" files");
            
            std::sort(m_files.begin(), m_files.end(), 
                [](const FileInfo& a, const FileInfo& b) {
                    return a.modifyTime > b.modifyTime;
                });
            
            for (size_t i = 0; i < m_files.size(); i++) {
                const auto& file = m_files[i];
                HLayout* itemLayout = new HLayout(m_contentLayout);
                m_contentLayout->Add(itemLayout);
                itemLayout->SetFixedHeight(m_itemHeight);
                // 奇偶行不同颜色
                itemLayout->Style.BackColor = (i % 2 == 0) ? Color(255, 255, 255) : Color(248, 248, 248);
                m_itemLayouts.push_back(itemLayout);
                
                Label* indexLabel = new Label(itemLayout);
                itemLayout->Add(indexLabel);
                indexLabel->SetText((L"#" + std::to_wstring(i + 1)).c_str());
                indexLabel->SetFixedWidth(60);
                indexLabel->TextAlign = TextAlign::MiddleCenter;
                
                Label* pathLabel = new Label(itemLayout);
                itemLayout->Add(pathLabel);
                pathLabel->SetText(file.fullPath.c_str());
                pathLabel->SetFixedWidth(500);
                
                Label* sizeLabel = new Label(itemLayout);
                itemLayout->Add(sizeLabel);
                sizeLabel->SetText(AppUtil::FormatFileSize(file.fileSize).c_str());
                sizeLabel->SetFixedWidth(100);
                
                Label* timeLabel = new Label(itemLayout);
                itemLayout->Add(timeLabel);
                timeLabel->SetText(AppUtil::FormatFileTime(file.modifyTime).c_str());
                timeLabel->SetFixedWidth(150);
            }
            
            if (OnLog) OnLog(L"[FileListView] Added: " + std::to_wstring(m_itemLayouts.size()) + L" items");
            
            this->RefreshLayout();
            m_scrollOffset = 0;
        }
        catch (const std::exception& e) {
            if (OnLog) OnLog(L"[FileListView] Error: " + std::wstring(e.what(), e.what() + strlen(e.what())));
        }
    }
    
    void OnItemDoubleClick(const Point& point) {
        int relativeY = point.Y + m_scrollOffset;
        
        if (relativeY >= m_headerHeight) {
            int itemIndex = (relativeY - m_headerHeight) / m_itemHeight;
            if (itemIndex >= 0 && itemIndex < (int)m_files.size()) {
                const FileInfo& file = m_files[itemIndex];
                AppUtil::SaveLog("[FileListView] Open: ", AppUtil::WStrToStr(file.fullPath));
                ShellExecuteW(NULL, L"open", file.fullPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                return;
            }
        }
        
        if (OnDoubleClickEmpty) {
            OnDoubleClickEmpty();
        }
    }
    
    const std::wstring& GetFolderPath() const {
        return m_folderPath;
    }
};