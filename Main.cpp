#include "MainForm.h"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    Application app;
    app.EnableHighDpi();
    
    MainForm frm(900, 600);
    frm.SetIcon(IDI_APP_ICON);
    frm.CenterToScreen();
    frm.Show();
    int result = app.Exec();
    
    return result;
}