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
    int m_headerHeight = 30;
    int m_itemHeight = 25;
    std::vector<HLayout*> m_itemLayouts;
    HLayout* m_headerLayout = nullptr;
    Size m_cachedContentSize;
    
public:
    std::function<void()> OnDoubleClickEmpty = nullptr;
    std::function<void(const std::wstring&)> OnLog = nullptr;

    virtual const Size& GetContentSize() override {
        m_cachedContentSize.Width = Width();
        m_cachedContentSize.Height = m_headerHeight + (int)m_itemLayouts.size() * m_itemHeight;
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
        this->EventPassThrough = Event::None;
        
        m_scrollBar.Parent = this;
        m_scrollBar.SetFixedWidth(14);
        m_scrollBar.OffsetCallback = [this](int offset) {
            m_scrollOffset = offset;
            OffsetItems(offset);
            this->Invalidate();
        };
        
        m_headerLayout = new HLayout(this);
        m_headerLayout->SetFixedHeight(m_headerHeight);
        m_headerLayout->Style.BackColor = Color(200, 200, 200);
        this->Add(m_headerLayout);
        
        Label* label1 = new Label(m_headerLayout);
        label1->SetFixedHeight(m_headerHeight);
        label1->SetFixedWidth(60);
        label1->SetText(L"序号");
        label1->Style.FontSize = 12;
        label1->Style.BackColor = Color(220, 220, 220);
        label1->TextAlign = TextAlign::MiddleCenter;
        m_headerLayout->Add(label1);
        
        Label* label2 = new Label(m_headerLayout);
        label2->SetFixedHeight(m_headerHeight);
        label2->SetFixedWidth(500);
        label2->SetText(L"文件路径");
        label2->Style.FontSize = 12;
        m_headerLayout->Add(label2);
        
        Label* label3 = new Label(m_headerLayout);
        label3->SetFixedHeight(m_headerHeight);
        label3->SetFixedWidth(100);
        label3->SetText(L"大小");
        label3->Style.FontSize = 12;
        m_headerLayout->Add(label3);
        
        Label* label4 = new Label(m_headerLayout);
        label4->SetFixedHeight(m_headerHeight);
        label4->SetFixedWidth(150);
        label4->SetText(L"修改时间");
        label4->Style.FontSize = 12;
        m_headerLayout->Add(label4);
        
        this->Invalidate();
    }
    
    void OffsetItems(int offset) {
        for (size_t i = 0; i < m_itemLayouts.size(); i++) {
            HLayout* item = m_itemLayouts[i];
            int baseY = i * m_itemHeight;
            item->SetY(baseY + offset);
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
        
        if (m_headerLayout) {
            m_headerLayout->SetRect({ 0, 0, Width(), m_headerHeight });
            m_headerLayout->RefreshLayout();
            
            int xpos = 0;
            for (auto& child : m_headerLayout->GetControls()) {
                child->SetX(xpos);
                xpos += child->Width();
            }
            
            // 确保 header 在最前面绘制
            this->Remove(m_headerLayout);
            this->Add(m_headerLayout);
        }
        
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
            this->Remove(item, true);
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
            
            int y = 0;
            for (size_t i = 0; i < m_files.size(); i++) {
                const auto& file = m_files[i];
                HLayout* itemLayout = new HLayout(this);
                // 添加到 FileListView（不是 header）
                this->Add(itemLayout);
                itemLayout->SetFixedHeight(m_itemHeight);
                itemLayout->SetY(y);
                itemLayout->Style.BackColor = (i % 2 == 0) ? Color(255, 255, 255) : Color(248, 248, 248);
                m_itemLayouts.push_back(itemLayout);
                
                Label* indexLabel = new Label(itemLayout);
                itemLayout->Add(indexLabel);
                indexLabel->SetText((L"#" + std::to_wstring(i + 1)).c_str());
                indexLabel->SetFixedWidth(60);
                indexLabel->Style.FontSize = 11;
                indexLabel->Style.ForeColor = Color(100, 100, 100);
                indexLabel->TextAlign = TextAlign::MiddleCenter;
                
                Label* pathLabel = new Label(itemLayout);
                itemLayout->Add(pathLabel);
                pathLabel->SetText(file.fullPath.c_str());
                pathLabel->SetFixedWidth(500);
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
                
                y += m_itemHeight;
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