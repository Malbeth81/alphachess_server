/*
* GameServerClient.h - Game server class that handles all the players connections.
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
#ifndef GAMESERVERCLIENT_H_
#define GAMESERVERCLIENT_H_

#include "gamedata.h"
#include "gameserver.h"
#include "system.h"
#include <limits.h>
#include <linkedlist.h>
#include <string>
#include <tcpclientsocket.h>
#include <tcpserversocket.h>
#include <thread.h>

using namespace std;

struct GameServerRoom;

class GameServer;

class GameServerClient : public Thread
{
public:
  unsigned int Id;
  string Name;
  bool Ready;
  GameServerRoom* Room;
  bool Synchronised;
  int Version;

  GameServerClient(GameServer* Parent, SOCKET Socket, unsigned int ClientId);

  long ConnectionTime();
  bool SendGameData(const void* Data, const unsigned long DataSize);
  bool SendHostChanged(const unsigned int Id);
  bool SendMessage(const unsigned int PlayerId, const string Message);
  bool SendMove(const unsigned long Data);
  bool SendName(const unsigned int PlayerId, const string PlayerName);
  bool SendNetworkRequest(const NetworkRequestType Request);
  bool SendNotification(const NotificationType Notification);
  bool SendPlayerId(const unsigned int Id);
  bool SendPlayerType(const unsigned int PlayerId, const PlayerType Type);
  bool SendPlayerJoined(const unsigned int PlayerId, const string PlayerName);
  bool SendPlayerLeft(const unsigned int PlayerId);
  bool SendPlayerReady(const unsigned int PlayerId);
  bool SendPlayerRequest(const PlayerRequestType Request);
  bool SendPromoteTo(const int Type);
  bool SendRoomInfo(const unsigned int RoomId, const string RoomName, const bool RoomLocked, const int PlayerCount);
  bool SendTime(const unsigned int PlayerId, const unsigned long Time);

private:
  GameServer* Server;
  TCPClientSocket* Socket;

  static int ReceiveData(GameServerClient* Player);
  unsigned int Run();
};

#endif
