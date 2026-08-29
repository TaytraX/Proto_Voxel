#define VMA_IMPLEMENTATION
#include "texture.h"
#include "networking.h"
#include <optional>
#include <iostream>
#include "framework.h"
#include "client.h"
#include "shared_context.h"
#include "state.h"
#include "scene.h"
#include "block.h"
#include <functional>

#define MAX_LOADSTRING 100
bool keys[256] = {};
bool mouseAdd = false;
bool mouseRemove = false;

// Variables globales :
HINSTANCE hInst;                                // instance actuelle
WCHAR szTitle[MAX_LOADSTRING];                  // Texte de la barre de titre
WCHAR szWindowClass[MAX_LOADSTRING];            // nom de la classe de fenêtre principale

// Déclarations anticipées des fonctions incluses dans ce module de code :
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
RAWMOUSE* mouse;
RAWKEYBOARD* keyboard;

Context context;

bool isRunning = true;

RAWINPUT* raw;
RAWINPUTDEVICE rid[2];
POINT lastMousePos;
POINT currentMousePos;
glm::vec3 cameraForward;
glm::vec3 cameraRight;
float cameraSpeed = 0.04f; // Vitesse de déplacement de la caméra
float cameraSensibility = 0.01f; // Vitesse de déplacement de la caméra

glm::vec2 mouseDelta;
glm::vec3 cameraWorldPos(0.0f, 1.0f, 0.0f);

POINT center;

void processInput(std::function<void(glm::ivec3)> updateChunkCallback) {
    camera.yaw += mouseDelta.x;
    camera.pitch -= mouseDelta.y;

    if (keys['W']) {
        camera.position += cameraForward * cameraSpeed;
        cameraWorldPos += cameraForward * cameraSpeed;
    }
    if (keys['S']) {
        camera.position -= cameraForward * cameraSpeed;
        cameraWorldPos -= cameraForward * cameraSpeed;
    }
    if (keys['A']) {
        camera.position -= cameraRight * cameraSpeed;
        cameraWorldPos -= cameraRight * cameraSpeed;
    }
    if (keys['D']) {
        camera.position += cameraRight * cameraSpeed;
        cameraWorldPos += cameraRight * cameraSpeed;
    }
    if (keys[VK_SPACE]) {
        camera.position.y += cameraSpeed;
        cameraWorldPos.y += cameraSpeed;
    }
    if (keys[VK_SHIFT]) {
        camera.position.y -= cameraSpeed;
        cameraWorldPos.y -= cameraSpeed;
    }
	if (keys[VK_TAB]) cameraSpeed += 0.01f;
	else cameraSpeed = 0.04f;
    if (keys[VK_ESCAPE]) isRunning = false;

    mouseDelta.x = 0.0;
    mouseDelta.y = 0.0;
    if (mouseAdd) {
        auto chunkPos = addBlock(cameraWorldPos, cameraForward, 1);
		mesh(scene::chunkMap[chunkPos], scene::chunkMeshMap[chunkPos].first);
        updateChunkCallback(chunkPos);
		mouseAdd = false;
    }
    if (mouseRemove) {
        auto chunkPos = removeBlock(cameraWorldPos, cameraForward);
        auto chunkIndex = glm::ivec2(chunkPos.x + RENDER_DISTANCE, chunkPos.z + RENDER_DISTANCE);
        mesh(scene::chunkMap[chunkPos], scene::chunkMeshMap[chunkPos].first);
        updateChunkCallback(chunkPos);
        mouseRemove = false;
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	AllocConsole();
	FILE* pCout;
    freopen_s(&pCout, "CONOUT$", "w", stdout);
    freopen_s(&pCout, "CONOUT$", "w", stderr);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Placez le code ici.

    // Initialise les chaînes globales
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CLIENT, szWindowClass, MAX_LOADSTRING);
    
    MyRegisterClass(hInstance);

    // Effectue l'initialisation de l'application :
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    for (int X = -RENDER_DISTANCE; X <= RENDER_DISTANCE; X++)
        for (int Z = -RENDER_DISTANCE; Z <= RENDER_DISTANCE; Z++) {
            if (floor(std::sqrt(X * X + Z * Z)) > RENDER_DISTANCE) continue;
            for (int x = 0; x < CHUNK_AXIS1_SIZE; ++x) {
                for (int y = 1; y < 2; ++y) {
                    for (int z = 0; z < CHUNK_AXIS1_SIZE; ++z) {
                        scene::chunkMap[{X, 0, Z}][voxelIndex(x, y, z)] = 2;
                        scene::chunkMap[{X, 0, Z}][voxelIndex(1, 2, 1)] = 1;
                    }
                }
            }

        }
    scene::genScene();
    scene::startWorker();

    RenderState renderState;
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENT));

    MSG msg;

    // Boucle de messages principale :
    while (isRunning)
    {
        SetCursorPos(center.x, center.y);
        // Windows
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            DispatchMessage(&msg);
        }

        processInput([&](glm::ivec3 chunkPos) {
            renderState.updateChunk(chunkPos);
        });
        renderState.update();
        renderState.drawFrame();
    }
    scene::stopWorker();
    return (int) msg.wParam;
}



