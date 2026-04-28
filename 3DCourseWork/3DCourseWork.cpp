// 3DCourseWork.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "3DCourseWork.h"
#include <windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <vector>
#include <string>
#include <cmath>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

#define MAX_LOADSTRING 100

// Глобальные переменные:
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна

HDC g_hDC = nullptr;                            //контекст устройства
HGLRC g_hRC = nullptr;                          //контекст OpenGL
HWND g_hWnd = nullptr;                          //главное окно

int g_width = 600;
int g_height = 800;

float g_rotateX = 0.0f;
float g_rotateY = 0.0f;
float g_rotateZ = 0.0f;

float g_distance = 7.0f;

bool g_mouseLeftDown = false;
int g_lastMouseX = 0;
int g_lastMouseY = 0;






// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
bool InitOpenGL(HWND hWnd);
void CleanupOpenGL();
void ResizeOpenGL(int width, int height);
void RenderScene();
void DrawAxes();

static bool FileExistsA(const std::string& path);
static std::string GetCurrentDirectoryStringA();
static std::string DirectoryOfA(const std::string& fullPath);
static std::string GetExeDirectoryA();
static std::string JoinPathA(const std::string& a, const std::string& b);
static std::string FindModelPath(const std::string& localPath);



//структуры для хранения OBJ моделей
struct Vec3
{
    float x;
    float y;
    float z;

    Vec3()
    {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    }

    Vec3(float X, float Y, float Z)
    {
        x = X;
        y = Y;
        z = Z;
    }
};

static Vec3 operator-(const Vec3& a, const Vec3& b)
{
    return Vec3(
        a.x - b.x,
        a.y - b.y,
        a.z - b.z
    );
}

static Vec3 Cross(const Vec3& a, const Vec3& b)
{
    return Vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

static Vec3 Normalize(const Vec3& v)
{
    float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);

    if (length < 0.000001f)
    {
        return Vec3(0.0f, 0.0f, 1.0f);
    }

    return Vec3(
        v.x / length,
        v.y / length,
        v.z / length
    );
}

struct Vertex
{
    Vec3 position;
    Vec3 normal;
};

struct ObjModel
{
    std::string name;
    std::vector<Vertex> vertices;
    bool loaded;

    ObjModel()
    {
        loaded = false;
    }
};

ObjModel g_body;
ObjModel g_leftUpperArm;
ObjModel g_leftForearm;
ObjModel g_leftHand;
ObjModel g_rightUpperArm;
ObjModel g_rightForearm;
ObjModel g_rightHand;

static bool FileExistsA(const std::string& path)
{
    DWORD attr = GetFileAttributesA(path.c_str());

    return attr != INVALID_FILE_ATTRIBUTES &&
        !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

static std::string GetCurrentDirectoryStringA()
{
    char buffer[MAX_PATH];

    DWORD length = GetCurrentDirectoryA(MAX_PATH, buffer);

    if (length == 0 || length >= MAX_PATH)
    {
        return ".";
    }

    return std::string(buffer);
}

static std::string DirectoryOfA(const std::string& fullPath)
{
    size_t position = fullPath.find_last_of("\\/");

    if (position == std::string::npos)
    {
        return ".";
    }

    return fullPath.substr(0, position);
}

static std::string GetExeDirectoryA()
{
    char buffer[MAX_PATH];

    DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);

    if (length == 0 || length >= MAX_PATH)
    {
        return ".";
    }

    return DirectoryOfA(std::string(buffer));
}

static std::string JoinPathA(const std::string& a, const std::string& b)
{
    if (a.empty())
    {
        return b;
    }

    char last = a[a.size() - 1];

    if (last == '\\' || last == '/')
    {
        return a + b;
    }

    return a + "\\" + b;
}

static std::string FindModelPath(const std::string& localPath)
{
    std::string currentDir = GetCurrentDirectoryStringA();
    std::string exeDir = GetExeDirectoryA();

    std::vector<std::string> candidates;

    candidates.push_back(localPath);
    candidates.push_back(JoinPathA(currentDir, localPath));
    candidates.push_back(JoinPathA(exeDir, localPath));

    candidates.push_back(JoinPathA(currentDir, "x64\\Debug\\" + localPath));
    candidates.push_back(JoinPathA(currentDir, "Debug\\" + localPath));
    candidates.push_back(JoinPathA(currentDir, "x64\\Release\\" + localPath));
    candidates.push_back(JoinPathA(currentDir, "Release\\" + localPath));

    candidates.push_back(JoinPathA(exeDir, "..\\" + localPath));
    candidates.push_back(JoinPathA(exeDir, "..\\..\\" + localPath));
    candidates.push_back(JoinPathA(currentDir, "..\\" + localPath));
    candidates.push_back(JoinPathA(currentDir, "..\\..\\" + localPath));

    for (size_t i = 0; i < candidates.size(); i++)
    {
        if (FileExistsA(candidates[i]))
        {
            return candidates[i];
        }
    }

    return "";
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Разместите код здесь.

    // Инициализация глобальных строк
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MY3DCOURSEWORK, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Выполнить инициализацию приложения:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY3DCOURSEWORK));

    MSG msg;

    // Цикл основного сообщения:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  ФУНКЦИЯ: MyRegisterClass()
