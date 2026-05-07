#pragma once
#include <Windows.h>
#include <VLayout.h>
#include <Label.h>
#include <HLayout.h>
#include <VScrollBar.h>
#include <CheckBox.h>
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
    std::vector<CheckBox*> m_checkBoxs;
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
        
        // 表头复选框功能：全选/取消全选当前可见文件
        CheckBox* headerCheckBox = (CheckBox*)this->FindControl("header")->GetControls()[0];
        if (headerCheckBox) {
            // 设置复选框样式：未选中时显示带灰色边框的白色方块
            headerCheckBox->Style.Border = 1;
            headerCheckBox->Style.Border.Color = Color(120, 120, 120);
            headerCheckBox->Style.Border.Style = StrokeStyle::Solid;
            headerCheckBox->Style.BackColor = Color(255, 255, 255);
            // 选中时：边框变为主题色，显示 ✔ 标记
            headerCheckBox->CheckedStyle.Border = 1;
            headerCheckBox->CheckedStyle.Border.Color = Color(0, 120, 212);
            headerCheckBox->CheckedStyle.Border.Style = StrokeStyle::Solid;
            headerCheckBox->CheckedStyle.BackColor = Color(255, 255, 255);
            headerCheckBox->CheckedStyle.ForeColor = Color(0, 120, 212);
            headerCheckBox->SetText(L"");
            headerCheckBox->TextAlign = TextAlign::MiddleCenter;
            
            headerCheckBox->CheckedChanged = [this, headerCheckBox](CheckBox* sender, bool checked) {
                // 更新表头复选框文字显示状态
                headerCheckBox->SetText(checked ? L"\u2714" : L"");
                // 只处理当前可见的文件行
                int firstVisible = 0;
                int lastVisible = (int)m_checkBoxs.size() - 1;
                
                // 计算可见范围（根据滚动偏移）
                if (m_scrollOffset < 0) {
                    firstVisible = (-m_scrollOffset) / m_itemHeight;
                }
                int visibleCount = Height() / m_itemHeight;
                lastVisible = min(lastVisible, firstVisible + visibleCount);
                
                for (int i = firstVisible; i <= lastVisible && i < (int)m_checkBoxs.size(); i++) {
                    if (m_checkBoxs[i]) {
                        m_checkBoxs[i]->SetCheck(checked);
                    }
                }
                
                if (OnLog) {
                    OnLog(L"[FileListView] " + std::wstring(checked ? L"全选" : L"取消全选") + L"可见文件");
                }
            };
        }
        
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
        m_checkBoxs.clear();
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
                
// 添加复选框
                CheckBox* cb = new CheckBox(itemLayout);
                itemLayout->Add(cb);
                cb->SetFixedWidth(20);
                cb->SetFixedHeight(m_itemHeight);
                // 复选框样式：灰色边框，白色背景
                cb->Style.Border = 1;
                cb->Style.Border.Color = Color(160, 160, 160);
                cb->Style.Border.Style = StrokeStyle::Solid;
                cb->Style.BackColor = Color(255, 255, 255);
                // 选中时：蓝色边框，蓝色 ✔ 标记
                cb->CheckedStyle.Border = 1;
                cb->CheckedStyle.Border.Color = Color(0, 120, 212);
                cb->CheckedStyle.Border.Style = StrokeStyle::Solid;
                cb->CheckedStyle.BackColor = Color(255, 255, 255);
                cb->CheckedStyle.ForeColor = Color(0, 120, 212);
                cb->SetText(L"");
                cb->TextAlign = TextAlign::MiddleCenter;
                // 勾选/取消时更新文字
                cb->CheckedChanged = [cb](CheckBox* sender, bool checked) {
                    cb->SetText(checked ? L"\u2714" : L"");
                };
                m_checkBoxs.push_back(cb);
                
                if (OnLog) OnLog(L"[FileListView] CheckBox added for row " + std::to_wstring(i));
                
                Label* indexLabel = new Label(itemLayout);
                itemLayout->Add(indexLabel);
                indexLabel->SetText((L"#" + std::to_wstring(i + 1)).c_str());
                indexLabel->SetFixedWidth(40);
                indexLabel->TextAlign = TextAlign::MiddleCenter;
                
                Label* pathLabel = new Label(itemLayout);
                itemLayout->Add(pathLabel);
                pathLabel->SetText(file.fullPath.c_str());
                pathLabel->SetFixedWidth(480);
                
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
        int itemIndex = relativeY / m_itemHeight;
        if (itemIndex >= 0 && itemIndex < (int)m_files.size()) {
            const FileInfo& file = m_files[itemIndex];
            AppUtil::SaveLog("[FileListView] Open: ", AppUtil::WStrToStr(file.fullPath));
            ShellExecuteW(NULL, L"open", file.fullPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
            return;
        }
        
        if (OnDoubleClickEmpty) {
            OnDoubleClickEmpty();
        }
    }
    
    const std::wstring& GetFolderPath() const {
        return m_folderPath;
    }
};