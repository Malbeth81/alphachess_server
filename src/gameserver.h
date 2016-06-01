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

#include "gameserverdata.h"
#include "gameserverclient.h"
#include "system.h"
#include <limits.h>
#include <list>
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
  list<GameServerClient*> Observers;
};

/* Web interface only */
struct GameServerClientInfo
{
  unsigned int Id;
  string Name;
  bool Ready;
  unsigned int RoomId;
  PlayerType Type;
  bool Synchronised;
  int Version;
  long ConnectionTime;
};

/* Web interface only */
struct GameServerRoomInfo
{
  unsigned int Id;
  bool Locked;
  bool Paused;
  bool Started;
  string Name;
  unsigned int Players;
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

  void ChangeSeat(GameServerClient* Client, PlayerType Type);
  GameServerRoom* CreateRoom(GameServerClient* Client, string Name);
  GameServerClient* FindPlayer(unsigned int Id);
  GameServerRoom* FindRoom(unsigned int Id);
  list<GameServerClientInfo*>* GetClients();
  list<GameServerRoomInfo*>* GetRooms();
  void JoinRoom(GameServerClient* Client, GameServerRoom* Room);
  void LeaveRoom(GameServerClient* Client);
  void RemoveClient(GameServerClient* Client);
  void SendGameData(GameServerClient* Client, unsigned char* Data, unsigned long DataSize);
  void SendMessage(GameServerClient* Client, char* Message);
  void SendMove(GameServerRoom* Room, unsigned long Data);
  void SendNotification(GameServerRoom* Room, NotificationType Notification);
  void SendPromotion(GameServerRoom* Room, int Type);
  void SendRequest(GameServerClient* Client, PlayerRequestType Request);
  void SendRoomList(GameServerClient* Client);
  void SendTime(GameServerRoom* Room, unsigned int Id, unsigned long Time);
  void SetName(GameServerClient* Client, char* PlayerName);
  void SetReady(GameServerClient* Client);

private:
  list<GameServerClient*> Clients;
  unsigned int ClientIdCounter;
  list<GameServerRoom*> Rooms;
  unsigned int RoomIdCounter;

  HANDLE Mutex;

  unsigned int Run();
};

#endif
