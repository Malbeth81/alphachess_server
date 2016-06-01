/*
* AlphaChessServer.cpp
*
* Copyright (C) 2007-2011 Marc-André Lamothe.
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
#include "alphachessserver.h"

static const int WM_SHELLTRAYICON = WM_APP+1;

ATOM AlphaChessServer::ClassAtom = 0;
string AlphaChessServer::ClassName = "AlphaChessServer";

string AlphaChessServer::ApplicationPath = GetApplicationPath();
string AlphaChessServer::WebRootDirectory = AlphaChessServer::ApplicationPath + "\\admin\\";

// PUBLIC FUNCTIONS ------------------------------------------------------------

AlphaChessServer* AlphaChessServer::GetInstance()
{
  static AlphaChessServer* Instance = NULL;
  if (Instance == NULL)
    Instance = new AlphaChessServer();
  return Instance;
}

AlphaChessServer::~AlphaChessServer()
{
  Stop();

  /* Destroy the window */
  if (Handle != NULL)
    DestroyWindow(Handle);
}

HWND AlphaChessServer::GetHandle()
{
  return Handle;
}

void AlphaChessServer::Initialize(HINSTANCE hInstance, LPSTR CmdLine, HWND hParent)
{
  const char* ServiceName = "alphachess";
  const char* ServiceLabel = "AlphaChess Server";

  if (IsRunningAsApplication())
  {
    bool InstallService = false;
    bool UninstallService = false;

    /* Parse parameters */
    if (CmdLine != NULL)
    {
      char* str = lowerstr(CmdLine);
      InstallService = (strpos(str, "-install") >= 0);
      UninstallService = (strpos(str, "-uninstall") >= 0);
      delete[] str;
    }

    if (InstallService)
    {
      /* Install service */
      bool Result = WinService::GetInstance()->Install(ServiceName, ServiceLabel, " executionmode=service");
      if (Result)
        MessageBox(NULL, "Service installed successfully!", "Install service", MB_OK);
      else
        MessageBox(NULL, "An error occured while installing the service!", "Install service", MB_OK);
      Stop();
      return;
    }
    else if (UninstallService)
    {
      /* Uninstall service */
      bool Result = WinService::GetInstance()->Uninstall(ServiceName);
      if (Result)
        MessageBox(NULL, "Service uninstalled successfully!", "Uninstall service", MB_OK);
      else
        MessageBox(NULL, "An error occured while uninstalling the service!", "Uninstall service", MB_OK);
      Stop();
      return;
    }

    /* Initialise application */
    if (ClassAtom == 0)
    {
      /* Register the window's class */
      WNDCLASSEX WndClass;
      WndClass.cbSize = sizeof(WNDCLASSEX);
      WndClass.lpszClassName = ClassName.c_str();
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
      ClassAtom = RegisterClassEx(&WndClass);
    }
    /* Create the window */
    if (ClassAtom != 0)
    {
      /* Create the menu */
      TrayMenu = CreatePopupMenu();
      AppendMenu(TrayMenu,IDS_TRAYMENU_ADMIN);
      AppendSeparator(TrayMenu);
      AppendMenu(TrayMenu,IDS_TRAYMENU_EXIT);

      Handle = CreateWindowEx(WS_EX_TOOLWINDOW ,ClassName.c_str(),"AlphaChessServer 4 Server",
          0,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,hParent,NULL,hInstance,this);
      if (Handle == NULL)
      {
        Stop();
        return;
      }
    }
  }
  else
  {
    /* Initialise service */
    Service = WinService::GetInstance();
    Service->AddObserver(this);
    if (!Service->Initialize())
    {
      Stop();
      return;
    }
  }
}

// PRIVATE FUNCTIONS -----------------------------------------------------------

AlphaChessServer::AlphaChessServer()
{
  /* Initialise class members */
  Handle = NULL;
  TrayIcon = NULL;
  TrayMenu = NULL;

  ChessServer = NULL;
  Service = NULL;
  WebServer = NULL;
}

string AlphaChessServer::GetJSONPlayers()
{
  string Result;
  if (ChessServer != NULL)
  {
    list<GameServerClientInfo*>* Clients = ChessServer->GetClients();
    list<GameServerClientInfo*>::iterator it;
    Result = "[";
    for (it = Clients->begin(); it != Clients->end(); it++)
    {
      if (it != Clients->begin())
        Result += ",";
      Result += "[\"";
      char* Str = inttostr((*it)->Id);
      Result += Str;
      delete[] Str;
      Result += "\",\"";
      Result += (*it)->Name;
      Result += "\",\"";
      Str = inttostr((*it)->Version);
      Result += Str;
      delete[] Str;
      Result += "\",\"";
      Str = inttostr((*it)->ConnectionTime);
      Result += Str;
      delete[] Str;
      Result += "\",\"";
      Str = inttostr((*it)->RoomId);
      Result += Str;
      delete[] Str;
      Result += "\",\"";
      Str = inttostr((*it)->Type);
      Result += Str;
      delete[] Str;
      Result += "\",\"";
      Result += ((*it)->Ready ? "1" : "0");
      Result += "\"]";
    }
    Result += "]";
    for (it = Clients->begin(); it != Clients->end(); it++)
      delete *it;
    delete Clients;
  }
  return Result;
}

