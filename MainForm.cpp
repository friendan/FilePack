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
    
    // 窗口拖动通过 WndProc 中 WM_NCHITTEST 实现
    
    statusLeft = (Label*)this->FindControl("statusLeft");
    statusCenter = (Label*)this->FindControl("statusCenter");
    statusRight = (Label*)this->FindControl("statusRight");
    btnAddTab = (Button*)this->FindControl("btnAddTab");
    mainTabs = (TabLayout*)this->FindControl("mainTabs");
    tabBar = (HLayout*)this->FindControl("tabBar");
    btnTabLog = (Button*)this->FindControl("btnTabLog");
    btnToggleLog = (Button*)this->FindControl("btnToggleLog");
    
    // 日志切换按钮
    if (btnToggleLog) {
        btnToggleLog->EventHandler = [this](Control* sender, EventArgs& args) {
            if (args.EventType == Event::OnMouseDown) {
                if (btnTabLog) {
                    bool isVisible = btnTabLog->IsVisible();
                    btnTabLog->SetVisible(!isVisible);
                    if (mainTabs) {
                        if (!isVisible) {
                            // 显示时切换到日志 TAB（索引 0）
                            mainTabs->SetPageIndex(0);
                            UpdateTabButtonStates(0);
                            UpdateStatus(L"", L"");
                        } else if (m_tabPages.size() > 0) {
                            // 隐藏时切换到第一个文件 TAB
                            mainTabs->SetPageIndex(m_newTabStartIndex);
                            UpdateTabButtonStates(m_newTabStartIndex);
                        }
                    }
                    this->Invalidate();
                }
            }
        };
    }
    
    VLayout* logTabPage = new VLayout(mainTabs);
    logTabPage->Name = L"logTabPage";
    logTabPage->SetDockStyle(DockStyle::Fill);
    logTabPage->Style.BackColor = Color(255, 255, 255);
    logTabPage->Margin.Top = 5;
    logTabPage->Margin.Left = 5;
    
    logBox = new TextBox();
    logBox->SetParent(logTabPage);
    logBox->SetDockStyle(DockStyle::Fill);
    logBox->Name = L"logBox";
    logBox->SetMultiLine(true);
    logBox->SetReadOnly(true);
    logBox->Style.BackColor = Color(255, 255, 255);
    logBox->Style.ForeColor = Color(0, 0, 0);
    logBox->Style.FontSize = 12;
    logBox->Margin.Left = 10;
    logBox->Margin.Top = 10;
    
    mainTabs->Add(logTabPage);
    
    Control* tempPage = new Control(mainTabs);
    tempPage->SetDockStyle(DockStyle::Fill);
    tempPage->Style.BackColor = Color(200, 200, 200);
    mainTabs->Add(tempPage);
    
    mainTabs->SetPageIndex(0);
    
    mainTabs->Remove(tempPage, true);
    
    Button* btnTabLog = (Button*)this->FindControl("btnTabLog");
    if (btnTabLog) {
        tabButtons.push_back(btnTabLog);
        UpdateTabButtonStates(0);
        m_newTabStartIndex = 1;
        btnTabLog->EventHandler = [this](Control* sender, EventArgs& args) {
            if (args.EventType == Event::OnMouseDown) {
                mainTabs->SetPageIndex(0);
                UpdateTabButtonStates(0);
                UpdateStatus(L"", L"");
                this->Invalidate();
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
    
    // 从配置文件恢复之前保存的文件夹 TAB（延迟执行，确保子类化已安装）
    auto savedFolders = m_config.GetFolders();
    if (!savedFolders.empty()) {
        this->Refresh();
        ezui::BeginInvoke([this, savedFolders]() {
            for (const auto& folder : savedFolders) {
                if (!folder.empty()) {
                    AddNewTab();
                    SelectFolderAndLoad(currentFileListView, folder);
                }
            }
        });
    }
    
    if (m_tabPages.empty()) {
        UpdateStatus(L"就绪", L"");
    }
    
    // 启用拖放文件/文件夹
    DragAcceptFiles(this->Hwnd(), TRUE);
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

void MainForm::UpdateStatus(const std::wstring& left, const std::wstring& center) {
    if (statusLeft) statusLeft->SetText(left);
}

void MainForm::AddNewTab() {
    tabCount++;
    std::wstring tabTitle = L"文件列表 " + std::to_wstring(tabCount);
    
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
    
    fileListView->OnLog = [this](const std::wstring& msg) {
        AddLog(msg);
    };
    
    fileListView->OnDoubleClickEmpty = [this, fileListView]() {
        SelectFolderAndLoad(fileListView);
    };
    
    currentFileListView = fileListView;
    
    mainTabs->Add(tabPage);
    
    m_tabPages.push_back(tabPage);

    int newTabIndex = (int)m_tabPages.size();
    
    // 创建TAB标签容器（标题 + 关闭按钮 + 分隔符）
    HLayout* tabContainer = new HLayout();
    tabContainer->SetFixedHeight(30);
    tabContainer->SetFixedWidth(156);
    tabContainer->Style.BackColor = Color(230, 230, 230);
    tabContainer->Margin.Left = 0;
    tabContainer->Margin.Right = 0;
    
    Label* titleLabel = new Label(tabContainer);
    tabContainer->Add(titleLabel);
    titleLabel->SetText(tabTitle.c_str());
    titleLabel->SetFixedWidth(120);
    titleLabel->TextAlign = TextAlign::MiddleLeft;
    titleLabel->Margin.Left = 8;
    titleLabel->Margin.Right = 2;
    
    // 双击 TAB 标签自定义名称
    TextBox* tabNameEditor = new TextBox(tabContainer);
    tabNameEditor->SetMultiLine(false);
    tabNameEditor->SetFixedWidth(120);
    tabNameEditor->SetFixedHeight(20);
    tabNameEditor->Style.Border = 1;
    tabNameEditor->Style.Border.Color = Color(0, 120, 212);
    tabNameEditor->Style.Border.Style = StrokeStyle::Solid;
    tabNameEditor->Style.FontSize = 12;
    tabNameEditor->Margin.Left = 8;
    tabNameEditor->SetVisible(false);
    tabContainer->Add(tabNameEditor);
    
    // 双击 Label 时事件穿透到 tabContainer，由 tabContainer 处理
    titleLabel->EventPassThrough = Event::OnMouseDoubleClick;
    tabContainer->EventHandler = [this, titleLabel, tabNameEditor, fileListView](Control* sender, EventArgs& args) {
        if (args.EventType == Event::OnMouseDoubleClick) {
            tabNameEditor->SetText(titleLabel->GetText().c_str());
            titleLabel->SetVisible(false);
            tabNameEditor->SetVisible(true);
            this->SetFocus(tabNameEditor);
        }
    };
    
    tabNameEditor->EventHandler = [this, titleLabel, tabNameEditor, fileListView](Control* sender, EventArgs& args) {
        if (args.EventType == Event::OnKillFocus) {
            std::wstring name = AppUtil::StrToWStr(tabNameEditor->GetText().c_str());
            if (!name.empty()) {
                titleLabel->SetText(name.c_str());
                std::wstring fp = fileListView->GetFolderPath();
                if (!fp.empty()) {
                    m_config.SetCustomTabName(fp, name);
                }
            }
            tabNameEditor->SetVisible(false);
            titleLabel->SetVisible(true);
        }
        if (args.EventType == Event::OnKeyDown) {
            KeyboardEventArgs& keyArgs = (KeyboardEventArgs&)args;
            if (keyArgs.wParam == VK_RETURN) {
                std::wstring name = AppUtil::StrToWStr(tabNameEditor->GetText().c_str());
                if (!name.empty()) {
                    titleLabel->SetText(name.c_str());
                    std::wstring fp = fileListView->GetFolderPath();
                    if (!fp.empty()) {
                        m_config.SetCustomTabName(fp, name);
                    }
                }
                tabNameEditor->SetVisible(false);
                titleLabel->SetVisible(true);
            }
        }
    };
    
    // 选择文件夹后更新 TAB 标题为文件夹名
    fileListView->OnFolderChanged = [this, titleLabel, fileListView](const std::wstring& folderPath) {
        std::wstring customName = m_config.GetCustomTabName(folderPath);
        if (!customName.empty()) {
            titleLabel->SetText(customName.c_str());
        } else {
            std::wstring folderName = folderPath;
            size_t pos = folderName.find_last_of(L"\\/");
            if (pos != std::wstring::npos) {
                folderName = folderName.substr(pos + 1);
            }
            titleLabel->SetText(folderName.c_str());
        }
        UpdateStatus(folderPath.c_str(), L"");
    };
    
    // 文件扫描完成后的回调，更新文件总数
    fileListView->OnFilesLoaded = [this, fileListView](int count) {
        std::wstring path = fileListView->GetFolderPath();
        if (!path.empty()) {
            UpdateStatus((path + L"  |  " + std::to_wstring(count)).c_str(), L"");
        }
    };
    
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
    closeBtn->Invalidate();
    
    // 在关闭按钮后面添加分隔竖线
    Label* rightSep = new Label(tabContainer);
    tabContainer->Add(rightSep);
    rightSep->SetFixedWidth(2);
    rightSep->SetFixedHeight(30);
    rightSep->Style.BackColor = Color(255, 0, 0);
    rightSep->Margin.Left = 0;
    rightSep->Margin.Right = 0;
    
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
            
            // 从配置中移除文件夹路径
            if (page) {
                for (auto ctl : page->GetControls()) {
                    FileListView* flv = dynamic_cast<FileListView*>(ctl);
                    if (flv) {
                        std::wstring fp = flv->GetFolderPath();
                        if (!fp.empty()) {
                            m_config.RemoveFolder(fp);
                        }
                        break;
                    }
                }
            }
            
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
    titleLabel->EventHandler = [this, tabContainer, fileListView](Control* sender, EventArgs& args) {
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
                // 更新状态栏显示文件夹路径和文件总数
                std::wstring path = fileListView->GetFolderPath();
                if (!path.empty()) {
                    UpdateStatus((path + L"  |  " + std::to_wstring(fileListView->GetFileCount())).c_str(), L"");
                } else {
                    UpdateStatus(L"", L"");
                }
                this->Invalidate();
            }
        }
    };
    
    // 插入 TAB 到 btnAddTab 之前
    int addBtnIndex = tabBar->IndexOf(btnAddTab);
    if (addBtnIndex >= 0) {
        tabBar->Insert(addBtnIndex, tabContainer);
    } else {
        tabBar->Add(tabContainer);
    }
    tabContainer->Invalidate();
    tabButtons.push_back(tabContainer);
    
    UpdateTabButtonStates(newTabIndex);
    
    mainTabs->SetPageIndex(newTabIndex);
    
    UpdateStatus(L"", tabTitle.c_str());
    
    // 最后统一刷新
    this->Refresh();
}

void MainForm::SelectFolderAndLoad(FileListView* fileListView) {
    if (!fileListView) {
        return;
    }
    
    BROWSEINFO bi = { 0 };
    bi.lpszTitle = L"选择文件夹";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != NULL) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDList(pidl, path)) {
            std::wstring folderPath(path);
            m_config.AddFolder(folderPath);
            fileListView->SetFolderPath(folderPath);
            this->Invalidate();
            UpdateStatus(folderPath.c_str(), L"");
        }
        CoTaskMemFree(pidl);
    }
}

