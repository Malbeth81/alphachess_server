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

#include <tcpclientsocket.h>
#include <tcpserversocket.h>
#include <linkedlist.h>
#include "gamedata.h"

struct RoomInfo;
struct PlayerInfo
{
  unsigned int PlayerId;
  char Name[60];
  bool Ready;
  RoomInfo* Room;
  TCPClientSocket* Socket;
  bool Synched;
};

struct RoomInfo
{
  unsigned int RoomId;
  bool Locked;
  bool Paused;
  char Name[60];
  PlayerInfo* Owner;
  PlayerInfo* WhitePlayer;
  PlayerInfo* BlackPlayer;
  LinkedList<PlayerInfo> Observers;
};

class GameServer
{
public:
  static const int Port;
  static const char* Id;
  static const int SupportedVersion;
  static const int Version;

  GameServer();
  ~GameServer();

  bool Open();
  bool Close();
private:
  TCPServerSocket Socket;
  HANDLE hServerThread;

  LinkedList<PlayerInfo> Players;
  unsigned int PlayerIdCounter;

  LinkedList<RoomInfo> Rooms;
  unsigned int RoomIdCounter;

  void CloseConnection(PlayerInfo* Player);
  void ChangePlayerType(PlayerInfo* Player, PlayerType Type);
  void JoinRoom(RoomInfo* Room, PlayerInfo* Player);
  void LeaveRoom(PlayerInfo* Player);
  PlayerInfo* NewPlayer();
  RoomInfo* NewRoom();
  PlayerInfo* OpenConnection(TCPClientSocket* Socket);
  void ReceiveData(PlayerInfo* Player);
  bool SendGameData(TCPClientSocket* Socket, const void* Data, const unsigned long DataSize);
  bool SendHostChanged(TCPClientSocket* Socket, const unsigned int Id);
  bool SendMessage(TCPClientSocket* Socket, const unsigned int PlayerId, const char* AMessage);
  bool SendMove(TCPClientSocket* Socket, const unsigned long Data);
  bool SendName(TCPClientSocket* Socket, const unsigned int PlayerId, const char* PlayerName);
  bool SendNetworkRequest(TCPClientSocket* Socket, const NetworkRequestType Request);
  bool SendNotification(TCPClientSocket* Socket, const NotificationType Notification);
  bool SendPlayerId(TCPClientSocket* Socket, const unsigned int Id);
  bool SendPlayerType(TCPClientSocket* Socket, const unsigned int PlayerId, const PlayerType Type);
  bool SendPlayerJoined(TCPClientSocket* Socket, const unsigned int PlayerId, const char* PlayerName);
  bool SendPlayerLeft(TCPClientSocket* Socket, const unsigned int PlayerId);
  bool SendPlayerReady(TCPClientSocket* Socket, const unsigned int PlayerId);
  bool SendPlayerRequest(TCPClientSocket* Socket, const PlayerRequestType Request);
  bool SendPromoteTo(TCPClientSocket* Socket, const int Type);
  bool SendRoomInfo(TCPClientSocket* Socket, const unsigned int RoomId, const char* RoomName, const bool RoomLocked, const int PlayerCount);
  bool SendTime(TCPClientSocket* Socket, const unsigned int PlayerId, const unsigned long Time);
  static unsigned long __stdcall ConnectionThread(void* arg);
  static unsigned long __stdcall ServerThread(void* arg);
};

#endif
