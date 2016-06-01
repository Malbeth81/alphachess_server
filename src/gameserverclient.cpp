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

bool GameServerClient::SendRoomInfo(const unsigned int RoomId, const string RoomName, const bool RoomLocked, const int PlayerCount)
{
  if (!Socket->SendInteger(ND_RoomInfo))
    return false;
  if (!Socket->SendInteger(RoomId))
    return false;
  if (!Socket->SendString(RoomName.c_str()))
    return false;
  if (!Socket->SendInteger(RoomLocked))
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
    int DataType = Client->Socket->ReceiveInteger();
    switch (DataType)
    {
      case ND_CreateRoom:
      {
        char* RoomName = Client->Socket->ReceiveString();
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received a request from player " << Client->Id << " to create a room named " << RoomName << std::endl;
  #endif
        Client->Server->JoinRoom(Client, Client->Server->CreateRoom(Client, RoomName));
        delete[] RoomName;
        break;
      }
      case ND_JoinRoom:
      {
        unsigned int RoomId = Client->Socket->ReceiveInteger();
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received a request from player " << Client->Id << " to join the room " << RoomId << std::endl;
  #endif
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
        unsigned int PlayerId = Client->Socket->ReceiveInteger();
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received a request from player " << Client->Id << " to kick player " << PlayerId << " from the room" << std::endl;
  #endif
        GameServerRoom* Room = Client->Room;
        if (Room != NULL && Room->Owner == Client)
        {
          /* Find the target player */
          GameServerClient* OtherPlayer = NULL;
          if (Room->BlackPlayer != NULL && Room->BlackPlayer->Id == PlayerId)
            OtherPlayer = Room->BlackPlayer;
          else if (Room->WhitePlayer != NULL && Room->WhitePlayer->Id == PlayerId)
            OtherPlayer = Room->WhitePlayer;
          else
          {
            OtherPlayer = Room->Observers.GetFirst();
            while (OtherPlayer != NULL && OtherPlayer->Id != PlayerId)
              OtherPlayer = Room->Observers.GetNext();
          }

          /* Remove the player from the room */
          Client->Server->LeaveRoom(OtherPlayer);
        }
        break;
      }
      case ND_ChangeType:
      {
        PlayerType Type = (PlayerType)Client->Socket->ReceiveInteger();
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received a request from player " << Client->Id << " to change his type to " << Type << std::endl;
  #endif
        GameServerRoom* Room = Client->Room;
        if (Room != NULL)
        {
          if ((Type == ObserverType && (Client == Room->BlackPlayer || Client == Room->WhitePlayer)) || (Type == BlackPlayerType && Room->BlackPlayer == NULL) || (Type == WhitePlayerType && Room->WhitePlayer == NULL))
          {
            /* Update the player */
            Client->Ready = false;

            /* Update the room */
            if (Client == Room->BlackPlayer)
              Room->BlackPlayer = NULL;
            else if (Client == Room->WhitePlayer)
              Room->WhitePlayer = NULL;
            else
              Room->Observers.Remove(Client);
            if (Type == BlackPlayerType)
              Room->BlackPlayer = Client;
            else if (Type == WhitePlayerType)
              Room->WhitePlayer = Client;
            else if (Type == ObserverType)
              Room->Observers.Add(Client);

            /* Notify the room's players */
            if (Room->WhitePlayer != NULL)
              Room->WhitePlayer->SendPlayerType(Client->Id,Type);
            if (Room->BlackPlayer != NULL)
              Room->BlackPlayer->SendPlayerType(Client->Id,Type);
            GameServerClient* Ptr = Room->Observers.GetFirst();
            while (Ptr != NULL)
            {
              Ptr->SendPlayerType(Client->Id,Type);
              Ptr = Room->Observers.GetNext();
            }
          }
        }
        break;
      }
      case ND_Disconnection:
      {
  #ifdef DEBUG
      /* Output to log */
      std::cout << "Player " << Client->Id << " disconnected from the server" << std::endl;
  #endif
        Client->Server->LeaveRoom(Client);
        return 0;
      }
      case ND_GameData:
      {
        unsigned long DataSize = (unsigned long)Client->Socket->ReceiveInteger();
        if (DataSize > 0)
        {
          unsigned char* Data = new unsigned char[DataSize];
          if (Client->Socket->ReceiveBytes(Data, DataSize) == DataSize)
          {
            GameServerRoom* Room = Client->Room;
            if (Room != NULL)
            {
              /* Forward to unsynchronised players */
              if (Room->WhitePlayer != NULL && !Room->WhitePlayer->Synchronised)
              {
                Room->WhitePlayer->SendGameData(Data,DataSize);
                Room->WhitePlayer->Synchronised = true;
              }
              if (Room->BlackPlayer != NULL && !Room->BlackPlayer->Synchronised)
              {
                Room->BlackPlayer->SendGameData(Data,DataSize);
                Room->BlackPlayer->Synchronised = true;
              }
              GameServerClient* Ptr = Room->Observers.GetFirst();
              while (Ptr != NULL)
              {
                if (!Ptr->Synchronised)
                  Ptr->SendGameData(Data,DataSize);
                Ptr->Synchronised = true;
                Ptr = Room->Observers.GetNext();
              }
            }
          }
          delete[] Data;
        }
        break;
      }
      case ND_Message:
      {
        char* Message = Client->Socket->ReceiveString();
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received a message from player " << Client->Id << " : " << Message << std::endl;
  #endif
        GameServerRoom* Room = Client->Room;
        if (Room != NULL)
        {
          /* Forward to the entire room */
          if (Room->WhitePlayer != NULL)
            Room->WhitePlayer->SendMessage(Client->Id,Message);
          if (Room->BlackPlayer != NULL)
            Room->BlackPlayer->SendMessage(Client->Id,Message);
          GameServerClient* Ptr = Room->Observers.GetFirst();
          while (Ptr != NULL)
          {
            Ptr->SendMessage(Client->Id,Message);
            Ptr = Room->Observers.GetNext();
          }
        }
        delete[] Message;
        break;
      }
      case ND_Move:
      {
        unsigned long Data = Client->Socket->ReceiveInteger();
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received a move from player " << Client->Id << std::endl;
  #endif
        GameServerRoom* Room = Client->Room;
        if (Room != NULL && (Client == Room->WhitePlayer || Client == Room->BlackPlayer))
        {
          /* Forward to the entire room */
          if (Room->WhitePlayer != NULL)
            Room->WhitePlayer->SendMove(Data);
          if (Room->BlackPlayer != NULL)
            Room->BlackPlayer->SendMove(Data);
          GameServerClient* Ptr = Room->Observers.GetFirst();
          while (Ptr != NULL)
          {
            Ptr->SendMove(Data);
            Ptr = Room->Observers.GetNext();
          }
        }
        break;
      }
      case ND_Name:
      {
        char* PlayerName = Client->Socket->ReceiveString();
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received player " << Client->Id << "'s name : " << PlayerName << std::endl;
  #endif
        /* Update the player */
        Client->Name = PlayerName;

        GameServerRoom* Room = Client->Room;
        if (Room != NULL)
        {
          /* Forward to the entire room */
          if (Room->WhitePlayer != NULL)
            Room->WhitePlayer->SendName(Client->Id,Client->Name);
          if (Room->BlackPlayer != NULL)
            Room->BlackPlayer->SendName(Client->Id,Client->Name);
          GameServerClient* Ptr = Room->Observers.GetFirst();
          while (Ptr != NULL)
          {
            Ptr->SendName(Client->Id,Client->Name);
            Ptr = Room->Observers.GetNext();
          }
        }
        else
          Client->SendName(Client->Id,Client->Name);
        delete[] PlayerName;
        break;
      }
      case ND_NetworkRequest:
      {
        NetworkRequestType Request = (NetworkRequestType)Client->Socket->ReceiveInteger();
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
        GameServerRoom* Room = Client->Room;
        if (Room != NULL && (Client == Room->WhitePlayer || Client == Room->BlackPlayer))
        {
          if (Notification != IAmReady && Notification != IResign && Notification != GameEnded)
          {
            /* Forward to the entire room */
            if (Room->WhitePlayer != NULL)
              Room->WhitePlayer->SendNotification(Notification);
            if (Room->BlackPlayer != NULL)
              Room->BlackPlayer->SendNotification(Notification);
            GameServerClient* Ptr = Room->Observers.GetFirst();
            while (Ptr != NULL)
            {
              Ptr->SendNotification(Notification);
              Ptr = Room->Observers.GetNext();
            }
          }
          switch (Notification)
          {
            case IAmReady:
            {
  #ifdef DEBUG
              /* Output to log */
              std::cout << "Received a ready notification from player " << Client->Id << std::endl;
  #endif
              /* Update player */
              Client->Ready = true;

              /* Notify the room's players */
              if (Room->WhitePlayer != NULL)
                Room->WhitePlayer->SendPlayerReady(Client->Id);
              if (Room->BlackPlayer != NULL)
                Room->BlackPlayer->SendPlayerReady(Client->Id);
              GameServerClient* Ptr = Room->Observers.GetFirst();
              while (Ptr != NULL)
              {
                Ptr->SendPlayerReady(Client->Id);
                Ptr = Room->Observers.GetNext();
              }
              if (Room->WhitePlayer != NULL && Room->WhitePlayer->Ready && Room->BlackPlayer != NULL && Room->BlackPlayer->Ready)
              {
                /* Update room players */
                Room->WhitePlayer->Ready = false;
                Room->BlackPlayer->Ready = false;

                /* Notify the room's players */
                if (Room->WhitePlayer != NULL)
                  Room->WhitePlayer->SendNotification(GameStarted);
                if (Room->BlackPlayer != NULL)
                  Room->BlackPlayer->SendNotification(GameStarted);
                Ptr = Room->Observers.GetFirst();
                while (Ptr != NULL)
                {
                  Ptr->SendNotification(GameStarted);
                  Ptr = Room->Observers.GetNext();
                }
              }
              break;
            }
            case IResign:
            {
              /* Notify the room's players */
              if (Room->WhitePlayer != NULL)
                Room->WhitePlayer->SendNotification(Resigned);
              if (Room->BlackPlayer != NULL)
                Room->BlackPlayer->SendNotification(Resigned);
              GameServerClient* Ptr = Room->Observers.GetFirst();
              while (Ptr != NULL)
              {
                Ptr->SendNotification(Resigned);
                Ptr = Room->Observers.GetNext();
              }
              break;
            }
            case GamePaused:
            {
              /* Update room */
              Room->Paused = true;

              /* Notify the room's players */
              if (Room->WhitePlayer != NULL)
                Room->WhitePlayer->SendNotification(GamePaused);
              if (Room->BlackPlayer != NULL)
                Room->BlackPlayer->SendNotification(GamePaused);
              GameServerClient* Ptr = Room->Observers.GetFirst();
              while (Ptr != NULL)
              {
                Ptr->SendNotification(GamePaused);
                Ptr = Room->Observers.GetNext();
              }
              break;
            }
            case GameResumed:
            {
              /* Update room */
              Room->Paused = false;

              /* Notify the room's players */
              if (Room->WhitePlayer != NULL)
                Room->WhitePlayer->SendNotification(GameResumed);
              if (Room->BlackPlayer != NULL)
                Room->BlackPlayer->SendNotification(GameResumed);
              GameServerClient* Ptr = Room->Observers.GetFirst();
              while (Ptr != NULL)
              {
                Ptr->SendNotification(GameResumed);
                Ptr = Room->Observers.GetNext();
              }
              break;
            }
            case DrawRequestAccepted:
            {
              /* Notify the room's players */
              if (Room->WhitePlayer != NULL)
                Room->WhitePlayer->SendNotification(GameDrawed);
              if (Room->BlackPlayer != NULL)
                Room->BlackPlayer->SendNotification(GameDrawed);
              GameServerClient* Ptr = Room->Observers.GetFirst();
              while (Ptr != NULL)
              {
                Ptr->SendNotification(GameDrawed);
                Ptr = Room->Observers.GetNext();
              }
              break;
            }
            case TakebackRequestAccepted:
            {
              /* Notify the room's players */
              if (Room->WhitePlayer != NULL)
                Room->WhitePlayer->SendNotification(TookbackMove);
              if (Room->BlackPlayer != NULL)
                Room->BlackPlayer->SendNotification(TookbackMove);
              GameServerClient* Ptr = Room->Observers.GetFirst();
              while (Ptr != NULL)
              {
                Ptr->SendNotification(TookbackMove);
                Ptr = Room->Observers.GetNext();
              }
              break;
            }
            default:
              break;
          }
        }
        break;
      }
      case ND_PlayerRequest:
      {
        PlayerRequestType Request = (PlayerRequestType)Client->Socket->ReceiveInteger();
        GameServerRoom* Room = Client->Room;
        if (Room != NULL)
        {
          /* Forward to the opposing player */
          if (Client == Room->WhitePlayer && Room->BlackPlayer != NULL)
            Room->BlackPlayer->SendPlayerRequest(Request);
          if (Client == Room->BlackPlayer && Room->WhitePlayer != NULL)
            Room->WhitePlayer->SendPlayerRequest(Request);
        }
        break;
      }
      case ND_PlayerTime:
      {
        unsigned long Time = Client->Socket->ReceiveInteger();
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received time from player " << Client->Id << std::endl;
  #endif
        GameServerRoom* Room = Client->Room;
        if (Room != NULL && (Client == Room->WhitePlayer || Client == Room->BlackPlayer))
        {
          /* Forward to the entire room */
          if (Room->WhitePlayer != NULL && Room->WhitePlayer != Client)
            Room->WhitePlayer->SendTime(Client->Id,Time);
          if (Room->BlackPlayer != NULL && Room->BlackPlayer != Client)
            Room->BlackPlayer->SendTime(Client->Id,Time);
          GameServerClient* Ptr = Room->Observers.GetFirst();
          while (Ptr != NULL)
          {
            Ptr->SendTime(Client->Id,Time);
            Ptr = Room->Observers.GetNext();
          }
        }
        break;
      }
      case ND_PromoteTo:
      {
        int Type = Client->Socket->ReceiveInteger();
  #ifdef DEBUG
        /* Output to log */
        std::cout << "Received a piece promotion to " << Type << " from player " << Client->Id << std::endl;
  #endif
        GameServerRoom* Room = Client->Room;
        if (Room != NULL)
        {
          /* Forward to the entire room */
          if (Room->WhitePlayer != NULL)
            Room->WhitePlayer->SendPromoteTo(Type);
          if (Room->BlackPlayer != NULL)
            Room->BlackPlayer->SendPromoteTo(Type);
          GameServerClient* Ptr = Room->Observers.GetFirst();
          while (Ptr != NULL)
          {
            Ptr->SendPromoteTo(Type);
            Ptr = Room->Observers.GetNext();
          }
        }
        break;
      }
      default:
        break;
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
  if (strcmp(GameServer::Id,Str) == 0 && Version >= GameServer::SupportedVersion)
  {
    /* Send the new id to the player */
    SendPlayerId(Id);

    while (ReceiveData(this));

    /* Remove the player from the server */
    Server->RemoveClient(this);
  }

  /* Close the socket */
  Socket->Close();

  /* Clean up */
  delete Socket;
  delete[] Str;

  return 0;
}
