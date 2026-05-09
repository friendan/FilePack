#include "MainForm.h"

MainForm::MainForm(int width, int height) : Window(width, height) {
    Init();
}

void MainForm::Init() {
    this->SetText(L"FilePack");
    
    std::wstring xmlContent;
    HRSRC hRsrc = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_MAIN_LAYOUT), RT_HTML);        
    if (!hRsrc) {
        hRsrc = FindResourceW(NULL, MAKEINTRESOURCEW(IDR_MAIN_LAYOUT), L"HTML");
    }
    
    if (hRsrc) {
        HGLOBAL hGlobal = LoadResource(NULL, hRsrc);
        if (hGlobal) {
            DWORD size = SizeofResource(NULL, hRsrc);
            const char* xmlData = (const char*)LockResource(hGlobal);
            if (xmlData && size > 0) {
                int wideLen = MultiByteToWideChar(CP_UTF8, 0, xmlData, size, NULL, 0);
                if (wideLen > 0) {
                    xmlContent.resize(wideLen);
                    MultiByteToWideChar(CP_UTF8, 0, xmlData, size, &xmlContent[0], wideLen);
                }
            }
        }
    }
    
    if (!xmlContent.empty()) {
        ui.LoadXmlData(xmlContent.c_str());
    }
    else {
        AppUtil::SaveLog("[FilePack] ERROR: XML content is empty!");
    }

    ui.SetupUI(this);
    statusLeft = (Label*)this->FindControl("statusLeft");
    statusCenter = (Label*)this->FindControl("statusCenter");
    statusRight = (Label*)this->FindControl("statusRight");
    btnAddTab = (Button*)this->FindControl("btnAddTab");
    mainTabs = (TabLayout*)this->FindControl("mainTabs");
    tabBar = (HLayout*)this->FindControl("tabBar");
    
    Button* btnSelectFolder = (Button*)this->FindControl("btnSelectFolder");
    if (btnSelectFolder) {
        AddLog(L"btnSelectFolder found and event handler set");
        btnSelectFolder->EventHandler = [this](Control* sender, EventArgs& args) {
            if (args.EventType == Event::OnMouseDown) {
                AddLog(L"Select folder button clicked");
                AddLog(currentFileListView ? L"currentFileListView is: NOT NULL" : L"currentFileListView is: NULL");
                if (currentFileListView) {
                    AddLog(L"Calling SelectFolderAndLoad...");
                    SelectFolderAndLoad(currentFileListView);
                } else {
                    AddLog(L"No file list view available");
                }
            }
        };
    } else {
        AddLog(L"btnSelectFolder not found!");
    }
    
    Control* logTabPage = new Control(mainTabs);
    logTabPage->Name = L"logTabPage";
    logTabPage->SetDockStyle(DockStyle::Fill);
    logTabPage->Style.BackColor = Color(255, 255, 255);
    
    logBox = new TextBox();
    logBox->SetParent(logTabPage);
    logBox->SetDockStyle(DockStyle::Fill);
    logBox->Name = L"logBox";
    logBox->SetMultiLine(true);
    logBox->SetReadOnly(true);
    logBox->Style.BackColor = Color(255, 255, 255);
    logBox->Style.ForeColor = Color(0, 0, 0);
    logBox->Style.FontSize = 12;
    
    mainTabs->Add(logTabPage);
    
    Control* tempPage = new Control(mainTabs);
    tempPage->SetDockStyle(DockStyle::Fill);
    tempPage->Style.BackColor = Color(200, 200, 200);
    mainTabs->Add(tempPage);
    
    mainTabs->SetPageIndex(0);
    
    mainTabs->Remove(tempPage, true);
    
    AddLog(L"Program started");
    AddLog(L"Log system initialized");
    
    Button* btnTabLog = (Button*)this->FindControl("btnTabLog");
    if (btnTabLog) {
        tabButtons.push_back(btnTabLog);
        UpdateTabButtonStates(0);
        m_newTabStartIndex = 1;
        btnTabLog->EventHandler = [this](Control* sender, EventArgs& args) {
            if (args.EventType == Event::OnMouseDown) {
                auto startTime = std::chrono::high_resolution_clock::now();
                AddLog(L"[PERF] Switching to log tab");
                
                mainTabs->SetPageIndex(0);
                UpdateTabButtonStates(0);
                
                auto endTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
                AddLog(L"[PERF] SetPageIndex took: " + std::to_wstring(duration) + L"ms");
                
                this->Invalidate();
                
                auto endTime2 = std::chrono::high_resolution_clock::now();
                auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(endTime2 - startTime).count();
                AddLog(L"[PERF] Invalidate + Total took: " + std::to_wstring(duration2) + L"ms");
            }
        };
    }
    
    if (btnAddTab) {
        btnAddTab->EventHandler = [this](Control* sender, EventArgs& args) {
            if (args.EventType == Event::OnMouseDown) {
                AddNewTab();
            }
        };
    }
    
    AddLog(L"ready...");
    UpdateStatus(L"就绪", L"", L"");
}

