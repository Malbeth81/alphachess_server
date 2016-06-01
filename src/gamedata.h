/*
* GameData.h - Definition of the data exchanged between the server and the clients.
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
#ifndef GAMEDATA_H_
#define GAMEDATA_H_

/* Type of data transmitted */
enum NetworkData
{
  ND_Unknown,
  /* Data sent by the client only */
  ND_CreateRoom, ND_JoinRoom, ND_LeaveRoom, ND_ChangeType, ND_RemovePlayer,
  /* Data sent by both the client & the server */
  ND_Disconnection, ND_GameData, ND_Message, ND_Move, ND_Name, ND_NetworkRequest, ND_Notification, ND_PlayerRequest, ND_PlayerTime, ND_PromoteTo,
  /* Data sent by the server only */
  ND_GameDataUpdate, ND_HostChanged, ND_PlayerId, ND_PlayerType, ND_PlayerJoined, ND_PlayerLeft, ND_PlayerReady, ND_RoomInfo
};

/* Type of notification */
enum NotificationType
{
  /* Notification sent by the client only */
  IAmReady, IResign, GameEnded,
  /* Notification sent by the client and the server */
  GamePaused, GameResumed, DrawRequestAccepted, DrawRequestRejected, TakebackRequestAccepted, TakebackRequestRejected,
  /* Notification sent by the server only */
  GameStarted, GameDrawed, JoinedRoom, LeftRoom, Resigned, TookbackMove
};

/* Type of network request */
enum NetworkRequestType {GameData, RoomList};

/* Type of player request */
enum PlayerRequestType {DrawRequest, TakebackRequest};

/* Type of player */
enum PlayerType {ObserverType, BlackPlayerType, WhitePlayerType};

#endif