string AlphaChessServer::GetJSONRooms()
{
  string Result;
  if (ChessServer != NULL)
  {
    list<GameServerRoomInfo*>* Rooms = ChessServer->GetRooms();
    list<GameServerRoomInfo*>::iterator it;
    Result = "[";
    for (it = Rooms->begin(); it != Rooms->end(); it++)
    {
      if (it != Rooms->begin())
        Result += ",";
      Result += "[\"";
      char* Str = inttostr((*it)->Id);
      Result += Str;
      delete[] Str;
      Result += "\",\"";
      Result += (*it)->Name;
      Result += "\",\"";
      if ((*it)->Private)
        Result += "Private";
      else
        Result += "Public";
      Result += "\",\"";
      if ((*it)->Started && (*it)->Paused)
        Result += "Paused";
      else if ((*it)->Started)
        Result += "Playing";
      else
        Result += "Waiting";
      Result += "\",\"";
      Str = inttostr((*it)->Players);
      Result += Str;
      delete[] Str;
      Result += "\"]";
    }
    Result += "]";
    for (it = Rooms->begin(); it != Rooms->end(); it++)
      delete *it;
    delete Rooms;
  }
  return Result;
}

HTTPResponse* __stdcall AlphaChessServer::HTTPServerProc(HTTPRequest* Request)
{
  if (Request->Method == HTTP_GET || Request->Method == HTTP_POST)
  {
    if (Request->Filename.length() == 0)
      Request->Filename = "index.html";
    string Extension = GetFileExtension(Request->Filename);
    string Filename = WebRootDirectory + Request->Filename;
    if (Extension == "gif" || Extension == "jpeg" || Extension == "jpg" || Extension == "png")
    {
      /* Prepare the response */
      HTTPImageResponse* Response = new HTTPImageResponse;
      Response->Size = sizeof(HTTPImageResponse);
      Response->Headers["Server"] = "AlphaChess 4 Server";
      if (Extension == "gif")
        Response->Headers["Content-Type"] = "image/gif";
      else if (Extension == "jpeg" || Extension == "jpg")
        Response->Headers["Content-Type"] = "image/jpeg";
      else if (Extension == "png")
        Response->Headers["Content-Type"] = "image/png";

      /* Load the file content */
      char* FileContent = getFileContent(Filename.c_str());
      if (FileContent != NULL)
      {
        Response->Content = FileContent;
        Response->ContentSize = getFileSize(Filename.c_str());
      }
      else
      {
        Response->Content = NULL;
        Response->ContentSize = 0;
        Response->Status = "404 Not Found";
      }

      /* Clean up */
      return Response;
    }
    else
    {
      /* Prepare the response */
      HTTPTextResponse* Response = new HTTPTextResponse;
      Response->Size = sizeof(HTTPTextResponse);
      Response->Headers["Server"] = "AlphaChess 4 Server";
      if (Extension == "html" || Extension == "tml")
        Response->Headers["Content-Type"] = "text/html";
      else if (Extension == "css")
        Response->Headers["Content-Type"] = "text/css";
      else if (Extension == "js")
        Response->Headers["Content-Type"] = "application/javascript";
      else if (Extension == "json")
        Response->Headers["Content-Type"] = "application/json";
      else if (Extension == "xml")
        Response->Headers["Content-Type"] = "text/xml";

      /* Load the file content */
      if (Extension == "json")
      {
        if (GetFileName(Request->Filename) == "players")
          Response->Content = GetInstance()->GetJSONPlayers();
        else if (GetFileName(Request->Filename) == "rooms")
          Response->Content = GetInstance()->GetJSONRooms();
      }
      else
      {
        char* FileContent = getFileContent(Filename.c_str());
        if (FileContent != NULL)
        {
          Response->Content = FileContent;
          delete[] FileContent;
        }
        else
          Response->Status = "404 Not Found";
      }
      return Response;
    }
  }
  else
  {
    HTTPTextResponse* Response = new HTTPTextResponse;
    Response->Size = sizeof(HTTPTextResponse);
    Response->Status = "501 Not Implemented";
    return Response;
  }
}

