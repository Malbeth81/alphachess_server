/*
* AlphaChessServer.h
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
#ifndef ALPHACHESSSERVER_H_
#define ALPHACHESSSERVER_H_

#include "gameserver.h"
#include "resource.h"
#include "system.h"
#include <cstrutils.h>
#include <string>
#include <httpserver.h>
#include <winutils.h>

using namespace std;

class AlphaChessServer
{
public:
  static AlphaChessServer* GetInstance(HINSTANCE hInstance, HWND hParent);

  ~AlphaChessServer();

  HWND GetHandle();
private:
  static ATOM ClassAtom;
  static string ClassName;

  static string ApplicationPath;
  static string WebRootDirectory;

  static AlphaChessServer* Instance;

  HWND Handle;
  NOTIFYICONDATA* TrayIcon;
  HMENU TrayMenu;

  GameServer* ChessServer;
  HTTPServer* WebServer;

  AlphaChessServer(HINSTANCE hInstance, HWND hParent);

  string GetJSONPlayers();
  string GetJSONRooms();
  static HTTPResponse* __stdcall HTTPServerProc(HTTPRequest* Request);
  static LRESULT __stdcall WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif
