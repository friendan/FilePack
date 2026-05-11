#pragma once
#include <Windows.h>
#include <chrono>
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

class MainForm : public Window {
private:
    UIManager ui;
    TextBox* logBox = nullptr;
    Button* btnTabLog = nullptr;
    Button* btnToggleLog = nullptr;
    Label* statusLeft = nullptr;
    Label* statusCenter = nullptr;
    Label* statusRight = nullptr;
    Button* btnAddTab = nullptr;
    TabLayout* mainTabs = nullptr;
    HLayout* tabBar = nullptr;
    int tabCount = 1;
    std::vector<Control*> tabButtons;
    std::vector<Control*> m_tabPages;
    FileListView* currentFileListView = nullptr;
    int m_currentTabIndex = 0;
    int m_newTabStartIndex = 0;

public:
    MainForm(int width, int height);
    void Init();
    void AddLog(const std::wstring& message);
    void ClearLog();
    void UpdateTabButtonStates(int selectedIndex);
    void UpdateStatus(const std::wstring& left, const std::wstring& center, const std::wstring& right);
    void AddNewTab();
    void SelectFolderAndLoad(FileListView* fileListView);
    virtual void OnClose(bool& close) override;
    virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};