void AlphaChessServer::Notify(const int Event, const void* Param)
{
  switch (Event)
  {
    case RoomGameStarted:
    {
      break;
    }
    case RoomGameEnded:
    {
      GameServerRoom* Room = (GameServerRoom*)Param;

      if (Room != NULL)
      {
        SYSTEMTIME Time;
        GetSystemTime(&Time);
        char* Str = FormatDate(&Time, 0, "yyyy-MM-dd");
        string FileName = WebRootDirectory;
        FileName.append("logs\\").append(Str).append(".log");
        delete[] Str;

        /* Add to history file */
        ofstream File(FileName.c_str(), ios::app);
        if (File.is_open())
        {
          string data;
          data.append(Room->Name).append(",");
          data.append(Room->Private ? "Private" : "Public").append(",");
          data.append(Room->WhitePlayer != NULL ? Room->WhitePlayer->Name : "").append(",");
          data.append(Room->BlackPlayer != NULL ? Room->BlackPlayer->Name : "").append(",");
          Str = inttostr(Room->Observers.size());
          data.append(Str).append(",");
          delete[] Str;
          if (Room->StartTimestamp > 0)
          {
            unsigned int TickCount = GetTickCount();
            Str = inttostr(TickCount > Room->StartTimestamp ? TickCount - Room->StartTimestamp : UINT_MAX - Room->StartTimestamp + TickCount);
          }
          else
            Str = inttostr(0);
          data.append(Str);
          delete[] Str;
          File.write(data.c_str(), data.size());
          File.write("\n", 1);
          File.close();
        }
      }
      break;
    }
    case ServiceStarting:
    {
      /* Start server */
      Start();
      break;
    }
    case ServiceStarted:
    {
      break;
    }
    case ServiceStopping:
    {
      /* Stop server */
      Stop();
      break;
    }
    case ServiceStopped:
    {
      break;
    }
  }
}

void AlphaChessServer::Start()
{
  /* Start the server */
  ChessServer = new GameServer();
  ChessServer->AddObserver(this);
  WebServer = new HTTPServer(HTTPServerProc);
  WebServer->Open(2580);
}

void AlphaChessServer::Stop()
{
  /* Stop the server */
  if (ChessServer != NULL)
  {
    delete ChessServer;
    ChessServer = NULL;
  }
  if (WebServer != NULL)
  {
    delete WebServer;
    WebServer = NULL;
  }

  /* Exit application */
  PostQuitMessage(0);
}

// PRIVATE WINAPI FUNCTIONS ----------------------------------------------------

LRESULT __stdcall AlphaChessServer::WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  switch (Msg)
  {
    case WM_CLOSE:
    {
      DestroyWindow(hWnd);
      return 0;
    }
    case WM_COMMAND:
    {
      /* Process the menu and accelerator messages */
      if (lParam == 0)
      {
        AlphaChessServer* Window = (AlphaChessServer*)GetWindowLong(hWnd, GWL_USERDATA);
        if (Window != NULL)
        {
          switch (LOWORD(wParam))
          {
            case IDS_TRAYMENU_ADMIN:
            {
              ShellExecute(hWnd,"open","http://localhost:2580/index.html",NULL,NULL,SW_SHOWNORMAL);
              break;
            }
            case IDS_TRAYMENU_EXIT:
            {
              DestroyWindow(hWnd);
              break;
            }
          }
        }
      }
      return 0;
    }
    case WM_CREATE:
    {
      CREATESTRUCT* Params = (CREATESTRUCT*)lParam;
      AlphaChessServer* Window = (AlphaChessServer*)(Params->lpCreateParams);
      SetWindowLong(hWnd, GWL_USERDATA, (LPARAM)Window);

      if (Window != NULL)
      {
        Window->TrayIcon = new NOTIFYICONDATA;
        Window->TrayIcon->cbSize = sizeof(NOTIFYICONDATA);
        Window->TrayIcon->hWnd = hWnd;
        Window->TrayIcon->uID = 0;
        Window->TrayIcon->uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        Window->TrayIcon->uCallbackMessage = WM_SHELLTRAYICON;
        Window->TrayIcon->hIcon = (HICON)LoadImage((HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE), "ID_TRAYICON", IMAGE_ICON, 16, 16, LR_CREATEDIBSECTION);
        strcpy(Window->TrayIcon->szTip, "AlphaChess 4 Server");
        Shell_NotifyIcon(NIM_ADD, Window->TrayIcon);

        /* Start server */
        Window->Start();
      }
      return 0;
    }
    case WM_DESTROY:
    {
      AlphaChessServer* Window = (AlphaChessServer*)GetWindowLong(hWnd, GWL_USERDATA);
      if (Window != NULL)
      {
        Shell_NotifyIcon(NIM_DELETE, Window->TrayIcon);
        delete Window->TrayIcon;
        Window->Stop();
      }
      else
        PostQuitMessage(0);
      return 0;
    }
    case WM_SHELLTRAYICON:
    {
      switch (LOWORD(lParam))
      {
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        {
          AlphaChessServer* Window = (AlphaChessServer*)GetWindowLong(hWnd, GWL_USERDATA);
          if (Window != NULL)
          {
            POINT P;
            if (GetCursorPos(&P) != 0)
              TrackPopupMenu(Window->TrayMenu, TPM_LEFTALIGN, P.x, P.y, 0, hWnd, NULL);
          }
          break;
        }
      }
      return 0;
    }
  }
  return DefWindowProc(hWnd,Msg,wParam,lParam);
}
