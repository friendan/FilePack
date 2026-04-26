#include <Windows.h>
#include <Application.h>
#include <Window.h>
#include <VLayout.h>
#include <HLayout.h>
#include <Label.h>
#include <Button.h>
#include <TextBox.h>
#include <CheckBox.h>
#include <VListView.h>
#include <UIManager.h>
#include <TabLayout.h>
#include "resource.h"
#include "AppUtil.hpp"
#include "PathUtil.hpp"
#include "FileListView.hpp"
#include <fstream>
#include <ctime>
#include <thread>
#include <atomic>
#include <shlobj.h>

using namespace ezui;

class MainFrm : public Window {
private:
    UIManager ui;
    TextBox* logBox = nullptr;  // 使用 TextBox，支持文本选择和复制
    Label* statusLeft = nullptr;
    Label* statusCenter = nullptr;
    Label* statusRight = nullptr;
    Button* btnAddTab = nullptr;  // 添加TAB按钮指针
    TabLayout* mainTabs = nullptr;  // 主TAB布局
    HLayout* tabBar = nullptr;  // TAB标签栏
    int tabCount = 1;  // TAB计数，从1开始（日志TAB已存在）
    std::vector<Button*> tabButtons;  // TAB按钮列表

public:
    MainFrm(int width, int height) : Window(width, height) {
        Init();
    }
    