void MainForm::AddLog(const std::wstring& message) {
    AppUtil::SaveLog("[FilePack UI] ", AppUtil::WStrToStr(message));
    
    if (logBox) {
        time_t now = time(0);
        tm ltm;
        localtime_s(&ltm, &now);
        wchar_t timeStr[32];
        swprintf_s(timeStr, L"%04d-%02d-%02d %02d:%02d:%02d", 
            ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday,
            ltm.tm_hour, ltm.tm_min, ltm.tm_sec);
        
        std::wstring fullMessage = std::wstring(timeStr) + L" " + message + L"\r\n";
        
        std::wstring currentText = AppUtil::StrToWStr(logBox->GetText().c_str());
        logBox->SetText((currentText + fullMessage).c_str());
    }
}

void MainForm::ClearLog() {
    if (logBox) {
        logBox->SetText(L"");
    }
}

void MainForm::UpdateTabButtonStates(int selectedIndex) {
    m_currentTabIndex = selectedIndex;
    for (size_t i = 0; i < tabButtons.size(); i++) {
        if (i == (size_t)selectedIndex) {
            tabButtons[i]->Style.BackColor = Color(194, 216, 166);
        } else {
            tabButtons[i]->Style.BackColor = Color(230, 230, 230);
        }
    }
}

void MainForm::UpdateStatus(const std::wstring& left, const std::wstring& center, const std::wstring& right) {
    if (statusLeft) statusLeft->SetText(left);
    if (statusCenter) statusCenter->SetText(center);
    if (statusRight) statusRight->SetText(right);
}