void MainForm::SelectFolderAndLoad(FileListView* fileListView, const std::wstring& folderPath) {
    if (!fileListView || folderPath.empty()) {
        return;
    }
    m_config.AddFolder(folderPath);
    fileListView->SetFolderPath(folderPath);
    this->Invalidate();
}

// 辅助函数：递归查找鼠标所在的最深层子控件
// parentX/parentY 是相对于当前 parent 父容器的坐标
static Control* FindControlAtPoint(Control* parent, int parentX, int parentY) {
    if (!parent || !parent->IsVisible()) return nullptr;
    for (auto* child : parent->GetControls()) {
        if (!child->IsVisible()) continue;
        auto cr = child->GetRect();
        if (parentX >= cr.X && parentX < cr.X + cr.Width &&
            parentY >= cr.Y && parentY < cr.Y + cr.Height) {
            // 进入子控件空间：将坐标转换为相对于子控件的
            return FindControlAtPoint(child, parentX - cr.X, parentY - cr.Y);
        }
    }
    return parent;
}

static bool IsInteractiveControl(Control* ctl) {
    if (!ctl) return false;
    // 有事件处理器或 Action 的都是交互控件
    if (ctl->EventHandler || ctl->Action != ControlAction::None) return true;
    // 明确的交互控件类型
    if (dynamic_cast<Button*>(ctl) ||
        dynamic_cast<CheckBox*>(ctl) ||
        dynamic_cast<TextBox*>(ctl) ||
        dynamic_cast<TabLayout*>(ctl) ||
        dynamic_cast<Label*>(ctl)) return true;
    return false;
}

