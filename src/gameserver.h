/*
* GameServer.h - Game server class that handles all the players connections.
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
#ifndef GAMESERVER_H_
#define GAMESERVER_H_

#include "gamedata.h"
#include "gameserverclient.h"
#include "system.h"
#include <limits.h>
#include <linkedlist.h>
#include <string>
#include <tcpserversocket.h>
#include <thread.h>

using namespace std;

class GameServerClient;

struct GameServerRoom
{
  unsigned int Id;
  bool Locked;
  bool Paused;
  string Name;
  GameServerClient* Owner;
  GameServerClient* WhitePlayer;
  GameServerClient* BlackPlayer;
  LinkedList<GameServerClient> Observers;
};

class GameServer : public Thread
{
public:
  static const int Port;
  static const char* Id;
  static const int SupportedVersion;
  static const int Version;

  GameServer();
  ~GameServer();

  GameServerRoom* CreateRoom(GameServerClient* Client, string Name);
  GameServerRoom* FindRoom(unsigned int Id);
  const LinkedList<GameServerClient> GetClients();
  const LinkedList<GameServerRoom> GetRooms();
  void JoinRoom(GameServerClient* Client, GameServerRoom* Room);
  void LeaveRoom(GameServerClient* Client);
  void RemoveClient(GameServerClient* Client);
  void SendRoomList(GameServerClient* Client);

private:
  LinkedList<GameServerClient> Clients;
  unsigned int ClientIdCounter;
  LinkedList<GameServerRoom> Rooms;
  unsigned int RoomIdCounter;

  unsigned int Run();
};

#endif
