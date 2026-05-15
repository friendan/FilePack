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
#include <thread>
#include <shellapi.h>
#include <CommCtrl.h>
#include <archive.h>
#include <archive_entry.h>
#include "PathUtil.hpp"
#include "Lzma2Enc.h"
#include "Alloc.h"
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
    std::function<void(const std::wstring&)> OnFolderChanged = nullptr;

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
        if (OnFolderChanged) {
            OnFolderChanged(path);
        }
    }
    
public:
    FileListView(Object* parentObject = NULL) : VLayout(parentObject) {
        Init();
    }
    
    ~FileListView() {
        HWND hWnd = Hwnd();
        if (hWnd) {
            RemoveWindowSubclass(hWnd, &FileListView::StaticWndProc, (UINT_PTR)this);
        }
    }
    
    static LRESULT CALLBACK StaticWndProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        if (uMsg == WM_RBUTTONDOWN) {
            FileListView* self = (FileListView*)dwRefData;
            if (!self) return DefSubclassProc(hW, uMsg, wParam, lParam);
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            // 遍历父控件链，计算 FileListView 在窗口客户区的位置
            int absX = 0, absY = 0;
            Control* c = self;
            while (c) {
                auto r = c->GetRect();
                absX += r.X;
                absY += r.Y;
                c = c->Parent;
                if (c && c->IsWindow()) break;
            }
            // 只响应 FileListView 自身区域内的右键
            if (pt.x >= absX && pt.x < absX + self->Width() &&
                pt.y >= absY && pt.y < absY + self->Height()) {
                MouseEventArgs args(Event::OnMouseDown, Point(pt.x - absX, pt.y - absY), MouseButton::Right);
                self->OnRightClick(args);
                return 0;
            }
        }
        return DefSubclassProc(hW, uMsg, wParam, lParam);
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
        
        // 表头复选框已删除
        
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
                SetWindowSubclass(hWnd, &FileListView::StaticWndProc, (UINT_PTR)this, (DWORD_PTR)this);
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
        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, 1001, L"选中今天修改的文件");
        AppendMenuW(hMenu, MF_STRING, 1002, L"选中前10行");
        AppendMenuW(hMenu, MF_STRING, 1003, L"继续选10行");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING, 1004, L"取消选中所有行");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING, 1005, L"打包选中文件为 tar.gz");
        AppendMenuW(hMenu, MF_STRING, 1006, L"打包选中文件为 7z (LZMA2)");

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
        case 1005:
            PackSelectedFiles();
            break;
        case 1006:
            PackSelectedFiles7z();
            break;
        }
    }
    
    void PackSelectedFiles() {
        // 统计选中的文件
        int totalSelected = 0;
        for (size_t i = 0; i < m_checkBoxs.size() && i < m_files.size(); i++) {
            if (m_checkBoxs[i]->GetCheck()) totalSelected++;
        }
        if (totalSelected == 0) {
            if (OnLog) OnLog(L"[Pack] No files selected");
            return;
        }
        
        // 生成文件名：年月日_时分秒.tar.gz
        SYSTEMTIME st;
        GetLocalTime(&st);
        wchar_t fileName[64];
        swprintf_s(fileName, L"%04d%02d%02d_%02d%02d%02d.tar.gz",
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond);
        
        // 生成路径：exe所在目录\pack\文件名
        std::wstring packDir = PathUtil::GetExeDir() + L"\\pack";
        PathUtil::EnsureDirExists(packDir);
        std::wstring outputPath = packDir + L"\\" + fileName;
        
        if (OnLog) OnLog(L"[Pack] Creating: " + outputPath);
        if (OnLog) OnLog(L"[Pack] Files: " + std::to_wstring(totalSelected));
        
        struct archive* a = archive_write_new();
        archive_write_add_filter_gzip(a);
        archive_write_set_format_pax(a);
        
        int r = archive_write_open_filename(a, AppUtil::WStrToStr(outputPath).c_str());
        if (r != ARCHIVE_OK) {
            if (OnLog) OnLog(L"[Pack] Failed to open: " + std::wstring(AppUtil::StrToWStr(archive_error_string(a))));
            archive_write_free(a);
            return;
        }
        
        int packedCount = 0;
        for (size_t fi = 0; fi < m_files.size() && fi < m_checkBoxs.size(); fi++) {
            if (!m_checkBoxs[fi]->GetCheck()) continue;
            const auto& filePath = m_files[fi].fullPath;
            // 计算相对路径：文件夹名/文件相对路径
            std::wstring folderName = m_folderPath;
            size_t pos = folderName.find_last_of(L"\\/");
            if (pos != std::wstring::npos) {
                folderName = folderName.substr(pos + 1);
            }
            std::wstring relativePath = filePath;
            if (relativePath.compare(0, m_folderPath.length(), m_folderPath) == 0) {
                if (relativePath.length() > m_folderPath.length() + 1) {
                    relativePath = relativePath.substr(m_folderPath.length() + 1);
                } else {
                    relativePath = L"";
                }
            }
            if (!relativePath.empty()) {
                relativePath = folderName + L"/" + relativePath;
            } else {
                relativePath = folderName;
            }
            
            // 读取文件内容
            FILE* f = nullptr;
            if (_wfopen_s(&f, filePath.c_str(), L"rb") != 0 || !f) {
                if (OnLog) OnLog(L"[Pack] Cannot open: " + filePath);
                continue;
            }
            
            _fseeki64(f, 0, SEEK_END);
            int64_t fileSize = _ftelli64(f);
            _fseeki64(f, 0, SEEK_SET);
            
            struct archive_entry* entry = archive_entry_new();
            archive_entry_set_pathname(entry, AppUtil::WStrToStr(relativePath).c_str());
            archive_entry_set_size(entry, fileSize);
            archive_entry_set_filetype(entry, AE_IFREG);
            archive_entry_set_perm(entry, 0644);
            // 保留文件修改时间
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                m_files[fi].modifyTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
            time_t mtime = std::chrono::system_clock::to_time_t(sctp);
            archive_entry_set_mtime(entry, mtime, 0);
            
            r = archive_write_header(a, entry);
            if (r != ARCHIVE_OK) {
                if (OnLog) OnLog(L"[Pack] Header error: " + std::wstring(AppUtil::StrToWStr(archive_error_string(a))));
                archive_entry_free(entry);
                fclose(f);
                continue;
            }
            
            char buf[65536];
            size_t bytesRead;
            while ((bytesRead = fread(buf, 1, sizeof(buf), f)) > 0) {
                archive_write_data(a, buf, bytesRead);
            }
            
            archive_entry_free(entry);
            fclose(f);
            packedCount++;
        }
        
        archive_write_close(a);
        archive_write_free(a);
        
        if (OnLog) OnLog(L"[Pack] Done! Packed " + std::to_wstring(packedCount) + L" files to: " + outputPath);
    }

    void PackSelectedFiles7z() {
        // 统计选中的文件并收集路径
        std::vector<std::wstring> fileList;
        for (size_t i = 0; i < m_checkBoxs.size() && i < m_files.size(); i++) {
            if (m_checkBoxs[i]->GetCheck()) {
                fileList.push_back(m_files[i].fullPath);
            }
        }
        if (fileList.empty()) {
            if (OnLog) OnLog(L"[7z] No files selected");
            return;
        }
        
        // 生成文件名：年月日_时分秒.7z
        SYSTEMTIME st;
        GetLocalTime(&st);
        wchar_t fileName[64];
        swprintf_s(fileName, L"%04d%02d%02d_%02d%02d%02d.7z",
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond);
        
        // 生成路径：exe所在目录\pack\文件名
        std::wstring packDir = PathUtil::GetExeDir() + L"\\pack";
        PathUtil::EnsureDirExists(packDir);
        std::wstring outputPath = packDir + L"\\" + fileName;
        
        if (OnLog) OnLog(L"[7z] Creating: " + outputPath);
        if (OnLog) OnLog(L"[7z] Files: " + std::to_wstring((int)fileList.size()));
        
        // 查找 7zG.exe
        std::wstring sevenZExe = PathUtil::GetExeDir() + L"\\7z\\7zG.exe";
        if (GetFileAttributesW(sevenZExe.c_str()) == INVALID_FILE_ATTRIBUTES) {
            sevenZExe = L"7zG.exe";
            if (OnLog) OnLog(L"[7z] Using system 7zG.exe");
        } else {
            if (OnLog) OnLog(L"[7z] Using: " + sevenZExe);
        }
        
        // 计算工作目录（m_folderPath 的父目录）和文件夹名
        std::wstring parentDir = m_folderPath;
        std::wstring folderName = m_folderPath;
        size_t pos = folderName.find_last_of(L"\\/");
        if (pos != std::wstring::npos) {
            folderName = folderName.substr(pos + 1);
            parentDir = parentDir.substr(0, pos);
        }
        
        // 构建命令行（使用相对路径）
        // 7zG.exe a -ad -mx5 -t7z "输出路径" "文件夹名\相对路径" ...
        std::wstring cmdLine = L"\"" + sevenZExe + L"\" a -ad -mx5 -t7z \"" + outputPath + L"\"";
        for (const auto& f : fileList) {
            // 计算相对路径：文件夹名\剩下的路径
            std::wstring relPath = f;
            if (relPath.compare(0, m_folderPath.length(), m_folderPath) == 0) {
                if (relPath.length() > m_folderPath.length() + 1) {
                    relPath = relPath.substr(m_folderPath.length() + 1);
                } else {
                    relPath = L"";
                }
            }
            if (!relPath.empty()) {
                relPath = folderName + L"\\" + relPath;
            } else {
                relPath = folderName;
            }
            cmdLine += L" \"" + relPath + L"\"";
        }
        
        if (OnLog) OnLog(L"[7z] Running: " + cmdLine);
        if (OnLog) OnLog(L"[7z] WorkDir: " + parentDir);
        
        // 启动 7zG.exe（显示窗口，不等待）
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOWNORMAL;
        
        // cmdline 需要可写缓冲区
        std::vector<wchar_t> cmdBuf(cmdLine.c_str(), cmdLine.c_str() + cmdLine.size() + 1);
        
        BOOL ok = CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE,
            0, NULL, parentDir.c_str(), &si, &pi);
        
        if (!ok) {
            if (OnLog) OnLog(L"[7z] Failed to launch 7zG.exe");
            return;
        }
        
        // 不等待，关闭句柄即可
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        if (OnLog) OnLog(L"[7z] 7zG.exe launched for " + std::to_wstring((int)fileList.size()) + L" files");
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
        // 先清空旧数据（主线程）
        for (auto item : m_itemLayouts) {
            m_contentLayout->Remove(item, true);
        }
        m_itemLayouts.clear();
        m_checkBoxs.clear();
        m_files.clear();
        this->RefreshLayout();
        
        // 后台线程遍历文件夹
        std::thread([this, folderPath]() {
            std::vector<FileInfo> files;
            try {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(folderPath)) {
                    if (entry.is_regular_file()) {
                        FileInfo info;
                        info.fullPath = entry.path().wstring();
                        info.fileSize = entry.file_size();
                        info.modifyTime = entry.last_write_time();
                        files.push_back(info);
                    }
                }
                
                std::sort(files.begin(), files.end(),
                    [](const FileInfo& a, const FileInfo& b) {
                        return a.modifyTime > b.modifyTime;
                    });
                
                // 回到主线程更新 UI
                ezui::BeginInvoke([this, files]() {
                    m_files = files;
                    
                    for (size_t i = 0; i < m_files.size(); i++) {
                        const auto& file = m_files[i];
                        HLayout* itemLayout = new HLayout(m_contentLayout);
                        m_contentLayout->Add(itemLayout);
                        itemLayout->SetFixedHeight(m_itemHeight);
                        Color originalBgColor = (i % 2 == 0) ? Color(255, 255, 255) : Color(248, 248, 248);
                        itemLayout->Style.BackColor = originalBgColor;
                        m_itemLayouts.push_back(itemLayout);
                        
                        CheckBox* cb = new CheckBox(itemLayout);
                        itemLayout->Add(cb);
                        cb->SetFixedWidth(20);
                        cb->SetFixedHeight(m_itemHeight);
                        cb->Style.Border = 1;
                        cb->Style.Border.Color = Color(160, 160, 160);
                        cb->Style.Border.Style = StrokeStyle::Solid;
                        cb->Style.BackColor = Color(255, 255, 255);
                        cb->CheckedStyle.Border = 1;
                        cb->CheckedStyle.Border.Color = Color(0, 120, 212);
                        cb->CheckedStyle.Border.Style = StrokeStyle::Solid;
                        cb->CheckedStyle.BackColor = Color(255, 255, 255);
                        cb->CheckedStyle.ForeColor = Color(0, 120, 212);
                        cb->SetText(L"");
                        cb->TextAlign = TextAlign::MiddleCenter;
                        cb->CheckedChanged = [cb, itemLayout, originalBgColor](CheckBox* sender, bool checked) {
                            cb->SetText(checked ? L"\u2714" : L"");
                            itemLayout->Style.BackColor = checked ? Color(220, 235, 255) : originalBgColor;
                            cb->Invalidate();
                        };
                        cb->EventPassThrough = Event::OnMouseDown;
                        m_checkBoxs.push_back(cb);
                        
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
                        pathLabel->SetRateWidth(1.0f);
                        pathLabel->SetElidedText(L"...");
                        
                        indexLabel->EventPassThrough = Event::OnMouseDoubleClick | Event::OnMouseDown;
                        timeLabel->EventPassThrough = Event::OnMouseDoubleClick | Event::OnMouseDown;
                        sizeLabel->EventPassThrough = Event::OnMouseDoubleClick | Event::OnMouseDown;
                        pathLabel->EventPassThrough = Event::OnMouseDoubleClick | Event::OnMouseDown;
                        
                        itemLayout->EventPassThrough = Event::OnMouseDoubleClick | Event::OnMouseDown;
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
                    
                    this->RefreshLayout();
                    m_scrollOffset = 0;
                });
            }
            catch (const std::exception& e) {
                std::string err = e.what();
                ezui::BeginInvoke([this, err]() {
                    if (OnLog) OnLog(L"[FileListView] Error: " + std::wstring(err.begin(), err.end()));
                });
            }
        }).detach();
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