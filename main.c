#include <windows.h>
#include <gl/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "SOIL.h"

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void EnableOpenGL(HWND hwnd, HDC*, HGLRC*);
void DisableOpenGL(HWND, HDC, HGLRC);

//funcoes
struct DadosCenario *cen;
struct Jogador *grade;
time_t tempo;

void desenhaJogo();
void iniciaJogo();
void terminaJogo();

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    WNDCLASSEX wcex;
    HWND hwnd;
    HDC hDC;
    HGLRC hRC;
    MSG msg;
    BOOL bQuit = FALSE;

    /* register window class */
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_OWNDC;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "GLSample";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);;


    if (!RegisterClassEx(&wcex))
        return 0;

    /* create main window */
    hwnd = CreateWindowEx(0,
                          "GLSample",
                          "Tetris Attack",
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          700, // tamanho janela
                          700,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    ShowWindow(hwnd, nCmdShow);

    /* enable OpenGL for the window */
    EnableOpenGL(hwnd, &hDC, &hRC);

    iniciaJogo();

    /* program main loop */
    while (!bQuit)
    {
        /* check for messages */
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            /* handle or dispatch messages */
            if (msg.message == WM_QUIT)
            {
                bQuit = TRUE;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            /* OpenGL animation code goes here */

            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glPushMatrix();

            // desenha o jogo aqui
            desenhaJogo();

            glPopMatrix();

            SwapBuffers(hDC);

            Sleep (10);
        }
    }

    // termina o jogo aqui
    terminaJogo();

    /* shutdown OpenGL */
    DisableOpenGL(hwnd, hDC, hRC);

    /* destroy the window explicitly */
    DestroyWindow(hwnd);

    return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE:
            PostQuitMessage(0);
        break;

        case WM_DESTROY:
            return 0;

        case WM_KEYDOWN:
        {
            switch (wParam)         //coloca os cases do input aqui
            {
                case VK_ESCAPE:
                    PostQuitMessage(0);
                    break;
                case VK_RIGHT:  //seta direita
                    gradeMovimenta(grade,0,cen);
                    break;
                case VK_LEFT:   //seta esquerda
                    gradeMovimenta(grade,1,cen);
                    break;
                case VK_UP:     //seta p cima
                    gradeMovimenta(grade,2,cen);
                    break;
                case VK_DOWN:   //seta p baixo
                    gradeMovimenta(grade,3,cen);
                    break;
                case VK_SPACE:  //barra de espaco (troca a peca de lugar)
                    gradeTrocaPeca(grade,cen);
                    break;
            }
        }
        break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

void EnableOpenGL(HWND hwnd, HDC* hDC, HGLRC* hRC)
{
    PIXELFORMATDESCRIPTOR pfd;

    int iFormat;

    /* get the device context (DC) */
    *hDC = GetDC(hwnd);

    /* set the pixel format for the DC */
    ZeroMemory(&pfd, sizeof(pfd));

    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW |
                  PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    iFormat = ChoosePixelFormat(*hDC, &pfd);

    SetPixelFormat(*hDC, iFormat, &pfd);

    /* create and enable the render context (RC) */
    *hRC = wglCreateContext(*hDC);

    wglMakeCurrent(*hDC, *hRC);

    // carregar texturas aqui
    carregaTexturas();
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
}

void DisableOpenGL (HWND hwnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hwnd, hDC);
}

// --------------------
// funcoes do jogo

void iniciaJogo(){
    srand(time(NULL));
    tempo = time(NULL);
    cen = cenarioInicia();
    pecaRandomiza(cen);
    grade = gradeJogador(7,3); //posicao em q a grade inicia
}

void desenhaJogo(){
    pecaDesenha(cen);
    cenarioDesenha(cen);
    gradeDesenha(grade);
    pontuacaoDesenha(cen,grade);
    verificaPeca(grade,cen);
    eliminaPeca(cen,grade); //eliminaPeca() --> destroiPeca()
    gravidadePeca(cen,grade);
    cenarioMovimentaA(cen,grade,&tempo);
}

void terminaJogo(){
    destroiCenario(cen);
    destroiGrade(grade);
}