void MainForm::AddNewTab() {
    tabCount++;
    std::wstring tabTitle = L"文件列表 " + std::to_wstring(tabCount);
    
    AddLog(L"Creating new tab: " + tabTitle);
    
    Control* tabPage = new Control(mainTabs);
    tabPage->Name = L"tabPage" + std::to_wstring(tabCount);
    tabPage->SetDockStyle(DockStyle::Fill);
    
    tabPage->EventHandler = [this, tabPage](Control* sender, EventArgs& args) {
        if (args.EventType == Event::OnMouseDoubleClick) {
            AddLog(L"Double click on tabPage detected");
            FileListView* fileListView = nullptr;
            for (auto ctl : tabPage->GetControls()) {
                fileListView = dynamic_cast<FileListView*>(ctl);
                if (fileListView) break;
            }
            if (fileListView) {
                SelectFolderAndLoad(fileListView);
            }
        }
    };
    
    FileListView* fileListView = new FileListView(tabPage);
    tabPage->Add(fileListView);
    fileListView->SetDockStyle(DockStyle::Fill);
    
    AddLog(L"FileListView created");
    
    fileListView->OnLog = [this](const std::wstring& msg) {
        AddLog(msg);
    };
    
    fileListView->OnDoubleClickEmpty = [this, fileListView]() {
        AddLog(L"Double click empty area detected");
        SelectFolderAndLoad(fileListView);
    };
    
    currentFileListView = fileListView;
    
    mainTabs->Add(tabPage);
    
    AddLog(L"Tab added to TabLayout");
    
    m_tabPages.push_back(tabPage);

    this->Refresh();
    
    int newTabIndex = (int)m_tabPages.size();
    
    // 创建TAB标签容器（标题 + 关闭按钮）
    HLayout* tabContainer = new HLayout(tabBar);
    tabContainer->SetFixedHeight(30);
    tabContainer->Style.BackColor = Color(230, 230, 230);
    tabContainer->Margin.Left = 2;
    tabContainer->Margin.Right = 2;
    
    Label* titleLabel = new Label(tabContainer);
    tabContainer->Add(titleLabel);
    titleLabel->SetText(tabTitle.c_str());
    titleLabel->SetAutoSize(true);
    titleLabel->TextAlign = TextAlign::MiddleCenter;
    titleLabel->Margin.Left = 8;
    titleLabel->Margin.Right = 4;
    
    Button* closeBtn = new Button(tabContainer);
    tabContainer->Add(closeBtn);
    closeBtn->SetText(L"x");
    closeBtn->SetFixedWidth(18);
    closeBtn->SetFixedHeight(18);
    closeBtn->Style.BackColor = Color(0, 0, 0, 0);
    closeBtn->Style.ForeColor = Color(100, 100, 100);
    closeBtn->Style.FontSize = 12;
    closeBtn->Margin.Right = 4;
    closeBtn->Style.Border = 0;
    
    // 关闭按钮事件：延迟删除控件，避免事件处理中自身被销毁
    closeBtn->EventHandler = [this, tabContainer](Control* sender, EventArgs& args) {
        if (args.EventType == Event::OnMouseDown) {
            // 查找索引
            int btnIndex = -1;
            for (size_t i = m_newTabStartIndex; i < tabButtons.size(); i++) {
                if (tabButtons[i] == tabContainer) {
                    btnIndex = (int)i;
                    break;
                }
            }
            if (btnIndex < 0) return;
            
            int pageIndex = btnIndex - m_newTabStartIndex;
            Control* page = (pageIndex < (int)m_tabPages.size()) ? m_tabPages[pageIndex] : nullptr;
            int switchTo = (btnIndex - 1 >= 0) ? btnIndex - 1 : 0;
            
            AddLog(L"Closing tab index: " + std::to_wstring(btnIndex));
            
            // 先移除 vector 数据
            tabButtons.erase(tabButtons.begin() + btnIndex);
            if (page) {
                m_tabPages.erase(m_tabPages.begin() + pageIndex);
            }
            
            // 延迟到事件处理完成后再删除控件
            ezui::BeginInvoke([this, tabContainer, page, switchTo]() {
                tabBar->Remove(tabContainer, true);
                if (page) {
                    mainTabs->Remove(page, true);
                }
                mainTabs->SetPageIndex(switchTo);
                UpdateTabButtonStates(switchTo);
                this->Invalidate();
            });
        }
    };
    
    // 点击标题文字切换到该 TAB
    titleLabel->EventHandler = [this, tabContainer](Control* sender, EventArgs& args) {
        if (args.EventType == Event::OnMouseDown) {
            int btnIndex = -1;
            for (size_t i = m_newTabStartIndex; i < tabButtons.size(); i++) {
                if (tabButtons[i] == tabContainer) {
                    btnIndex = (int)i;
                    break;
                }
            }
            if (btnIndex >= 0) {
                mainTabs->SetPageIndex(btnIndex);
                UpdateTabButtonStates(btnIndex);
                this->Invalidate();
            }
        }
    };
    
    // 插入到 btnAddTab 之前
    int addBtnIndex = tabBar->IndexOf(btnAddTab);
    if (addBtnIndex >= 0) {
        tabBar->Insert(addBtnIndex, tabContainer);
    } else {
        tabBar->Add(tabContainer);
    }
    tabButtons.push_back(tabContainer);
    
    UpdateTabButtonStates(newTabIndex);
    
    mainTabs->SetPageIndex(newTabIndex);
    
    AddLog(L"Switched to new tab");
    UpdateStatus(L"已添加新TAB", tabTitle.c_str(), L"");
}

void MainForm::SelectFolderAndLoad(FileListView* fileListView) {
    AddLog(L"[DEBUG] SelectFolderAndLoad ENTER");
    AddLog(fileListView ? L"SelectFolderAndLoad called, fileListView pointer: valid" : L"SelectFolderAndLoad called, fileListView pointer: null");
    
    if (!fileListView) {
        AddLog(L"ERROR: fileListView is null!");
        return;
    }
    
    BROWSEINFO bi = { 0 };
    bi.lpszTitle = L"选择文件夹";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    AddLog(L"[DEBUG] Calling SHBrowseForFolder...");
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    AddLog(L"[DEBUG] SHBrowseForFolder returned");
    if (pidl != NULL) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDList(pidl, path)) {
            std::wstring folderPath(path);
            AddLog(L"Calling SetFolderPath...");
            fileListView->SetFolderPath(folderPath);
            AddLog(L"SetFolderPath returned");
            
            this->Invalidate();
            AddLog(L"UI refreshed");
            
            AddLog(L"Selected folder: " + folderPath);
            UpdateStatus(L"已选择文件夹", folderPath.c_str(), L"");
        }
        CoTaskMemFree(pidl);
    }
    AddLog(L"[DEBUG] SelectFolderAndLoad EXIT");
}

void MainForm::OnClose(bool& close) {
    Application::Exit(0);
}