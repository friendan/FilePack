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
#include <CommCtrl.h>
#pragma comment(lib, "comctl32.lib")

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
            m_contentLayout->EventPassThrough = Event::OnMouseDoubleClick | Event::OnMouseDown;
            m_contentLayout->EventHandler = [this](Control* sender, EventArgs& args) {
                if (args.EventType == Event::OnMouseDoubleClick) {
                    MouseEventArgs& mouseArgs = (MouseEventArgs&)args;
                    // 只处理空白区域（没有文件行的区域）
                    if (mouseArgs.Location.Y >= (int)m_itemLayouts.size() * m_itemHeight - m_scrollOffset) {
                        if (OnDoubleClickEmpty) {
                            OnDoubleClickEmpty();
                        }
                    }
                }
            };
        }

        // 延迟安装窗口子类化处理右键消息（等 Hwnd() 可用后）
        ezui::BeginInvoke([this]() {
            HWND hWnd = Hwnd();
            if (hWnd) {
                SetWindowSubclass(hWnd, [](HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) -> LRESULT {
                    if (uMsg == WM_RBUTTONDOWN) {
                        FileListView* self = (FileListView*)dwRefData;
                        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                        auto r = self->GetRect();
                        MouseEventArgs args(Event::OnMouseDown, Point(pt.x - r.X, pt.y - r.Y), MouseButton::Right);
                        self->OnRightClick(args);
                        return 0;
                    }
                    return DefSubclassProc(hW, uMsg, wParam, lParam);
                }, (UINT_PTR)this, (DWORD_PTR)this);
            }
        });

        this->Invalidate();
    }
    
    void SelectAllTodayFiles() {
        auto now = std::chrono::system_clock::now();
        auto now_t = std::chrono::system_clock::to_time_t(now);
        tm today;
        localtime_s(&today, &now_t);

        for (size_t i = 0; i < m_files.size() && i < m_checkBoxs.size(); i++) {
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                m_files[i].modifyTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
            std::time_t ft = std::chrono::system_clock::to_time_t(sctp);
            tm file_tm;
            localtime_s(&file_tm, &ft);

            bool isToday = (file_tm.tm_year == today.tm_year &&
                          file_tm.tm_mon == today.tm_mon &&
                          file_tm.tm_mday == today.tm_mday);

            CheckBox* cb = m_checkBoxs[i];
            cb->SetCheck(isToday);
            if (cb->CheckedChanged) {
                cb->CheckedChanged(cb, isToday);
            }
        }
    }

    void SelectTopN(int n) {
        int count = min(n, (int)min(m_files.size(), m_checkBoxs.size()));
        for (int i = 0; i < count; i++) {
            CheckBox* cb = m_checkBoxs[i];
            cb->SetCheck(true);
            if (cb->CheckedChanged) {
                cb->CheckedChanged(cb, true);
            }
        }
    }

    void SelectNextN(int n) {
        int lastSelected = -1;
        for (int i = (int)m_checkBoxs.size() - 1; i >= 0; i--) {
            if (m_checkBoxs[i]->GetCheck()) {
                lastSelected = i;
                break;
            }
        }

        if (lastSelected < 0) {
            SelectTopN(n);
            return;
        }

        int start = lastSelected + 1;
        int end = min(start + n, (int)min(m_files.size(), m_checkBoxs.size()));
        for (int i = start; i < end; i++) {
            CheckBox* cb = m_checkBoxs[i];
            cb->SetCheck(true);
            if (cb->CheckedChanged) {
                cb->CheckedChanged(cb, true);
            }
        }
    }

    void OnRightClick(const MouseEventArgs& mouseArgs) {
        // 获取点击的行索引
        int relativeY = mouseArgs.Location.Y + m_scrollOffset;
        int itemIndex = relativeY / m_itemHeight;

        // 如果点击在文件行上，选中该行
        if (itemIndex >= 0 && itemIndex < (int)m_checkBoxs.size()) {
            CheckBox* cb = m_checkBoxs[itemIndex];
            cb->SetCheck(true);
            if (cb->CheckedChanged) {
                cb->CheckedChanged(cb, true);
            }
        }

        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, 1001, L"选中今天修改的文件");
        AppendMenuW(hMenu, MF_STRING, 1002, L"选中前10行");
        AppendMenuW(hMenu, MF_STRING, 1003, L"继续选10行");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING, 1004, L"取消选中所有行");

        POINT pt = { mouseArgs.Location.X, mouseArgs.Location.Y };
        ClientToScreen(Hwnd(), &pt);

        UINT cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, Hwnd(), NULL);
        DestroyMenu(hMenu);

        switch (cmd) {
        case 1001:
            SelectAllTodayFiles();
            break;
        case 1002:
            SelectTopN(10);
            break;
        case 1003:
            SelectNextN(10);
            break;
        case 1004:
            for (auto* cb : m_checkBoxs) {
                cb->SetCheck(false);
                if (cb->CheckedChanged) {
                    cb->CheckedChanged(cb, false);
                }
            }
            break;
        }
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
                Color originalBgColor = (i % 2 == 0) ? Color(255, 255, 255) : Color(248, 248, 248);
                itemLayout->Style.BackColor = originalBgColor;
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
                // 勾选/取消时更新文字，并改变行背景色
                cb->CheckedChanged = [cb, itemLayout, originalBgColor](CheckBox* sender, bool checked) {
                    cb->SetText(checked ? L"\u2714" : L"");
                    itemLayout->Style.BackColor = checked ? Color(220, 235, 255) : originalBgColor;
                    cb->Invalidate();
                };
                m_checkBoxs.push_back(cb);
                
                if (OnLog) OnLog(L"[FileListView] CheckBox added for row " + std::to_wstring(i));
                
                Label* indexLabel = new Label(itemLayout);
                itemLayout->Add(indexLabel);
                indexLabel->SetText((L"#" + std::to_wstring(i + 1)).c_str());
                indexLabel->SetFixedWidth(40);
                indexLabel->TextAlign = TextAlign::MiddleCenter;
                
                Label* timeLabel = new Label(itemLayout);
                itemLayout->Add(timeLabel);
                timeLabel->SetText(AppUtil::FormatFileTime(file.modifyTime).c_str());
                timeLabel->SetFixedWidth(150);
                
                Label* sizeLabel = new Label(itemLayout);
                itemLayout->Add(sizeLabel);
                sizeLabel->SetText(AppUtil::FormatFileSize(file.fileSize).c_str());
                sizeLabel->SetFixedWidth(100);
                
                Label* pathLabel = new Label(itemLayout);
                itemLayout->Add(pathLabel);
                pathLabel->SetText(file.fullPath.c_str());
                pathLabel->TextAlign = TextAlign::MiddleLeft;
                // 文件路径列自动占满剩余宽度
                pathLabel->SetRateWidth(1.0f);
                // 超出列宽时显示省略号
                pathLabel->SetElidedText(L"...");
                
                // 行内所有子控件穿透双击和右键事件
                indexLabel->EventPassThrough = Event::OnMouseDoubleClick | Event::OnMouseDown;
                timeLabel->EventPassThrough = Event::OnMouseDoubleClick | Event::OnMouseDown;
                sizeLabel->EventPassThrough = Event::OnMouseDoubleClick | Event::OnMouseDown;
                pathLabel->EventPassThrough = Event::OnMouseDoubleClick | Event::OnMouseDown;
                
                itemLayout->EventPassThrough = Event::OnMouseDoubleClick | Event::OnMouseDown;
                // 行双击事件：切换复选框
                int cbIndex = i;
                itemLayout->EventHandler = [this, cbIndex](Control* sender, EventArgs& args) {
                    if (args.EventType == Event::OnMouseDoubleClick) {
                        if (cbIndex < (int)m_checkBoxs.size() && m_checkBoxs[cbIndex]) {
                            CheckBox* cb = m_checkBoxs[cbIndex];
                            cb->SetCheck(!cb->GetCheck());
                            if (cb->CheckedChanged) {
                                cb->CheckedChanged(cb, cb->GetCheck());
                            }
                        }
                    }
                };
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
            // 切换复选框状态（等同于点击复选框）
            if (itemIndex < (int)m_checkBoxs.size() && m_checkBoxs[itemIndex]) {
                CheckBox* cb = m_checkBoxs[itemIndex];
                cb->SetCheck(!cb->GetCheck());
                if (cb->CheckedChanged) {
                    cb->CheckedChanged(cb, cb->GetCheck());
                }
            }
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