//
//  FONCTION : MyRegisterClass()
//
//  OBJECTIF : Inscrit la classe de fenêtre.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CLIENT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FONCTION : InitInstance(HINSTANCE, int)
//
//   OBJECTIF : enregistre le handle d'instance et crée une fenêtre principale
//
//   COMMENTAIRES :
//
//        Dans cette fonction, nous enregistrons le handle de l'instance dans une variable globale, puis
//        nous créons et affichons la fenêtre principale du programme.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Stocke le handle d'instance dans la variable globale

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   RECT rect;

   rid[0].usUsagePage = 0x01; // Generic desktop controls
   rid[0].usUsage = 0x02;     // Mouse
   rid[0].dwFlags = RIDEV_NOLEGACY;
   rid[0].hwndTarget = hWnd;

   rid[1].usUsagePage = 0x01; // Generic desktop controls
   rid[1].usUsage = 0x06;     // Keyboard
   rid[1].dwFlags = RIDEV_NOLEGACY;
   rid[1].hwndTarget = hWnd;

   RegisterRawInputDevices(rid, 2, sizeof(rid[0]));
   //connectToServer("127.0.0.1", 82807);
   context.init(hWnd, hInstance);

   if (!hWnd)
   {
      return FALSE;
   }

   GetClientRect(hWnd, &rect);
   center = { rect.right / 2, rect.bottom / 2 };
   ShowCursor(FALSE);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FONCTION : WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  OBJECTIF : Traite les messages pour la fenêtre principale.
//
//  WM_COMMAND  - traite le menu de l'application
//  WM_PAINT    - Dessine la fenêtre principale
//  WM_DESTROY  - génère un message d'arrêt et retourne
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    cameraForward.x = cos(camera.pitch) * cos(camera.yaw);
    cameraForward.y = sin(camera.pitch);
    cameraForward.z = sin(camera.yaw) * cos(camera.pitch);

	cameraRight = glm::normalize(glm::cross(cameraForward, glm::vec3(0.0f, 1.0f, 0.0f)));

    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Analyse les sélections de menu :
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
    case WM_INPUT:
    {
        UINT size;
        GetRawInputData(
            (HRAWINPUT)lParam,
            RID_INPUT,
            nullptr,
            &size,
            sizeof(RAWINPUTHEADER)
        );

        raw = (RAWINPUT*)malloc(size);

        GetRawInputData(
            (HRAWINPUT)lParam,
            RID_INPUT,
            raw,
            &size,
            sizeof(RAWINPUTHEADER)
        );

		mouse = &raw->data.mouse;
		keyboard = &raw->data.keyboard;

        if (raw->header.dwType == RIM_TYPEMOUSE)
        {
            mouseDelta.x = mouse->lLastX * cameraSensibility;
            mouseDelta.y = mouse->lLastY * cameraSensibility;

            mouseRemove = (mouse->usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN);
            mouseAdd = (mouse->usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN);
        }
        else if (raw->header.dwType == RIM_TYPEKEYBOARD)
        {
            //std::cout << "camera position is (" << camera.position.x << ", " << camera.position.y << ", " << camera.position.z << ')' << std::endl;
            if ((keyboard->Flags & RI_KEY_BREAK) == 0)
            {
                keys[keyboard->VKey] = true;
            }
            else
            {
                keys[keyboard->VKey] = false;
            }
        }

        free(raw);
    }
    break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            // TODO: Ajoutez n’importe quel code de dessin ici...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Gestionnaire de messages pour la boîte de dialogue À propos de.
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