LRESULT MainForm::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_NCHITTEST) {
        LRESULT result = __super::WndProc(uMsg, wParam, lParam);
        if (result == HTCLIENT) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(Hwnd(), &pt);
            // 从 main 容器开始查找鼠标下的控件
            Control* mainContainer = this->FindControl("main");
            if (mainContainer) {
                Control* target = FindControlAtPoint(mainContainer, pt.x, pt.y);
                if (target && IsInteractiveControl(target)) {
                    return HTCLIENT;
                }
            }
            return HTCAPTION;
        }
        return result;
    }
    if (uMsg == WM_DROPFILES) {
        HDROP hDrop = (HDROP)wParam;
        
        UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
        if (fileCount > 0) {
            wchar_t path[MAX_PATH];
            DragQueryFileW(hDrop, 0, path, MAX_PATH);
            
            // 只接受文件夹
            DWORD attr = GetFileAttributesW(path);
            if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
                std::wstring folderPath(path);
                
                // 找到当前显示的 Tab 对应的 FileListView
                FileListView* targetListView = nullptr;
                int curPageIndex = mainTabs->GetPageIndex();
                if (curPageIndex >= m_newTabStartIndex) {
                    int tabIdx = curPageIndex - m_newTabStartIndex;
                    if (tabIdx >= 0 && tabIdx < (int)m_tabPages.size()) {
                        for (auto ctl : m_tabPages[tabIdx]->GetControls()) {
                            FileListView* flv = dynamic_cast<FileListView*>(ctl);
                            if (flv) {
                                targetListView = flv;
                                break;
                            }
                        }
                    }
                }
                
                if (targetListView) {
                    m_config.AddFolder(folderPath);
                    targetListView->SetFolderPath(folderPath);
                    currentFileListView = targetListView;
                    UpdateStatus(folderPath.c_str(), L"");
                    this->Invalidate();
                }
            }
        }
        
        DragFinish(hDrop);
        return 0;
    }
    
    return __super::WndProc(uMsg, wParam, lParam);
}

void MainForm::OnClose(bool& close) {
    Application::Exit(0);
}