//
//  ЦЕЛЬ: Регистрирует класс окна.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY3DCOURSEWORK));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MY3DCOURSEWORK);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   ФУНКЦИЯ: InitInstance(HINSTANCE, int)
//
//   ЦЕЛЬ: Сохраняет маркер экземпляра и создает главное окно
//
//   КОММЕНТАРИИ:
//
//        В этой функции маркер экземпляра сохраняется в глобальной переменной, а также
//        создается и выводится главное окно программы.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Сохранить маркер экземпляра в глобальной переменной

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
       CW_USEDEFAULT, 0, 1000, 700, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }
   g_hWnd = hWnd;

   if (!InitOpenGL(hWnd))
   {
       MessageBox(hWnd, L"Не удалось инициализировать OpenGL", L"Ошибка", MB_OK | MB_ICONERROR);
       return FALSE; 
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  ФУНКЦИЯ: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ЦЕЛЬ: Обрабатывает сообщения в главном окне.
//
//  WM_COMMAND  - обработать меню приложения
//  WM_PAINT    - Отрисовка главного окна
//  WM_DESTROY  - отправить сообщение о выходе и вернуться
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Разобрать выбор в меню:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WM_KEYDOWN:
    {
        switch (wParam)
        {
        case 'X':
            g_rotateX += 5.0f;
            break;

        case 'Y':
            g_rotateY += 5.0f;
            break;

        case 'Z':
            g_rotateZ += 5.0f;
            break;

        case 'R':
            g_rotateX = 0.0f;
            g_rotateY = 0.0f;
            g_rotateZ = 0.0f;
            g_distance = 7.0f;
            break;

        case VK_ESCAPE:
            DestroyWindow(hWnd);
            break;
        }
        InvalidateRect(hWnd, nullptr, FALSE);
    }
    break;

    case WM_LBUTTONDOWN:
        {
            g_mouseLeftDown = true;

            g_lastMouseX = LOWORD(lParam);
            g_lastMouseY = HIWORD(lParam);

            SetCapture(hWnd);
        }
        break;

    case WM_LBUTTONUP:
        {
            g_mouseLeftDown = false;
            ReleaseCapture();
        }
        break;

    case WM_MOUSEMOVE:
    {
        if (g_mouseLeftDown)
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            int dx = x - g_lastMouseX;
            int dy = y - g_lastMouseY;

            g_rotateZ += dx * 0.5f;
            g_rotateX += dy * 0.5f;

            g_lastMouseX = x;
            g_lastMouseY = y;

            InvalidateRect(hWnd, nullptr, FALSE);
        }
    }
    break;

    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);

        if (delta > 0)
        {
            g_distance -= 0.5f;
        }
        else
        {
            g_distance += 0.5f;
        }

        if (g_distance < 2.0f)
        {
            g_distance = 2.0f;
        }

        if (g_distance > 20.0f)
        {
            g_distance = 20.0f;
        }

        InvalidateRect(hWnd, nullptr, FALSE);
    }
    break;

    case WM_SIZE:
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            ResizeOpenGL(width, height);
        }
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            RenderScene();
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_DESTROY:
            CleanupOpenGL();
            PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Обработчик сообщений для окна "О программе".
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

bool InitOpenGL(HWND hWnd)
{
    g_hDC = GetDC(hWnd);
    
    if (!g_hDC)
    {
        return FALSE;
    }

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;

    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;

    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;

    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(g_hDC, &pfd);

    if (pixelFormat == 0)
    {
        return false;
    }

    if (!SetPixelFormat(g_hDC, pixelFormat, &pfd))
    {
        return false;
    }

    g_hRC = wglCreateContext(g_hDC);

    if (!g_hRC)
    {
        return false;
    }

    if (!wglMakeCurrent(g_hDC, g_hRC))
    {
        return false;
    }

    glEnable(GL_DEPTH_TEST);

    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);

    ResizeOpenGL(g_width, g_height);

    std::string testPath = FindModelPath("models\\robot_body_static.obj");

    if (testPath.empty())
    {
        MessageBoxA(hWnd, "Model file not found", "OBJ check", MB_OK | MB_ICONWARNING);
    }
    else
    {
        MessageBoxA(hWnd, testPath.c_str(), "OBJ file found", MB_OK | MB_ICONINFORMATION);
    }

    return true;

    
}

void CleanupOpenGL()
{
    if (g_hRC)
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(g_hRC);
        g_hRC = nullptr;
    }

    if (g_hDC && g_hWnd)
    {
        ReleaseDC(g_hWnd, g_hDC);
        g_hDC = nullptr;
    }
}

void ResizeOpenGL(int width, int height)
{
    if (height == 0)
    {
        height = 1;
    }

    g_width = width;
    g_height = height;

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(
        45.0,
        (double)width / (double)height,
        0.1,
        100.0
    );

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void RenderScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

    gluLookAt(
        4.0, -g_distance, 4.0,
        0.0, 0.0, 0.0,
        0.0, 0.0, 1.0
    );

    glRotatef(g_rotateX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_rotateY, 0.0f, 1.0f, 0.0f);
    glRotatef(g_rotateZ, 0.0f, 0.0f, 1.0f);

    DrawAxes();

    SwapBuffers(g_hDC);
}

void DrawAxes()
{
    glLineWidth(3.0f);

    glBegin(GL_LINES);

    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-2.0f, 0.0f, 0.0f);
    glVertex3f(2.0f, 0.0f, 0.0f);

    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, -2.0f, 0.0f);
    glVertex3f(0.0f, 2.0f, 0.0f);

    glColor3f(0.0f, 0.3f, 1.0f);
    glVertex3f(0.0f, 0.0f, -2.0f);
    glVertex3f(0.0f, 0.0f, 2.0f);

    glEnd();
    glLineWidth(1.0f);
}
