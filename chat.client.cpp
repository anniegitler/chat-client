#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <cstring>
#include <thread>
#include <atomic>
#include <string>
#include <richedit.h>
#include <commdlg.h>
#include "filetransferclass.h"

#pragma comment(lib, "Comdlg32.lib")

#pragma comment(lib, "ws2_32.lib")

SOCKET clientsocket = INVALID_SOCKET;

NetworkTransfer transfer;

std::thread receiverThread;
std::atomic<bool> running = true;


HWND chatbox;
HWND messagebox;
HWND sendbutton;
HWND usersbox;
HWND filebutton;
HWND usernamebox;
HWND usernamebutton;


#define ID_USERS    1000
#define ID_CHAT     1001
#define ID_MESSAGE  1002
#define ID_SEND     1003
#define ID_FILE     1004
#define ID_USERNAME       1005
#define ID_SET_USERNAME   1006
#define CHAT_PORT   17119

#define WM_NEW_CHAT (WM_APP + 1)

void addchattext(HWND box, const char* text, bool right)
{

    int length = GetWindowTextLengthA(box);

    SendMessageA(box, EM_SETSEL, length, length);

    PARAFORMAT2 format{};

    format.cbSize = sizeof(PARAFORMAT2);

    format.dwMask = PFM_ALIGNMENT;


    if (right)
    {
        format.wAlignment = PFA_RIGHT;
    }
    else
    {
        format.wAlignment = PFA_LEFT;
    }

    SendMessageA(box, EM_SETPARAFORMAT, 0, (LPARAM)&format);

    SendMessageA(box, EM_REPLACESEL, FALSE, (LPARAM)text);
}

