/*
* GameServerClient.cpp - Game server class that handles all the players connections.
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
#include "gameserverclient.h"
#ifdef DEBUG
#include <iostream>
#endif

// Public functions ------------------------------------------------------------

GameServerClient::GameServerClient(GameServer* Parent, SOCKET SocketId, unsigned int ClientId)
{
  Id = ClientId;
  Name = "";
  Ready = false;
  Room = NULL;
  Server = Parent;
  Socket = new TCPClientSocket(SocketId);
  Synchronised = false;
  Version = 0;
  Resume();
}

GameServerClient::~GameServerClient()
{
  Socket->Close();
}

long GameServerClient::ConnectionTime()
{
  return Socket->GetConnectionTime();
}

bool GameServerClient::SendGameData(const void* Data, const unsigned long DataSize)
{
  if (!Socket->SendInteger(ND_GameData))
    return false;
  if (!Socket->SendInteger(DataSize))
    return false;
  if (!Socket->SendBytes(Data, DataSize))
    return false;
  return true;
}

bool GameServerClient::SendHostChanged(const unsigned int Id)
{
  if (!Socket->SendInteger(ND_HostChanged))
    return false;
  if (!Socket->SendInteger(Id))
    return false;
  return true;
}

bool GameServerClient::SendMessage(const unsigned int PlayerId, const string Message)
{
  if (!Socket->SendInteger(ND_Message))
    return false;
  if (!Socket->SendInteger(PlayerId))
    return false;
  if (!Socket->SendString(Message.c_str()))
    return false;
  return true;
}

bool GameServerClient::SendMove(const unsigned long Data)
{
  if (!Socket->SendInteger(ND_Move))
    return false;
  if (!Socket->SendInteger(Data))
    return false;
  return true;
}

bool GameServerClient::SendName(const unsigned int PlayerId, const string PlayerName)
{
  if (!Socket->SendInteger(ND_Name))
    return false;
  if (!Socket->SendInteger(PlayerId))
    return false;
  if (!Socket->SendString(PlayerName.c_str()))
    return false;
  return true;
}

bool GameServerClient::SendNetworkRequest(const NetworkRequestType Request)
{
  if (!Socket->SendInteger(ND_NetworkRequest))
    return false;
  if (!Socket->SendInteger(Request))
    return false;
  return true;
}

bool GameServerClient::SendNotification(const NotificationType Notification)
{
  if (!Socket->SendInteger(ND_Notification))
    return false;
  if (!Socket->SendInteger(Notification))
    return false;
  return true;
}

bool GameServerClient::SendPlayerId(const unsigned int Id)
{
  if (!Socket->SendInteger(ND_PlayerId))
    return false;
  if (!Socket->SendInteger(Id))
    return false;
  return true;
}

bool GameServerClient::SendPlayerType(const unsigned int PlayerId, const PlayerType Type)
{
  if (!Socket->SendInteger(ND_PlayerType))
    return false;
  if (!Socket->SendInteger(PlayerId))
    return false;
  if (!Socket->SendInteger(Type))
    return false;
  return true;
}

bool GameServerClient::SendPlayerJoined(const unsigned int PlayerId, const string PlayerName)
{
  if (!Socket->SendInteger(ND_PlayerJoined))
    return false;
  if (!Socket->SendInteger(PlayerId))
    return false;
  if (!Socket->SendString(PlayerName.c_str()))
    return false;
  return true;
}

bool GameServerClient::SendPlayerLeft(const unsigned int PlayerId)
{
  if (!Socket->SendInteger(ND_PlayerLeft))
    return false;
  if (!Socket->SendInteger(PlayerId))
    return false;
  return true;
}

bool GameServerClient::SendPlayerReady(const unsigned int PlayerId)
{
  if (!Socket->SendInteger(ND_PlayerReady))
    return false;
  if (!Socket->SendInteger(PlayerId))
    return false;
  return true;
}

bool GameServerClient::SendPlayerRequest(const PlayerRequestType Request)
{
  if (!Socket->SendInteger(ND_PlayerRequest))
    return false;
  if (!Socket->SendInteger(Request))
    return false;
  return true;
}

bool GameServerClient::SendPromoteTo(const int Type)
{
  if (!Socket->SendInteger(ND_PromoteTo))
    return false;
  if (!Socket->SendInteger(Type))
    return false;
  return true;
}

bool GameServerClient::SendRoomInfo(const unsigned int RoomId, const string RoomName, const bool RoomPrivate, const int PlayerCount)
{
  if (!Socket->SendInteger(ND_RoomInfo))
    return false;
  if (!Socket->SendInteger(RoomId))
    return false;
  if (!Socket->SendString(RoomName.c_str()))
    return false;
  if (!Socket->SendInteger(RoomPrivate))
    return false;
  if (!Socket->SendInteger(PlayerCount))
    return false;
  return true;
}

bool GameServerClient::SendTime(const unsigned int PlayerId, const unsigned long Time)
{
  if (!Socket->SendInteger(ND_PlayerTime))
    return false;
  if (!Socket->SendInteger(PlayerId))
    return false;
  if (!Socket->SendInteger(Time))
    return false;
  return true;
}

// Private static functions ----------------------------------------------------

int GameServerClient::ReceiveData(GameServerClient* Client)
{
  if (Client != NULL)
  {
    long DataType = Client->Socket->ReceiveInteger();
    switch (DataType)
    {
      case -1:
      {
        return 0;
      }
      case ND_CreateRoom:
      {
        char* RoomName = Client->Socket->ReceiveString();
        if (RoomName == NULL)
          return 0;
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received a request from player " << Client->Id << " to create a room named " << RoomName << std::endl;
  #endif
        Client->Server->LeaveRoom(Client);
        Client->Server->JoinRoom(Client, Client->Server->CreateRoom(Client, RoomName));
        delete[] RoomName;
        break;
      }
      case ND_JoinRoom:
      {
        long RoomId = Client->Socket->ReceiveInteger();
        if (RoomId == -1)
          return 0;
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received a request from player " << Client->Id << " to join the room " << RoomId << std::endl;
  #endif
        Client->Server->LeaveRoom(Client);
        Client->Server->JoinRoom(Client, Client->Server->FindRoom(RoomId));
        break;
      }
      case ND_LeaveRoom:
      {
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received a request from player " << Client->Id << " to leave the room" << std::endl;
  #endif
        Client->Server->LeaveRoom(Client);
        break;
      }
      case ND_RemovePlayer:
      {
        long PlayerId = Client->Socket->ReceiveInteger();
        if (PlayerId == -1)
          return 0;
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received a request from player " << Client->Id << " to kick player " << PlayerId << " from the room" << std::endl;
  #endif
        if (Client->Room != NULL && Client == Client->Room->Owner)
          Client->Server->LeaveRoom(Client->Server->FindPlayer(PlayerId));
        break;
      }
      case ND_ChangeType:
      {
        PlayerType Type = (PlayerType)Client->Socket->ReceiveInteger();
        if (Type == -1)
          return 0;
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received a request from player " << Client->Id << " to change his type to " << Type << std::endl;
  #endif
        Client->Server->ChangeSeat(Client, Type);
        break;
      }
      case ND_Disconnection:
      {
  #ifdef DEBUG
      /* Output to log */
      std::cout << "Player " << Client->Id << " disconnected from the server" << std::endl;
  #endif
        return 0;
      }
      case ND_GameData:
      {
        long DataSize = (unsigned long)Client->Socket->ReceiveInteger();
        if (DataSize == -1)
          return 0;
        unsigned char* Data = new unsigned char[DataSize];
        if (Data == NULL)
          return 0;
        if (Client->Socket->ReceiveBytes(Data, DataSize) == (unsigned long)DataSize)
          Client->Server->SendGameData(Client, Data, DataSize);
        delete[] Data;
        break;
      }
      case ND_Message:
      {
        char* Message = Client->Socket->ReceiveString();
        if (Message == NULL)
          return 0;
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received a message from player " << Client->Id << " : " << Message << std::endl;
  #endif
        Client->Server->SendMessage(Client, Message);
        delete[] Message;
        break;
      }
      case ND_Move:
      {
        long Data = Client->Socket->ReceiveInteger();
        if (Data == -1)
          return 0;
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received a move from player " << Client->Id << std::endl;
  #endif
        if (Client->Room != NULL && (Client == Client->Room->WhitePlayer || Client == Client->Room->BlackPlayer))
          Client->Server->SendMove(Client->Room, Data);
        break;
      }
      case ND_Name:
      {
        char* PlayerName = Client->Socket->ReceiveString();
        if (PlayerName == NULL)
          return 0;
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received player " << Client->Id << "'s name : " << PlayerName << std::endl;
  #endif
        Client->Server->SetName(Client, PlayerName);
        delete[] PlayerName;
        break;
      }
      case ND_NetworkRequest:
      {
        NetworkRequestType Request = (NetworkRequestType)Client->Socket->ReceiveInteger();
        if (Request == -1)
          return 0;
        switch (Request)
        {
          case RoomList:
          {
            Client->Server->SendRoomList(Client);
            break;
          }
          default:
            break;
        }
        break;
      }
      case ND_Notification:
      {
        NotificationType Notification = (NotificationType)Client->Socket->ReceiveInteger();
        if (Notification == -1)
          return 0;
        GameServerRoom* Room = Client->Room;
        if (Room != NULL && (Client == Room->WhitePlayer || Client == Room->BlackPlayer))
        {
          switch (Notification)
          {
            case IAmReady:
            {
  #ifdef DEBUG
              /* Output to log */
              std::cout << "Received a ready notification from player " << Client->Id << std::endl;
  #endif
              Client->Server->SetReady(Client);
              break;
            }
            case IResign:
            {
              Client->Server->SendNotification(Room, Resigned);
              Client->Server->EndGame(Room);
              break;
            }
            case GamePaused:
            {
              Room->Paused = true;
              Client->Server->SendNotification(Room, GamePaused);
              break;
            }
            case GameResumed:
            {
              Room->Paused = false;
              Client->Server->SendNotification(Room, GameResumed);
              break;
            }
            case DrawRequestAccepted:
            {
              Client->Server->SendNotification(Room, GameDrawed);
              Client->Server->EndGame(Room);
              break;
            }
            case TakebackRequestAccepted:
            {
              Client->Server->SendNotification(Room, TookbackMove);
              break;
            }
            case GameEnded:
            {
              Client->Server->EndGame(Room);
              break;
            }
            default:
              Client->Server->SendNotification(Room, Notification);
          }
        }
        break;
      }
      case ND_PlayerRequest:
      {
        PlayerRequestType Request = (PlayerRequestType)Client->Socket->ReceiveInteger();
        if (Request == -1)
          return 0;
        Client->Server->SendRequest(Client, Request);
        break;
      }
      case ND_PlayerTime:
      {
        long Time = Client->Socket->ReceiveInteger();
        if (Time == -1)
          return 0;
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received time from player " << Client->Id << std::endl;
  #endif
        if (Client->Room != NULL && (Client == Client->Room->WhitePlayer || Client == Client->Room->BlackPlayer))
          Client->Server->SendTime(Client->Room, Client->Id, Time);
        break;
      }
      case ND_PromoteTo:
      {
        int Type = Client->Socket->ReceiveInteger();
        if (Type == -1)
          return 0;
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received a piece promotion to " << Type << " from player " << Client->Id << std::endl;
  #endif
        Client->Server->SendPromotion(Client->Room, Type);
        break;
      }
      default:
        return 0;
    }
    return 1;
  }
  return 0;
}

// Private functions -----------------------------------------------------------

unsigned int GameServerClient::Run()
{
  /* Exchange version information with the client */
  Socket->SendString(GameServer::Id);
  Socket->SendInteger(GameServer::Version);
  char* Str = Socket->ReceiveString();
  Version = Socket->ReceiveInteger();

  /* Validate the version information */
  if (Str != NULL && strcmp(GameServer::Id,Str) == 0 && Version >= GameServer::SupportedVersion)
  {
    /* Send the new id to the player */
    SendPlayerId(Id);

    while (ReceiveData(this) > 0);

    /* Remove the player from the server */
    Server->LeaveRoom(this);
  }

  /* Close the socket */
  Server->RemoveClient(this);
  Socket->Close();

  /* Clean up */
  delete Socket;
  delete[] Str;

  return 0;
}
