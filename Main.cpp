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
#include "resource.h"
#include "AppUtil.hpp"
#include "PathUtil.hpp"
#include <fstream>
#include <ctime>
#include <thread>
#include <atomic>

using namespace ezui;

class MainFrm : public Window {
private:
    UIManager ui;
    TextBox* logBox = nullptr;  // 使用 TextBox，支持文本选择和复制
    Label* statusLeft = nullptr;
    Label* statusCenter = nullptr;
    Label* statusRight = nullptr;
    Button* btnRecognize = nullptr;  // 识别按钮指针

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
        logBox = (TextBox*)this->FindControl("logBox");
        statusLeft = (Label*)this->FindControl("statusLeft");
        statusCenter = (Label*)this->FindControl("statusCenter");
        statusRight = (Label*)this->FindControl("statusRight");
        btnRecognize = (Button*)this->FindControl("btnRecognize");
        
        if (logBox) {
            logBox->Style.FontSize = 12;
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
    
    virtual void OnClose(bool& close) override {
        Application::Exit(0);
    }
};

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // 初始化GDI+
    DrawGrid::Inst()->InitGdiPlus();
    Application app;
    app.EnableHighDpi();
    
    MainFrm frm(900, 600);
    frm.SetIcon(IDI_APP_ICON);  // 设置窗口图标
    frm.CenterToScreen();
    frm.Show();
    int result = app.Exec();
    
    // 卸载GDI+
    DrawGrid::Inst()->UninitGdiPlus();
    return result;
}
