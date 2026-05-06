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
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iomanip>

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

bool g_armAnimation = false;
bool g_wireframe  = false;
bool g_autoRotate = false;

float g_animTime = 0.0f;
DWORD g_lastTime = 0;

float g_fps = 0.0f;
int g_frameCount = 0;
DWORD g_lastFpsTime = 0;

float g_leftShoulderX = 0.0f;
float g_leftShoulderY = 0.0f;
float g_leftShoulderZ = 0.0f;
float g_leftElbowY = 0.0f;
float g_leftWristX = 0.0f;
float g_leftWristZ = 0.0f;

float g_rightShoulderX = 0.0f;
float g_rightShoulderY = 0.0f;
float g_rightShoulderZ = 0.0f;
float g_rightElbowY = 0.0f;
float g_rightWristX = 0.0f;
float g_rightWristZ = 0.0f;

GLuint g_fontBase = 0;
HFONT g_hFont = nullptr;

int g_screenshotIndex = 1;







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
void UpdateAnimation();
void InitLighting();
void SetupLight();

void UpdateFPS();

bool InitFont();
void CleanupFont();
void DrawText2D(float x, float y, const std::string& text);
void DrawInfoText();

bool SaveScreenshot(const std::string& fileName);

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

void DrawObjModel(const ObjModel& model);
void DrawRobot();

static int ParseObjIndex(const std::string& token);
static bool LoadObjModel(const std::string& localPath, ObjModel& model);

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

static int ParseObjIndex(const std::string& token)
{
    size_t slashPos = token.find('/');

    std::string numberPart;

    if (slashPos == std::string::npos)
    {
        numberPart = token;
    }
    else
    {
        numberPart = token.substr(0, slashPos);
    }

    return std::atoi(numberPart.c_str());
}