    void Init() {
        this->SetText(L"FilePack");
        
        // 从RC资源加载布局
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
        
        // 创建第一个固定TAB：日志窗口
        Control* logTabPage = new Control(mainTabs);
        logTabPage->Name = L"logTabPage";
        logTabPage->SetDockStyle(DockStyle::Fill);
        logTabPage->Style.BackColor = Color(144, 238, 144);  // 淡绿色背景
        
        // 创建TextBox - 参考demo的方式
        logBox = new TextBox();
        logBox->SetParent(logTabPage);
        logBox->SetDockStyle(DockStyle::Fill);
        logBox->Name = L"logBox";
        logBox->SetMultiLine(true);
        logBox->SetReadOnly(true);
        logBox->Style.BackColor = Color(144, 238, 144);  // 淡绿色背景
        logBox->Style.ForeColor = Color(255, 0, 0);  // 红色文字
        logBox->Style.FontSize = 12;
        
        // 立即设置测试文本
        logBox->SetText(L"TEST TEXT - TextBox is working!");
        
        // 添加到TAB布局
        mainTabs->Add(logTabPage);
        
        // 先添加一个临时页面，确保TabLayout有内容
        Control* tempPage = new Control(mainTabs);
        tempPage->SetDockStyle(DockStyle::Fill);
        tempPage->Style.BackColor = Color(200, 200, 200);  // 灰色
        mainTabs->Add(tempPage);
        
        // 强制刷新布局
        mainTabs->RefreshLayout();
        this->Refresh();
        
        // 切换回第一个TAB（日志TAB）
        mainTabs->SetPageIndex(0);
        mainTabs->RefreshLayout();
        
        // 移除临时页面
        mainTabs->Remove(tempPage, true);
        mainTabs->RefreshLayout();
        
        // 测试日志功能
        AddLog(L"Program started");
        AddLog(L"Log system initialized");
        
        // 绑定日志TAB按钮事件
        Button* btnTabLog = (Button*)this->FindControl("btnTabLog");
        if (btnTabLog) {
            tabButtons.push_back(btnTabLog);
            btnTabLog->EventHandler = [this](Control* sender, EventArgs& args) {
                if (args.EventType == Event::OnMouseDown) {
                    mainTabs->SetPageIndex(0);
                }
            };
        }
        
        // 绑定添加TAB按钮事件
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
    
    void AddLog(const std::wstring& message) {
        // 使用 AppUtil::SaveLog 统一记录日志
        AppUtil::SaveLog("[FilePack UI] ", AppUtil::WStrToStr(message));
        
        if (logBox) {
            // 获取当前时间
            time_t now = time(0);
            tm ltm;
            localtime_s(&ltm, &now);
            wchar_t timeStr[32];
            swprintf_s(timeStr, L"%04d-%02d-%02d %02d:%02d:%02d", 
                ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday,
                ltm.tm_hour, ltm.tm_min, ltm.tm_sec);
            
            // 组合时间戳和消息
            std::wstring fullMessage = std::wstring(timeStr) + L" " + message + L"\r\n";
            
            // 获取当前文本并追加新日志
            std::wstring currentText = AppUtil::StrToWStr(logBox->GetText().c_str());
            logBox->SetText((currentText + fullMessage).c_str());
        }
    }
    
    void ClearLog() {
        if (logBox) {
            logBox->SetText(L"");
        }
    }
    
    void UpdateStatus(const std::wstring& left, const std::wstring& center, const std::wstring& right) {
        if (statusLeft) statusLeft->SetText(left);
        if (statusCenter) statusCenter->SetText(center);
        if (statusRight) statusRight->SetText(right);
    }
    
    void AddNewTab() {
        tabCount++;
        std::wstring tabTitle = L"文件列表 " + std::to_wstring(tabCount);
        
        AddLog(L"Creating new tab: " + tabTitle);
        
        // 创建新的TAB页面
        Control* tabPage = new Control(mainTabs);
        tabPage->Name = L"tabPage" + std::to_wstring(tabCount);
        tabPage->SetDockStyle(DockStyle::Fill);
        
        // 创建文件列表视图
        FileListView* fileListView = new FileListView(tabPage);
        fileListView->SetDockStyle(DockStyle::Fill);
        
        AddLog(L"FileListView created");
        
        // 设置双击和拖放事件 - 绑定到fileListView而不是tabPage
        fileListView->EventHandler = [this, fileListView](Control* sender, EventArgs& args) {
            if (args.EventType == Event::OnMouseDoubleClick) {
                AddLog(L"Double click detected on FileListView");
                SelectFolderAndLoad(fileListView);
            }
        };
        
        // 添加到TAB布局
        mainTabs->Add(tabPage);
        
        AddLog(L"Tab added to TabLayout");
        
        // 创建TAB按钮
        Button* newTabBtn = new Button(tabBar);
        newTabBtn->SetText(tabTitle.c_str());
        newTabBtn->SetFixedWidth(100);
        
        int newTabIndex = mainTabs->GetControls().size() - 1;
        newTabBtn->EventHandler = [this, newTabIndex](Control* sender, EventArgs& args) {
            if (args.EventType == Event::OnMouseDown) {
                mainTabs->SetPageIndex(newTabIndex);
            }
        };
        
        tabBar->Add(newTabBtn);
        tabButtons.push_back(newTabBtn);
        
        // 切换到新添加的TAB
        mainTabs->SetPageIndex(newTabIndex);
        
        AddLog(L"Switched to new tab");
        UpdateStatus(L"已添加新TAB", tabTitle.c_str(), L"");
    }
    
    void SelectFolderAndLoad(FileListView* fileListView) {
        // 打开文件夹选择对话框
        BROWSEINFO bi = { 0 };
        bi.lpszTitle = L"选择文件夹";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        
        LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
        if (pidl != NULL) {
            wchar_t path[MAX_PATH];
            if (SHGetPathFromIDList(pidl, path)) {
                std::wstring folderPath(path);
                fileListView->SetFolderPath(folderPath);
                
                AddLog(L"Selected folder: " + folderPath);
                UpdateStatus(L"已选择文件夹", folderPath.c_str(), L"");
            }
            CoTaskMemFree(pidl);
        }
    }
    
    virtual void OnClose(bool& close) override {
        Application::Exit(0);
    }
};

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    Application app;
    app.EnableHighDpi();
    
    MainFrm frm(900, 600);
    frm.SetIcon(IDI_APP_ICON);  // 设置窗口图标
    frm.CenterToScreen();
    frm.Show();
    int result = app.Exec();
    
    return result;
}
