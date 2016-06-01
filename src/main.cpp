/*
* Main.cpp - The program's entry point, WinMain.
*
* Copyright (C) 2007-2010 Marc-André Lamothe.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Library General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#include <windows.h>
#include "gameserver.h"

#define ClassName "AlphaServer"
#define WindowTitle "AlphaServer"

HINSTANCE hInstance;
HWND hWindow;

GameServer* Server;

LRESULT __stdcall WindowProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

// WINAPI FUNCTIONS ------------------------------------------------------------

void CreateWindowApp()
{
  /* Create the main window */
  hWindow = CreateWindowEx(0,ClassName,WindowTitle,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_CLIPCHILDREN,
    CW_USEDEFAULT,CW_USEDEFAULT,400,300,HWND_DESKTOP,NULL,hInstance,NULL);
}

int __stdcall WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdLine, int Show)
{
  MSG Messages;
  WNDCLASSEX WndClass;
  hInstance = hInst;
  /* Specify the window class information */
  WndClass.cbSize = sizeof(WNDCLASSEX);
  WndClass.lpszClassName = ClassName;
  WndClass.hInstance = hInstance;
  WndClass.lpfnWndProc = WindowProc;
  WndClass.style = 0;
  WndClass.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
  WndClass.hIcon = NULL;
  WndClass.hIconSm = NULL;
  WndClass.hCursor = LoadCursor(NULL,IDC_ARROW);
  WndClass.lpszMenuName = NULL;
  WndClass.cbClsExtra = 0;
  WndClass.cbWndExtra = 0;
  /* Register the new window class */
  if (RegisterClassEx(&WndClass) == 0)
    return 0;
  /* Create and show the window */
  CreateWindowApp();
  ShowWindow(hWindow,SW_SHOW);
  /* Message loop */
  while (GetMessage(&Messages,NULL,0,0) != 0)
  {
    if (IsDialogMessage(hWindow,&Messages) == 0)
    {
      TranslateMessage(&Messages);
      DispatchMessage(&Messages);
    }
  }
  return 0;
}

LRESULT __stdcall WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  switch (Msg)
  {
    case WM_CLOSE:
    {
      DestroyWindow(hWnd);
      return 0;
    }
    case WM_CREATE:
    {
      Server = new GameServer();
      Server->Open();
      return 0;
    }
    case WM_DESTROY:
    {
      Server->Close();
      PostQuitMessage(0);
      return 0;
    }
  }
  return DefWindowProc(hWnd,Msg,wParam,lParam);
}