void ReceiveMessages(HWND hwnd)
{
    while (running)
    {
        std::string received;

        char type = transfer.receive(clientsocket, received);

        if (type == 0)
        {
            break;
        }


        if (type == 'M')
        {

            std::string* text = new std::string("Server: " + received + "\r\n");

            PostMessageA(hwnd, WM_NEW_CHAT, (WPARAM)text, 0);
        }


        if (type == 'F')
        {
            std::string* text = new std::string("Server sent a file: " + received + "\r\n");

            PostMessageA(hwnd, WM_NEW_CHAT, (WPARAM)text, 0);
        }
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {

        usersbox       = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE | WS_BORDER, 20, 80, 540, 55, hwnd, (HMENU)ID_USERS, NULL, NULL);

        chatbox        = CreateWindowA("RICHEDIT20A", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_READONLY | WS_VSCROLL, 20, 200, 540, 280, hwnd, (HMENU)ID_CHAT, NULL, NULL);

        messagebox     = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 100, 500, 350, 32, hwnd, (HMENU)ID_MESSAGE, NULL, NULL);

        usernamebox    = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 20, 150, 200, 30, hwnd, (HMENU)ID_USERNAME, NULL, NULL);

        sendbutton     = CreateWindowA("BUTTON", "Send", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 460, 500, 100, 32, hwnd, (HMENU)ID_SEND, NULL, NULL);

        filebutton     = CreateWindowA("BUTTON", "file", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 500, 70, 32, hwnd, (HMENU)ID_FILE, NULL, NULL);
        
        usernamebutton = CreateWindowA("BUTTON", "set username", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 230, 150, 120, 30, hwnd, (HMENU)ID_SET_USERNAME, NULL, NULL);


        return 0;
    }


    case WM_COMMAND:
    {
        if (LOWORD(wParam) == ID_SET_USERNAME)
        {

            char username[100];

            GetWindowTextA(usernamebox, username, sizeof(username));

            if (strlen(username) == 0)
            {
                MessageBoxA(hwnd, "Enter Username", "username", MB_OK);

                return 0;
            }

            std::string message = "USERNAME:";

            message += username;

            transfer.sendMessage(clientsocket, message.c_str());

            SetWindowTextA(usernamebox, "");

            MessageBoxA(hwnd, "Username Set, YEEY!", "username", MB_OK);

            return 0;
        }


        if (LOWORD(wParam) == ID_SEND)
        {
            char message[4096];

            GetWindowTextA(messagebox, message, sizeof(message));


            if (strlen(message) == 0)
            {
                return 0;
            }




            if (transfer.sendMessage(clientsocket, message))
            {
                std::string text = "";

                text += message;

                text += "\r\n";


                addchattext(chatbox, text.c_str(), true);
            }


            SetWindowTextA(messagebox, "");


            SetFocus(messagebox);

            return 0;
        }

        if (LOWORD(wParam) == ID_FILE)
        {
            char filename[MAX_PATH] = "";

            OPENFILENAMEA file{};

            file.lStructSize = sizeof(OPENFILENAMEA);

            file.hwndOwner = hwnd;

            file.lpstrFile = filename;

            file.nMaxFile = MAX_PATH;

            file.lpstrFilter = "All Files\0*.*\0\0";

            file.Flags = OFN_FILEMUSTEXIST;

            if (GetOpenFileNameA(&file))
            {
                if (transfer.sendFile(clientsocket, filename))
                {

                    std::string text = "You sent: ";

                    text += filename;
                    text += "\r\n";

                    addchattext(chatbox, text.c_str(), true);

                }



            }

            return 0;

        }

        return 0;

    }





    case WM_NEW_CHAT:
    {
        std::string* text = (std::string*)wParam;


        addchattext(chatbox, text->c_str(), false);


        delete text;


        return 0;
    }


    case WM_DESTROY:
    {
        running = false;


        if (clientsocket != INVALID_SOCKET)
        {
            shutdown(clientsocket, SD_BOTH);
        }


        if (receiverThread.joinable())
        {
            receiverThread.join();
        }


        if (clientsocket != INVALID_SOCKET)
        {
            closesocket(clientsocket);
        }


        WSACleanup();

        PostQuitMessage(0);

        return 0;
    }


    }


    return DefWindowProcA(hwnd, msg, wParam, lParam);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{


    WSADATA wsadata;

    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
    {
        MessageBoxA(NULL, "WSAStartup failed", "Error", MB_OK);

        return 1;
    }




    clientsocket = socket(AF_INET, SOCK_STREAM, 0);


    if (clientsocket == INVALID_SOCKET)
    {
        MessageBoxA(NULL, "Could not create socket", "Error", MB_OK);

        WSACleanup();

        return 1;
    }




    sockaddr_in serveraddress{};

    serveraddress.sin_family = AF_INET;

    serveraddress.sin_port = htons(CHAT_PORT);



    inet_pton(AF_INET, "17.117.1.10", &serveraddress.sin_addr);




    if (connect(clientsocket, (sockaddr*)&serveraddress, sizeof(serveraddress)) == SOCKET_ERROR)
    {
        MessageBoxA(NULL, "Could not connect to server", "Connection error", MB_OK);

        closesocket(clientsocket);

        WSACleanup();

        return 1;
    }



    WNDCLASSA wc{};

    wc.lpfnWndProc = WndProc;

    wc.hInstance = hInstance;

    wc.lpszClassName = "ClientWindowClass";

    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);


    if (!RegisterClassA(&wc))
    {
        MessageBoxA(NULL, "RegisterClass failed", "Error", MB_OK);

        return 1;
    }


    LoadLibraryA("Riched20.dll");

    HWND hwnd = CreateWindowA("ClientWindowClass", "Client Chat", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 600, 600, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL)
    {
        MessageBoxA(NULL, "CreateWindow failed", "Error", MB_OK);

        return 1;
    }



    ShowWindow(hwnd, nCmdShow);

    UpdateWindow(hwnd);



    receiverThread = std::thread(ReceiveMessages, hwnd);




    MSG msg{};

    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);

        DispatchMessageA(&msg);
    }


    return 0;
}