static bool LoadObjModel(const std::string& localPath, ObjModel& model)
{
    std::string path = FindModelPath(localPath);

    if (path.empty())
    {
        model.loaded = false;
        return false;
    }

    std::ifstream file(path);

    if (!file.is_open())
    {
        model.loaded = false;
        return false;
    }

    model.name = localPath;
    model.vertices.clear();
    model.loaded = false; 

    std::vector<Vec3> positions;
    std::string line;

    while (std::getline(file, line))
    {
        if (line.size() < 3)
        {
            continue;
        }

        if (line[0] == 'v' && line[1] == ' ')
        {
            std::stringstream ss(line);

            char prefix;
            float x;
            float y;
            float z;
            ss >> prefix >> x >> y >> z;

            positions.push_back(Vec3(x, y, z));
        }
        else if (line[0] == 'f' && line[1] == ' ')
        {
            std::stringstream ss(line);

            char prefix;
            ss >> prefix;

            std::vector<int> faceIndices;
            std::string token;

            while (ss >> token)
            {
                int index = ParseObjIndex(token);

                if (index > 0 && index <= (int)positions.size())
                {
                    faceIndices.push_back(index - 1);
                }
            }

            if (faceIndices.size() >= 3)
            {
                for (size_t i = 1; i + 1 < faceIndices.size(); i++)
                {
                    Vec3 p1 = positions[faceIndices[0]];
                    Vec3 p2 = positions[faceIndices[i]];
                    Vec3 p3 = positions[faceIndices[i + 1]];

                    Vec3 normal = Normalize(Cross(p2 - p1, p3 - p1));

                    Vertex v1;
                    v1.position = p1;
                    v1.normal = normal;

                    Vertex v2;
                    v2.position = p2;
                    v2.normal = normal;

                    Vertex v3;
                    v3.position = p3;
                    v3.normal = normal;

                    model.vertices.push_back(v1);
                    model.vertices.push_back(v2);
                    model.vertices.push_back(v3);
                }
            }
        }
    }

    model.loaded = !model.vertices.empty();

    return model.loaded;
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

    case WM_TIMER:
    {
        UpdateAnimation();
        InvalidateRect(hWnd, nullptr, FALSE);
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

        case 'W':
            g_wireframe = !g_wireframe;
            break;

        case 'A':
            g_autoRotate = !g_autoRotate;
            break;

        case 'M':
            g_armAnimation = !g_armAnimation;
            break;

        case 'R':
            g_rotateX = 0.0f;
            g_rotateY = 0.0f;
            g_rotateZ = 0.0f;
            g_distance = 7.0f;

            g_leftShoulderX = 0.0f;
            g_leftShoulderY = 0.0f;
            g_leftShoulderZ = 0.0f;
            g_leftElbowY = 0.0f;
            g_leftWristX = 0.0f;
            g_leftWristZ = 0.0f;

            g_rightShoulderX = 0.0f;
            g_rightShoulderY = 0.0f;
            g_rightShoulderZ = 0.0f;
            g_rightElbowY = 0.0f;
            g_rightWristX = 0.0f;
            g_rightWristZ = 0.0f;
            g_wireframe = false;
            break;

        case 'S':
        {
            std::ostringstream fileName;
            fileName << "screenshot_" << g_screenshotIndex << ".bmp";

            if (SaveScreenshot(fileName.str()))
            {
                ++g_screenshotIndex;
                MessageBoxA(g_hWnd, "Screenshot saved", "Info", MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                MessageBoxA(g_hWnd, "Failed to save screenshot", "Error", MB_OK | MB_ICONERROR);
            }

            break;
        }

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

    InitLighting();

    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);

    ResizeOpenGL(g_width, g_height);

    InitFont();
    
    bool modelsLoaded = true;

    modelsLoaded &= LoadObjModel("models\\robot_body_static.obj", g_body);
    modelsLoaded &= LoadObjModel("models\\left_upper_arm.obj", g_leftUpperArm);
    modelsLoaded &= LoadObjModel("models\\left_forearm.obj", g_leftForearm);
    modelsLoaded &= LoadObjModel("models\\left_hand.obj", g_leftHand);

    modelsLoaded &= LoadObjModel("models\\right_upper_arm.obj", g_rightUpperArm);
    modelsLoaded &= LoadObjModel("models\\right_forearm.obj", g_rightForearm);
    modelsLoaded &= LoadObjModel("models\\right_hand.obj", g_rightHand);

    if (!modelsLoaded)
    {
        MessageBoxA(
            hWnd,
            "Не все OBJ-модели были загружены. Проверь папку models и названия файлов.",
            "OBJ load",
            MB_OK | MB_ICONWARNING
        );
    }

    g_lastTime = GetTickCount();
    g_lastFpsTime = g_lastTime;

    SetTimer(hWnd, 1, 16, nullptr);

    return true;
}

void CleanupOpenGL()
{
    CleanupFont();

    if (g_hWnd)
    {
        KillTimer(g_hWnd, 1);
    }

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

void CleanupFont()
{
    if (g_fontBase != 0)
    {
        glDeleteLists(g_fontBase, 96);
        g_fontBase = 0;
    }

    if (g_hFont)
    {
        DeleteObject(g_hFont);
        g_hFont = nullptr;
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

void InitLighting()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    glEnable(GL_NORMALIZE);

    GLfloat ambientLight[] = { 0.25f, 0.25f, 0.25f, 1.0f };
    GLfloat diffuseLight[] = { 0.85f, 0.85f, 0.85f, 1.0f };
    GLfloat specularLight[] = { 0.35f, 0.35f, 0.35f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);

    GLfloat materialSpecular[] = { 0.35f, 0.35f, 0.35f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32.0f);
}

bool InitFont()
{
    g_fontBase = glGenLists(96);

    g_hFont = CreateFontA(
        -16,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_TT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        FF_DONTCARE | DEFAULT_PITCH,
        "Consolas"
    );

    if (!g_hFont)
    {
        return false;
    }

    SelectObject(g_hDC, g_hFont);

    return wglUseFontBitmapsA(g_hDC, 32, 96, g_fontBase) == TRUE;
}

void SetupLight()
{
    GLfloat lightPosition[] = { 4.0f, -5.0f, 7.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
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

    SetupLight();

    glPolygonMode(GL_FRONT_AND_BACK, g_wireframe ? GL_LINE : GL_FILL);

    glRotatef(g_rotateX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_rotateY, 0.0f, 1.0f, 0.0f);
    glRotatef(g_rotateZ, 0.0f, 0.0f, 1.0f);

    DrawAxes();

    DrawRobot();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    UpdateFPS();
    DrawInfoText();

    SwapBuffers(g_hDC);
}

void UpdateAnimation()
{
    DWORD now = GetTickCount();

    if (g_lastTime == 0)
    {
        g_lastTime = now;
    }

    float deltaTime = (now - g_lastTime) / 1000.0f;
    g_lastTime = now;

    if (deltaTime > 0.1f)
    {
        deltaTime = 0.1f;
    }

    g_animTime += deltaTime;

    if (g_autoRotate)
    {
        g_rotateZ += 20.0f * deltaTime;

        if (g_rotateZ > 360.0f)
        {
            g_rotateZ -= 360.0f;
        }
    }

    if (g_armAnimation)
    {
        g_leftShoulderX = 25.0f * sinf(g_animTime * 1.4f);
        g_leftShoulderY = 15.0f * sinf(g_animTime * 0.9f);
        g_leftShoulderZ = -20.0f + 15.0f * sinf(g_animTime * 1.1f);

        g_leftElbowY = 45.0f + 30.0f * sinf(g_animTime * 1.8f);

        g_leftWristX = 20.0f * sinf(g_animTime * 2.1f);
        g_leftWristZ = 25.0f * cosf(g_animTime * 1.7f);


        g_rightShoulderX = -25.0f * sinf(g_animTime * 1.2f);
        g_rightShoulderY = 15.0f * cosf(g_animTime * 0.8f);
        g_rightShoulderZ = 20.0f + 15.0f * cosf(g_animTime * 1.0f);

        g_rightElbowY = 45.0f + 30.0f * cosf(g_animTime * 1.6f);

        g_rightWristX = -20.0f * cosf(g_animTime * 2.0f);
        g_rightWristZ = 25.0f * sinf(g_animTime * 1.5f);
    }
}

void UpdateFPS()
{
    DWORD now = GetTickCount();

    if (g_lastFpsTime == 0)
    {
        g_lastFpsTime = now;
    }

    g_frameCount++;

    DWORD elapsed = now - g_lastFpsTime;

    if (elapsed >= 1000)
    {
        g_fps = g_frameCount * 1000.0f / elapsed;

        g_frameCount = 0;
        g_lastFpsTime = now;
    }
}

void DrawAxes()
{
    glDisable(GL_LIGHTING);

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

    glEnable(GL_LIGHTING);
}

void DrawText2D(float x, float y, const std::string& text)
{
    if (g_fontBase == 0)
    {
        return;
    }

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    gluOrtho2D(0, g_width, 0, g_height);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x, y);

    glListBase(g_fontBase - 32);
    glCallLists((GLsizei)text.length(), GL_UNSIGNED_BYTE, text.c_str());

    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void DrawInfoText()
{
    std::ostringstream text;

    text << std::fixed << std::setprecision(1);

    text.str("");
    text << "Rotation X: " << g_rotateX;
    DrawText2D(15.0f, g_height - 25.0f, text.str());

    text.str("");
    text << "Rotation Y: " << g_rotateY;
    DrawText2D(15.0f, g_height - 45.0f, text.str());

    text.str("");
    text << "Rotation Z: " << g_rotateZ;
    DrawText2D(15.0f, g_height - 65.0f, text.str());

    text.str("");
    text << "Distance: " << g_distance;
    DrawText2D(15.0f, g_height - 85.0f, text.str());

    text.str("");
    text << "Mode: " << (g_wireframe ? "Wireframe" : "Solid");
    DrawText2D(15.0f, g_height - 105.0f, text.str());

    text.str("");
    text << "Auto rotate: " << (g_autoRotate ? "ON" : "OFF");
    DrawText2D(15.0f, g_height - 125.0f, text.str());

    text.str("");
    text.clear();
    text << "FPS: " << g_fps;
    DrawText2D(15.0f, g_height - 145.0f, text.str());

    DrawText2D(15.0f, 65.0f, "X/Y/Z - rotate object");
    DrawText2D(15.0f, 45.0f, "W - wireframe/solid | A - auto rotate | M - arm animation");
    DrawText2D(15.0f, 25.0f, "Mouse drag - rotate scene | Wheel - zoom | R - reset | S - screenshot");
}

bool SaveScreenshot(const std::string& fileName)
{
    int width = g_width;
    int height = g_height;

    if (width <= 0 || height <= 0)
    {
        return false;
    }

    std::vector<unsigned char> pixels(width * height * 3);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(
        0,
        0,
        width,
        height,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        pixels.data()
    );

    int rowSize = width * 3;
    int paddingSize = (4 - (rowSize % 4)) % 4;
    int rowSizeWithPadding = rowSize + paddingSize;

    int imageSize = rowSizeWithPadding * height;
    int fileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + imageSize;

    BITMAPFILEHEADER fileHeader{};
    fileHeader.bfType = 0x4D42;
    fileHeader.bfSize = fileSize;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    BITMAPINFOHEADER infoHeader{};
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = imageSize;

    std::ofstream file(fileName, std::ios::binary);

    if (!file.is_open())
    {
        return false;
    }

    file.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

    std::vector<unsigned char> padding(paddingSize, 0);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int index = (y * width + x) * 3;

            unsigned char r = pixels[index + 0];
            unsigned char g = pixels[index + 1];
            unsigned char b = pixels[index + 2];

            file.put(b);
            file.put(g);
            file.put(r);
        }

        if (paddingSize > 0)
        {
            file.write(reinterpret_cast<const char*>(padding.data()), paddingSize);
        }
    }

    file.close();
    return true;
}

void DrawObjModel(const ObjModel& model)
{
    if (!model.loaded)
    {
        return;
    }

    glBegin(GL_TRIANGLES);

    for (size_t i = 0; i < model.vertices.size(); i++)
    {
        const Vertex& v = model.vertices[i];

        glNormal3f(v.normal.x, v.normal.y, v.normal.z);
        glVertex3f(v.position.x, v.position.y, v.position.z);
    }
    glEnd();
}

void DrawRobot()
{
    
    glColor3f(0.75f, 0.78f, 0.82f);
    DrawObjModel(g_body);

   
    glPushMatrix();

    glColor3f(0.45f, 0.75f, 1.0f);

    glTranslatef(0.0f, -0.98f, 3.35f);

    glRotatef(g_leftShoulderX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_leftShoulderY, 0.0f, 1.0f, 0.0f);
    glRotatef(g_leftShoulderZ, 0.0f, 0.0f, 1.0f);

    DrawObjModel(g_leftUpperArm);

    glTranslatef(0.0f, 0.0f, -1.30f);

    glRotatef(g_leftElbowY, 0.0f, 1.0f, 0.0f);

    DrawObjModel(g_leftForearm);

    glTranslatef(0.0f, 0.0f, -1.10f);

    glRotatef(g_leftWristX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_leftWristZ, 0.0f, 0.0f, 1.0f);

    DrawObjModel(g_leftHand);

    glPopMatrix();

    
    glPushMatrix();

    glColor3f(1.0f, 0.62f, 0.35f);

    glTranslatef(0.0f, 0.98f, 3.35f);

    glRotatef(g_rightShoulderX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_rightShoulderY, 0.0f, 1.0f, 0.0f);
    glRotatef(g_rightShoulderZ, 0.0f, 0.0f, 1.0f);

    DrawObjModel(g_rightUpperArm);

    glTranslatef(0.0f, 0.0f, -1.30f);

    glRotatef(g_rightElbowY, 0.0f, 1.0f, 0.0f);

    DrawObjModel(g_rightForearm);

    glTranslatef(0.0f, 0.0f, -1.10f);

    glRotatef(g_rightWristX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_rightWristZ, 0.0f, 0.0f, 1.0f);

    DrawObjModel(g_rightHand);

    glPopMatrix();
}
