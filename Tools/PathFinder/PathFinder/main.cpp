#pragma once
// PathFind_Astar.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//
#pragma once
#include "resource.h"
#include "framework.h"
#include "CJPS.h"
#include "CAstar.h"
#include "Windowsx.h"
#include "CTlsProfile.h"
#include "CParser.h"
#include "CCrashDump.h"

#define MAX_LOADSTRING 100

HBITMAP hMemDCBitmap;
HBITMAP hMemDCBitmap_old;
HDC hMemDC;
RECT MemDCRect;
HWND hWnd; //저장용 변수

// 전역 변수:
/////////////////////////////////////

CJPS* g_JPS;
CAstar* g_Astar;

bool g_bErase = false;
bool g_bDrag = false;
bool g_bScroll = false;
int g_iType = CPathFinder::ASTAR;
int ScreenX;
int ScreenY;

bool g_bClick;
HPEN g_hPen;
int g_iOldX;
int g_iOldY;



HBITMAP g_hMemDCBitmap;
HBITMAP g_hMemDCBitmap_old;
HDC g_hMemDC;
RECT g_MemDCRect;
HWND g_hWnd; //저장용 변수

bool g_Pathfind = false;

///////////////////////////////////

int sX = -1;
int sY = -1;
int eX = -1;
int eY = -1;

HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PATHFINDER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    srand(time(NULL));
    g_JPS = new CJPS;
    g_Astar = new CAstar;
    g_JPS->Initialize();
    g_Astar->Initialize();

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PATHFINDER));

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PATHFINDER));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_PATHFINDER);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));



    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, (24) * GRID_WIDTH, (24) * GRID_HEIGHT, nullptr, nullptr, hInstance, nullptr);

    g_hWnd = hWnd;

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    int xPos;
    int yPos;
    int iTileX;
    int iTileY;
    CPathFinder* PathFinder;

    if (g_iType == CPathFinder::ASTAR)
    {
        PathFinder = (CPathFinder*)g_Astar;
    }
    else
    {
        PathFinder = (CPathFinder*)g_JPS;
    }

    switch (message)
    {

    case WM_CREATE:
        hdc = GetDC(hWnd);
        GetClientRect(hWnd, &g_MemDCRect);
        g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_MemDCRect.right, g_MemDCRect.bottom);
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);
        g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);

        break;

    case WM_LBUTTONDOWN:
        g_bDrag = true;

        xPos = GET_X_LPARAM(lParam);
        yPos = GET_Y_LPARAM(lParam);

        iTileX = xPos / PathFinder->GetGridSize();
        iTileY = yPos / PathFinder->GetGridSize();

        if (iTileX < PathFinder->GetWidth() && iTileY < PathFinder->GetHeight())
        {
            if (PathFinder->GetGridStatus(iTileX, iTileY) == CPathFinder::Node::BLOCK)
            {
                g_bErase = true;
            }
            else
            {
                g_bErase = false;
            }
        }

        break;

    case WM_LBUTTONUP:
        g_bDrag = false;
        break;

    case WM_MBUTTONDOWN:
        xPos = GET_X_LPARAM(lParam);
        yPos = GET_Y_LPARAM(lParam);

        if (sX != -1 && sY != -1)
        {
            g_JPS->SetBlank(sX, sY);
            g_Astar->SetBlank(sX, sY);
        }
        sX = xPos / PathFinder->GetGridSize();
        sY = yPos / PathFinder->GetGridSize();
        g_JPS->SetStart(sX, sY);
        g_Astar->SetStart(sX, sY);

        InvalidateRect(hWnd, NULL, false);
        break;

    case WM_RBUTTONDOWN:
        xPos = GET_X_LPARAM(lParam);
        yPos = GET_Y_LPARAM(lParam);

        if (eX != -1 && eY != -1)
        {
            g_JPS->SetBlank(eX, eY);
            g_Astar->SetBlank(eX, eY);
        }
        eX = xPos / PathFinder->GetGridSize();
        eY = yPos / PathFinder->GetGridSize();
        g_JPS->SetEnd(eX, eY);
        g_Astar->SetEnd(eX, eY);

        InvalidateRect(hWnd, NULL, false);
        break;


    case WM_MOUSEMOVE:

        xPos = GET_X_LPARAM(lParam);
        yPos = GET_Y_LPARAM(lParam);

        if (g_bDrag)
        {
            iTileX = xPos / PathFinder->GetGridSize();
            iTileY = yPos / PathFinder->GetGridSize();

            if (iTileX < PathFinder->GetWidth() && iTileY < PathFinder->GetHeight())
            {
                if (g_bErase == TRUE)
                {
                    g_JPS->SetBlank(iTileX, iTileY);
                    g_Astar->SetBlank(iTileX, iTileY);

                }
                else
                {
                    g_JPS->SetBlock(iTileX, iTileY);
                    g_Astar->SetBlock(iTileX, iTileY);
                }
                InvalidateRect(hWnd, NULL, false);
            }

        }
        break;

    case WM_KEYDOWN:

        switch (wParam)
        {
        case VK_RETURN:
            if (sX != -1 && sY != -1 && eX != -1 && eY != -1)
            {
                PathFinder->ClearPath();
                PathFinder->PathFind(sX, sY, eX, eY);
                //while (PathFinder->PathFindByStep(sX, sY, eX, eY))
                //{
                InvalidateRect(hWnd, NULL, false);
                //}

            }
            break;
        case VK_SPACE:
            if (sX != -1 && sY != -1 && eX != -1 && eY != -1)
            {
                PathFinder->ClearPath();
                PathFinder->PathFindByStep(sX, sY, eX, eY);
                while (PathFinder->PathFindByStepUpdate())
                {
                    InvalidateRect(hWnd, NULL, false);
                    UpdateWindow(g_hWnd);
                }
                InvalidateRect(hWnd, NULL, false);

            }
            break;

        case VK_BACK:

            sX = -1;
            eX = -1;
            g_JPS->Clear();
            g_Astar->Clear();
            InvalidateRect(hWnd, NULL, false);
            g_Pathfind = false;
            break;

        case VK_CONTROL:
            g_bScroll = true;
            break;

        case VK_UP:
            if (ScreenY > 5)
            {
                g_JPS->SetMapY(-5);
                InvalidateRect(hWnd, NULL, false);
            }
            break;
        case VK_DOWN:
            if (ScreenY < g_MemDCRect.bottom)
            {
                g_JPS->SetMapY(5);
                InvalidateRect(hWnd, NULL, false);
            }
            break;
        case VK_LEFT:
            if (ScreenX > 5)
            {
                g_JPS->SetMapX(-5);
                InvalidateRect(hWnd, NULL, false);
            }
            break;
        case VK_RIGHT:
            if (ScreenX < g_MemDCRect.right)
            {
                g_JPS->SetMapX(5);
                InvalidateRect(hWnd, NULL, false);
            }
            break;

        case 0x5A: //z
            g_JPS->ClearBlock();
            g_Astar->ClearBlock();
            InvalidateRect(hWnd, NULL, false);
            break;

        case 0x58: //x
            g_JPS->ClearPath();
            g_Astar->ClearPath();
            InvalidateRect(hWnd, NULL, false);
            break;

        case 0x31: //1
            g_iType = CPathFinder::ASTAR;
            g_JPS->ClearPath();
            g_Astar->ClearPath();
            InvalidateRect(hWnd, NULL, false);
            break;

        case 0x32: //2
            g_iType = CPathFinder::JPS;
            g_JPS->ClearPath();
            g_Astar->ClearPath();
            InvalidateRect(hWnd, NULL, false);
            break;
        case 0x47: //G
            g_Astar->SetGForm();
            InvalidateRect(hWnd, NULL, false);
            break;
        case 0x48: //H
            g_Astar->SetHForm();
            InvalidateRect(hWnd, NULL, false);
            break;

        case 0x50: //p
            for (int i = 0; i < 100; i++)
            {
                PRO_BEGIN("JPS");
                for (int j = 0; j < 100; j++)
                {
                    g_JPS->ClearPath();
                    g_JPS->PathFind(sX, sY, eX, eY);
                }
                PRO_END("JPS");

            }
            for (int i = 0; i < 100; i++)
            {
                PRO_BEGIN("Astar");
                for (int j = 0; j < 100; j++)
                {
                    g_Astar->ClearPath();
                    g_Astar->PathFind(sX, sY, eX, eY);
                }
                PRO_END("Astar");
            }
            g_iType = CPathFinder::JPS;
            CTlsProfile::ProfileDataOutText();
            CTlsProfile::ProfileReset();
            InvalidateRect(hWnd, NULL, false);
            break;

            break;
        case VK_HOME:
            int iBlock;
            int x;
            int y;
            CParser Parse;
            Parse.LoadFile(L"PathFind.txt");
            Parse.GetValue(L"PathFind", L"BlockCount", iBlock);

            for (int i = 0; i < iBlock; i++)
            {
                x = rand() % (PathFinder->GetWidth() - 1);
                y = rand() % (PathFinder->GetHeight() - 1);
                g_JPS->SetBlock(x, y);
                g_Astar->SetBlock(x, y);
            }

            InvalidateRect(hWnd, NULL, false);
            break;

            break;
        }
    case WM_KEYUP:
        switch (wParam)
        {
        case VK_CONTROL:
            g_bScroll = false;
            break;
        }
        break;
        
        InvalidateRect(hWnd, NULL, false);
        break;

    case WM_MOUSEWHEEL: //마우스 휠동작 메시지
        if (1)
        {
            SHORT size = (SHORT)HIWORD(wParam);
            if (size > 0) //마우스휠을 올릴 경우
            {
                g_JPS->IncreaseGridSize();
                g_Astar->IncreaseGridSize();
                InvalidateRect(hWnd, NULL, false);
            }
            else  //마우스휠을 내릴 경우
            {
                g_JPS->DecreaseGridSize();
                g_Astar->DecreaseGridSize();
                InvalidateRect(hWnd, NULL, false);
            }
        }
        break;

    case WM_PAINT:

        PatBlt(g_hMemDC, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, WHITENESS);
        PathFinder->Render(g_hMemDC);

        hdc = BeginPaint(hWnd, &ps);
        BitBlt(hdc, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, g_hMemDC, 0, 0, SRCCOPY);
        EndPaint(hWnd, &ps);

        break;

    case WM_SIZE:
        SelectObject(g_hMemDC, g_hMemDCBitmap_old);
        DeleteObject(g_hMemDC);
        DeleteObject(g_hMemDCBitmap);

        hdc = GetDC(hWnd);

        GetClientRect(hWnd, &g_MemDCRect);
        g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_MemDCRect.right, g_MemDCRect.bottom);
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);

        g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);

        break;

    case WM_DESTROY:
        SelectObject(g_hMemDC, g_hMemDCBitmap_old);
        DeleteObject(g_hMemDC);
        DeleteObject(g_hMemDCBitmap);


        PostQuitMessage(0);
        break;


    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
