/*
* GameServer.cpp - Game server class that handles all the players connections.
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
#include "gameserver.h"

#ifdef DEBUG
#include <iostream>
#endif

struct ConnectionInfo
{
  GameServer* Server;
  SOCKET SocketId;
};

/* Initialise static class members */
const int GameServer::Port = 2570;
const char* GameServer::Id = "AlphaChess";
const int GameServer::SupportedVersion = 400;
const int GameServer::Version = 400;

// Public functions, executed in the main thread (UI) --------------------------

GameServer::GameServer()
{
  hServerThread = NULL;
  PlayerIdCounter = 0;
  RoomIdCounter = 0;
  Socket.SetBlocking(false);
}

GameServer::~GameServer()
{
  Close();
}

bool GameServer::Open()
{
  if (!Socket.IsOpened())
  {
    /* Create a server socket that listens for connections */
    Socket.Open(Port);

    unsigned long ThreadId;
    /* Start the threads */
    hServerThread = CreateThread(NULL,0,GameServer::ServerThread,(void*)this,0,&ThreadId);
    if (hServerThread != NULL)
      return true;
  }
  return false;
}

bool GameServer::Close()
{
  /* Close the server socket */
  Socket.Close();

  /* Disconnect all the players (the thread will then stop automatically and delete the player and socket) */
  PlayerInfo* Ptr = Players.GetFirst();
  while (Ptr != NULL)
  {
    Ptr->Socket->SendInteger(ND_Disconnection);
    Ptr = Players.GetNext();
  }

  /* Delete all the rooms */
  while (Rooms.Size() > 0)
    delete Rooms.Remove();

  /* Wait for the server thread to stop */
  unsigned long ExitCode = STILL_ACTIVE;
  while (GetExitCodeThread(hServerThread,&ExitCode) != 0 && ExitCode == STILL_ACTIVE)
    Sleep(100);

  return true;
}

// Private functions -----------------------------------------------------------

void GameServer::CloseConnection(PlayerInfo* Player)
{
  if (Player != NULL)
  {
    /* Remove the player from the room */
    LeaveRoom(Player);
    /* Remove the player from the server */
    Players.Remove(Player);
    delete Player;
  }
  /* Close the socket */
  Player->Socket->Close();
}

void GameServer::ChangePlayerType(PlayerInfo* Player, PlayerType Type)
{
  /* Get the room the player is in */
  RoomInfo* Room = Player->Room;
  if (Room != NULL)
  {
    /* Validate that the change is possible */
    if ((Type == ObserverType && (Player == Room->BlackPlayer || Player == Room->WhitePlayer))
      || (Type == BlackPlayerType && Room->BlackPlayer == NULL) || (Type == WhitePlayerType && Room->WhitePlayer == NULL))
    {
      /* Change the player's type */
      if (Player == Room->BlackPlayer)
        Room->BlackPlayer = NULL;
      else if (Player == Room->WhitePlayer)
        Room->WhitePlayer = NULL;
      else
        Room->Observers.Remove(Player);
      if (Type == BlackPlayerType)
        Room->BlackPlayer = Player;
      else if (Type == WhitePlayerType)
        Room->WhitePlayer = Player;
      else if (Type == ObserverType)
        Room->Observers.Add(Player);
      /* Reset ready flag */
      Player->Ready = false;
      /* Notify the room's players that the player's type has changed */
      PlayerInfo* Ptr;
      if (Room->WhitePlayer != NULL)
        SendPlayerType(Room->WhitePlayer->Socket,Player->PlayerId,Type);
      if (Room->BlackPlayer != NULL)
        SendPlayerType(Room->BlackPlayer->Socket,Player->PlayerId,Type);
      Ptr = Room->Observers.GetFirst();
      while (Ptr != NULL)
      {
        SendPlayerType(Ptr->Socket,Player->PlayerId,Type);
        Ptr = Room->Observers.GetNext();
      }
    }
  }
}

void GameServer::JoinRoom(RoomInfo* Room, PlayerInfo* Player)
{
  /* Notify the player that he his joining the room */
  SendNotification(Player->Socket, JoinedRoom);
  /* Notify the room's players that a player is joining */
  PlayerInfo* Ptr;
  if (Room->WhitePlayer != NULL)
    SendPlayerJoined(Room->WhitePlayer->Socket,Player->PlayerId,Player->Name);
  if (Room->BlackPlayer != NULL)
    SendPlayerJoined(Room->BlackPlayer->Socket,Player->PlayerId,Player->Name);
  Ptr = Room->Observers.GetFirst();
  while (Ptr != NULL)
  {
    SendPlayerJoined(Ptr->Socket,Player->PlayerId,Player->Name);
    Ptr = Room->Observers.GetNext();
  }
  /* Send info on the room to the player */
  if (Room->WhitePlayer != NULL)
  {
    SendPlayerJoined(Player->Socket,Room->WhitePlayer->PlayerId,Room->WhitePlayer->Name);
    SendPlayerType(Player->Socket,Room->WhitePlayer->PlayerId,WhitePlayerType);
  }
  if (Room->BlackPlayer != NULL)
  {
    SendPlayerJoined(Player->Socket,Room->BlackPlayer->PlayerId,Room->BlackPlayer->Name);
    SendPlayerType(Player->Socket,Room->BlackPlayer->PlayerId,BlackPlayerType);
  }
  Ptr = Room->Observers.GetFirst();
  while (Ptr != NULL)
  {
    SendPlayerJoined(Player->Socket,Ptr->PlayerId,Ptr->Name);
    Ptr = Room->Observers.GetNext();
  }
  /* Add the player to the room */
  Room->Observers.Add(Player);
  Player->Room = Room;
  Player->Ready = false;
  if (Room->Owner == Player)
    Player->Synched = true;
  else
  {
    SendNetworkRequest(Room->Owner->Socket, GameData);
    Player->Synched = false;
  }
}

void GameServer::LeaveRoom(PlayerInfo* Player)
{
  /* Find the room the player is in */
  RoomInfo* Room = Player->Room;
  if (Room != NULL)
  {
    /* Notify the player that he left the room */
    SendNotification(Player->Socket, LeftRoom);
    /* Remove the player from the room */
    if (Room->WhitePlayer == Player)
      Room->WhitePlayer = NULL;
    else if (Room->BlackPlayer == Player)
      Room->BlackPlayer = NULL;
    else
      Room->Observers.Remove(Player);
    Player->Room = NULL;
    Player->Ready = false;
    /* Delete the room if there are no more players in it */
    if (Room->WhitePlayer == NULL && Room->BlackPlayer == NULL && Room->Observers.Size() == 0)
    {
      Rooms.Remove(Room);
      delete Room;
    }
    else
    {
      /* Change the room's owner */
      if (Room->Owner == Player)
      {
        if (Room->WhitePlayer != NULL)
          Room->Owner = Room->WhitePlayer;
        else if (Room->BlackPlayer != NULL)
          Room->Owner = Room->BlackPlayer;
        else
          Room->Owner = Room->Observers.GetFirst();
        /* Notify the player that he is the new game host */
        SendHostChanged(Room->Owner->Socket, Room->Owner->PlayerId);
      }
      PlayerInfo* Ptr;
      /* Notify the room's players that a player left */
      if (Room->WhitePlayer != NULL)
        SendPlayerLeft(Room->WhitePlayer->Socket,Player->PlayerId);
      if (Room->BlackPlayer != NULL)
        SendPlayerLeft(Room->BlackPlayer->Socket,Player->PlayerId);
      Ptr = Room->Observers.GetFirst();
      while (Ptr != NULL)
      {
        SendPlayerLeft(Ptr->Socket,Player->PlayerId);
        Ptr = Room->Observers.GetNext();
      }
    }
  }
}

PlayerInfo* GameServer::NewPlayer()
{
  /* Increment the player id count */
  if (PlayerIdCounter == UINT_MAX)
    PlayerIdCounter = 1;
  else
    PlayerIdCounter++;
  /* Create a new player */
  PlayerInfo* Player = new PlayerInfo;
  Player->PlayerId = PlayerIdCounter;
  Player->Name[0] = '\0';
  Player->Ready = false;
  Player->Synched = false;
  Player->Room = NULL;
  Player->Socket = NULL;
  return Player;
}

RoomInfo* GameServer::NewRoom()
{
  /* Increment the room id count */
  if (RoomIdCounter == UINT_MAX)
    RoomIdCounter = 1;
  else
    RoomIdCounter++;
  /* Create a new room */
  RoomInfo* Room = new RoomInfo;
  Room->RoomId = RoomIdCounter;
  Room->Locked = false;
  Room->Paused = false;
  Room->Name[0] = '\0';
  Room->Owner = NULL;
  Room->BlackPlayer = 0;
  Room->WhitePlayer = 0;
  return Room;
}

PlayerInfo* GameServer::OpenConnection(TCPClientSocket* Socket)
{
  PlayerInfo* Player = NULL;

  /* Exchange version information with the client */
  Socket->SendString(Id);
  Socket->SendInteger(Version);
  char* RemoteId = Socket->ReceiveString();
  int RemoteVersion = Socket->ReceiveInteger();

  /* Validate the version information */
  if (strcmp(Id,RemoteId) == 0 && RemoteVersion >= SupportedVersion)
  {
    /* Store information on the player */
    Player = NewPlayer();
    Player->Socket = Socket;
    /* Add the player to the list */
    Players.Add(Player);
    /* Send the new id to the player */
    SendPlayerId(Socket,Player->PlayerId);
  }
  delete[] RemoteId;
  return Player;
}

void GameServer::ReceiveData(PlayerInfo* Player)
{
  if (Player != NULL && Player->Socket != NULL)
  {
    while (true)
    {
      int DataType = Player->Socket->ReceiveInteger();
      switch (DataType)
      {
        case ND_CreateRoom:
        {
          char* RoomName = Player->Socket->ReceiveString();
#ifdef DEBUG
          /* Output to log */
          std::cout << "Received a request from player " << Player->PlayerId << " to create a room named " << RoomName << std::endl;
#endif
          /* Leave the current room, if any */
          LeaveRoom(Player);
          /* Create a new room */
          RoomInfo* Room = NewRoom();
          strncpy(Room->Name,RoomName,sizeof(Room->Name));
          Room->Owner = Player;
          /* Add the room to the list */
          Rooms.Add(Room);
          /* Add the player to the room */
          JoinRoom(Room, Player);
          /* Notify the player that he is the game host */
          SendHostChanged(Player->Socket, Player->PlayerId);
          delete[] RoomName;
          break;
        }
        case ND_JoinRoom:
        {
          unsigned int RoomId = Player->Socket->ReceiveInteger();
#ifdef DEBUG
          /* Output to log */
          std::cout << "Received a request from player " << Player->PlayerId << " to join the room " << RoomId << std::endl;
#endif
          /* Get the room */
          if (RoomId != 0)
          {
            RoomInfo* Room = Rooms.GetFirst();
            while (Room != NULL && Room->RoomId != RoomId)
              Room = Rooms.GetNext();
            if (Room != NULL && Room != Player->Room && Room->Locked == false)
            {
              /* Leave the current room, if any */
              LeaveRoom(Player);
              /* Add the player to the room */
              JoinRoom(Room, Player);
            }
          }
          break;
        }
        case ND_LeaveRoom:
        {
#ifdef DEBUG
          /* Output to log */
          std::cout << "Received a request from player " << Player->PlayerId << " to leave the room" << std::endl;
#endif
          /* Remove the player from the room */
          LeaveRoom(Player);
          break;
        }
        case ND_RemovePlayer:
        {
          unsigned int PlayerId = Player->Socket->ReceiveInteger();
#ifdef DEBUG
          /* Output to log */
          std::cout << "Received a request from player " << Player->PlayerId << " to kick player " << PlayerId << " from the room" << std::endl;
#endif
          /* Get the room the player is in */
          RoomInfo* Room = Player->Room;
          if (Room != NULL && Room->Owner == Player)
          {
            PlayerInfo* OtherPlayer;
            /* Find the player to remove */
            if (Room->BlackPlayer != NULL && Room->BlackPlayer->PlayerId == PlayerId)
              OtherPlayer = Room->BlackPlayer;
            else if (Room->WhitePlayer != NULL && Room->WhitePlayer->PlayerId == PlayerId)
              OtherPlayer = Room->WhitePlayer;
            else
            {
              OtherPlayer = Room->Observers.GetFirst();
              while (OtherPlayer != NULL && OtherPlayer->PlayerId != PlayerId)
                OtherPlayer = Room->Observers.GetNext();
            }
            /* Remove the player from the room */
            if (OtherPlayer != NULL)
              LeaveRoom(OtherPlayer);
          }
          break;
        }
        case ND_ChangeType:
        {
          PlayerType Type = (PlayerType)Player->Socket->ReceiveInteger();
#ifdef DEBUG
          /* Output to log */
          std::cout << "Received a request from player " << Player->PlayerId << " to change his type to " << Type << std::endl;
#endif
          /* Change the player's type */
          ChangePlayerType(Player, Type);
          break;
        }
        case ND_Disconnection:
        {
#ifdef DEBUG
        /* Output to log */
        std::cout << "Player " << Player->PlayerId << " disconnected from the server" << std::endl;
#endif
          return;
        }
        case ND_GameData:
        {
          /* Read the size of the incoming data */
          unsigned long DataSize = (unsigned long)Player->Socket->ReceiveInteger();
          if (DataSize > 0)
          {
            /* Receive the incoming data */
            unsigned char* Data = new unsigned char[DataSize];
            memset(Data,sizeof(Data),0);
            if (Player->Socket->ReceiveBytes(Data, DataSize) == DataSize)
            {
              /* Get the room the player is in */
              RoomInfo* Room = Player->Room;
              if (Room != NULL)
              {
                /* Send the game data to the entire room */
                if (Room->WhitePlayer != NULL && !Room->WhitePlayer->Synched)
                {
                  SendGameData(Room->WhitePlayer->Socket,Data,DataSize);
                  Room->WhitePlayer->Synched = true;
                }
                if (Room->BlackPlayer != NULL && !Room->BlackPlayer->Synched)
                {
                  SendGameData(Room->BlackPlayer->Socket,Data,DataSize);
                  Room->BlackPlayer->Synched = true;
                }
                PlayerInfo* Ptr = Room->Observers.GetFirst();
                while (Ptr != NULL)
                {
                  if (!Ptr->Synched)
                    SendGameData(Ptr->Socket,Data,DataSize);
                  Ptr->Synched = true;
                  Ptr = Room->Observers.GetNext();
                }
              }
            }
            /* Clean up */
            delete[] Data;
          }
          break;
        }
        case ND_Message:
        {
          char* Message = Player->Socket->ReceiveString();
#ifdef DEBUG
          /* Output to log */
          std::cout << "Received a message from player " << Player->PlayerId << " : " << Message << std::endl;
#endif
          /* Get the room the player is in */
          RoomInfo* Room = Player->Room;
          if (Room != NULL)
          {
            /* Send the message to the entire room */
            if (Room->WhitePlayer != NULL)
              SendMessage(Room->WhitePlayer->Socket,Player->PlayerId,Message);
            if (Room->BlackPlayer != NULL)
              SendMessage(Room->BlackPlayer->Socket,Player->PlayerId,Message);
            PlayerInfo* Ptr = Room->Observers.GetFirst();
            while (Ptr != NULL)
            {
              SendMessage(Ptr->Socket,Player->PlayerId,Message);
              Ptr = Room->Observers.GetNext();
            }
          }
          delete[] Message;
          break;
        }
        case ND_Move:
        {
          unsigned long Data = Player->Socket->ReceiveInteger();
#ifdef DEBUG
          /* Output to log */
          std::cout << "Received a move from player " << Player->PlayerId << std::endl;
#endif
          /* Get the room the player is in */
          RoomInfo* Room = Player->Room;
          if (Room != NULL && (Player == Room->WhitePlayer || Player == Room->BlackPlayer))
          {
            PlayerInfo* Ptr;
            /* Send the move to the entire room */
            if (Room->WhitePlayer != NULL)
              SendMove(Room->WhitePlayer->Socket,Data);
            if (Room->BlackPlayer != NULL)
              SendMove(Room->BlackPlayer->Socket,Data);
            Ptr = Room->Observers.GetFirst();
            while (Ptr != NULL)
            {
              SendMove(Ptr->Socket,Data);
              Ptr = Room->Observers.GetNext();
            }
          }
          break;
        }
        case ND_Name:
        {
          char* PlayerName = Player->Socket->ReceiveString();
#ifdef DEBUG
          /* Output to log */
          std::cout << "Received player " << Player->PlayerId << "'s name : " << PlayerName << std::endl;
#endif
          /* Change the client's information */
          strcpy(Player->Name,PlayerName);
          /* Get the room the player is in */
          RoomInfo* Room = Player->Room;
          if (Room != NULL)
          {
            /* Send the name to the entire room */
            if (Room->WhitePlayer != NULL)
              SendName(Room->WhitePlayer->Socket,Player->PlayerId,Player->Name);
            if (Room->BlackPlayer != NULL)
              SendName(Room->BlackPlayer->Socket,Player->PlayerId,Player->Name);
            PlayerInfo* Ptr = Room->Observers.GetFirst();
            while (Ptr != NULL)
            {
              SendName(Ptr->Socket,Player->PlayerId,Player->Name);
              Ptr = Room->Observers.GetNext();
            }
          }
          else
            /* Send the name back to the player */
            SendName(Player->Socket,Player->PlayerId,Player->Name);
          delete[] PlayerName;
          break;
        }
        case ND_NetworkRequest:
        {
          NetworkRequestType Request = (NetworkRequestType)Player->Socket->ReceiveInteger();
          switch (Request)
          {
            case RoomList:
            {
              /* Send the list of rooms to the client */
              RoomInfo* Ptr = Rooms.GetFirst();
              while (Ptr != NULL)
              {
                SendRoomInfo(Player->Socket,Ptr->RoomId,Ptr->Name,Ptr->Locked,(Ptr->BlackPlayer != NULL ? 1 : 0) + (Ptr->WhitePlayer != NULL ? 1 : 0) + Ptr->Observers.Size());
                Ptr = Rooms.GetNext();
              }
              break;
            }
            default:
              break;
          }
          break;
        }
        case ND_Notification:
        {
          NotificationType Notification = (NotificationType)Player->Socket->ReceiveInteger();
          /* Get the room the player is in */
          RoomInfo* Room = Player->Room;
          if (Room != NULL && (Player == Room->WhitePlayer || Player == Room->BlackPlayer))
          {
            if (Notification != IAmReady && Notification != IResign && Notification != GameEnded)
            {
              /* Send the notification to the entire room */
              if (Room->WhitePlayer != NULL)
                SendNotification(Room->WhitePlayer->Socket,Notification);
              if (Room->BlackPlayer != NULL)
                SendNotification(Room->BlackPlayer->Socket,Notification);
              PlayerInfo* Ptr = Room->Observers.GetFirst();
              while (Ptr != NULL)
              {
                SendNotification(Ptr->Socket,Notification);
                Ptr = Room->Observers.GetNext();
              }
            }
            switch (Notification)
            {
              case IAmReady:
              {
#ifdef DEBUG
                /* Output to log */
                std::cout << "Received a ready notification from player " << Player->PlayerId << std::endl;
#endif
                PlayerInfo* Ptr;
                /* Set the ready flag */
                Player->Ready = true;
                /* Notify all the players in the room that the player is ready */
                if (Room->WhitePlayer != NULL)
                  SendPlayerReady(Room->WhitePlayer->Socket,Player->PlayerId);
                if (Room->BlackPlayer != NULL)
                  SendPlayerReady(Room->BlackPlayer->Socket,Player->PlayerId);
                Ptr = Room->Observers.GetFirst();
                while (Ptr != NULL)
                {
                  SendPlayerReady(Ptr->Socket,Player->PlayerId);
                  Ptr = Room->Observers.GetNext();
                }
                if (Room->WhitePlayer != NULL && Room->WhitePlayer->Ready && Room->BlackPlayer != NULL && Room->BlackPlayer->Ready)
                {
                  /* Reset the ready flags */
                  Room->WhitePlayer->Ready = false;
                  Room->BlackPlayer->Ready = false;
                  /* Notify all the players in the room that the game is starting */
                  if (Room->WhitePlayer != NULL)
                    SendNotification(Room->WhitePlayer->Socket,GameStarted);
                  if (Room->BlackPlayer != NULL)
                    SendNotification(Room->BlackPlayer->Socket,GameStarted);
                  Ptr = Room->Observers.GetFirst();
                  while (Ptr != NULL)
                  {
                    SendNotification(Ptr->Socket,GameStarted);
                    Ptr = Room->Observers.GetNext();
                  }
                }
                break;
              }
              case IResign:
              {
                /* Send notification to the entire room that a player resigned */
                if (Room->WhitePlayer != NULL)
                  SendNotification(Room->WhitePlayer->Socket,Resigned);
                if (Room->BlackPlayer != NULL)
                  SendNotification(Room->BlackPlayer->Socket,Resigned);
                PlayerInfo* Ptr = Room->Observers.GetFirst();
                while (Ptr != NULL)
                {
                  SendNotification(Ptr->Socket,Resigned);
                  Ptr = Room->Observers.GetNext();
                }
                break;
              }
              case GamePaused:
              {
                Room->Paused = true;
                /* Send notification to the entire room that the game is paused */
                if (Room->WhitePlayer != NULL)
                  SendNotification(Room->WhitePlayer->Socket,GamePaused);
                if (Room->BlackPlayer != NULL)
                  SendNotification(Room->BlackPlayer->Socket,GamePaused);
                PlayerInfo* Ptr = Room->Observers.GetFirst();
                while (Ptr != NULL)
                {
                  SendNotification(Ptr->Socket,GamePaused);
                  Ptr = Room->Observers.GetNext();
                }
                break;
              }
              case GameResumed:
              {
                Room->Paused = false;
                /* Send notification to the entire room that the game is paused */
                if (Room->WhitePlayer != NULL)
                  SendNotification(Room->WhitePlayer->Socket,GameResumed);
                if (Room->BlackPlayer != NULL)
                  SendNotification(Room->BlackPlayer->Socket,GameResumed);
                PlayerInfo* Ptr = Room->Observers.GetFirst();
                while (Ptr != NULL)
                {
                  SendNotification(Ptr->Socket,GameResumed);
                  Ptr = Room->Observers.GetNext();
                }
                break;
              }
              case DrawRequestAccepted:
              {
                PlayerInfo* Ptr;
                /* Send notification to the entire room that the game is drawed */
                if (Room->WhitePlayer != NULL)
                  SendNotification(Room->WhitePlayer->Socket,GameDrawed);
                if (Room->BlackPlayer != NULL)
                  SendNotification(Room->BlackPlayer->Socket,GameDrawed);
                Ptr = Room->Observers.GetFirst();
                while (Ptr != NULL)
                {
                  SendNotification(Ptr->Socket,GameDrawed);
                  Ptr = Room->Observers.GetNext();
                }
                break;
              }
              case TakebackRequestAccepted:
              {
                /* Send notification to the entire room that the move was took back */
                if (Room->WhitePlayer != NULL)
                  SendNotification(Room->WhitePlayer->Socket,TookbackMove);
                if (Room->BlackPlayer != NULL)
                  SendNotification(Room->BlackPlayer->Socket,TookbackMove);
                PlayerInfo* Ptr = Room->Observers.GetFirst();
                while (Ptr != NULL)
                {
                  SendNotification(Ptr->Socket,TookbackMove);
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
          PlayerRequestType Request = (PlayerRequestType)Player->Socket->ReceiveInteger();
          /* Get the room the player is in */
          RoomInfo* Room = Player->Room;
          if (Room != NULL)
          {
            /* Send the request to the opposing player */
            if (Player == Room->WhitePlayer && Room->BlackPlayer != NULL)
              SendPlayerRequest(Room->BlackPlayer->Socket,Request);
            if (Player == Room->BlackPlayer && Room->WhitePlayer != NULL)
              SendPlayerRequest(Room->WhitePlayer->Socket,Request);
          }
          break;
        }
        case ND_PlayerTime:
        {
          unsigned long Time = Player->Socket->ReceiveInteger();
#ifdef DEBUG
          /* Output to log */
          std::cout << "Received time from player " << Player->PlayerId << std::endl;
#endif
          /* Get the room the player is in */
          RoomInfo* Room = Player->Room;
          if (Room != NULL && (Player == Room->WhitePlayer || Player == Room->BlackPlayer))
          {
            PlayerInfo* Ptr;
            /* Send the time to the entire room */
            if (Room->WhitePlayer != NULL && Room->WhitePlayer != Player)
              SendTime(Room->WhitePlayer->Socket,Player->PlayerId,Time);
            if (Room->BlackPlayer != NULL && Room->BlackPlayer != Player)
              SendTime(Room->BlackPlayer->Socket,Player->PlayerId,Time);
            Ptr = Room->Observers.GetFirst();
            while (Ptr != NULL)
            {
              SendTime(Ptr->Socket,Player->PlayerId,Time);
              Ptr = Room->Observers.GetNext();
            }
          }
          break;
        }
        case ND_PromoteTo:
        {
          int Type = Player->Socket->ReceiveInteger();
#ifdef DEBUG
          /* Output to log */
          std::cout << "Received a piece promotion to " << Type << " from player " << Player->PlayerId << std::endl;
#endif
          /* Get the room the player is in */
          RoomInfo* Room = Player->Room;
          if (Room != NULL)
          {
            /* Send the promotion to the entire room */
            if (Room->WhitePlayer != NULL)
              SendPromoteTo(Room->WhitePlayer->Socket,Type);
            if (Room->BlackPlayer != NULL)
              SendPromoteTo(Room->BlackPlayer->Socket,Type);
            PlayerInfo* Ptr = Room->Observers.GetFirst();
            while (Ptr != NULL)
            {
              SendPromoteTo(Ptr->Socket,Type);
              Ptr = Room->Observers.GetNext();
            }
          }
          break;
        }
        default:
          return;
      }
    }
  }
}

bool GameServer::SendGameData(TCPClientSocket* Socket, const void* Data, const unsigned long DataSize)
{
  if (!Socket->SendInteger(ND_GameData))
    return false;
  if (!Socket->SendInteger(DataSize))
    return false;
  if (!Socket->SendBytes(Data, DataSize))
    return false;
  return true;
}

bool GameServer::SendHostChanged(TCPClientSocket* Socket, const unsigned int Id)
{
  if (!Socket->SendInteger(ND_HostChanged))
    return false;
  if (!Socket->SendInteger(Id))
    return false;
  return true;
}

bool GameServer::SendMessage(TCPClientSocket* Socket, const unsigned int PlayerId, const char* AMessage)
{
  if (!Socket->SendInteger(ND_Message))
    return false;
  if (!Socket->SendInteger(PlayerId))
    return false;
  if (!Socket->SendString(AMessage))
    return false;
  return true;
}

bool GameServer::SendMove(TCPClientSocket* Socket, const unsigned long Data)
{
  if (!Socket->SendInteger(ND_Move))
    return false;
  if (!Socket->SendInteger(Data))
    return false;
  return true;
}

bool GameServer::SendName(TCPClientSocket* Socket, const unsigned int PlayerId, const char* PlayerName)
{
  if (!Socket->SendInteger(ND_Name))
    return false;
  if (!Socket->SendInteger(PlayerId))
    return false;
  if (!Socket->SendString(PlayerName))
    return false;
  return true;
}

bool GameServer::SendNetworkRequest(TCPClientSocket* Socket, const NetworkRequestType Request)
{
  if (!Socket->SendInteger(ND_NetworkRequest))
    return false;
  if (!Socket->SendInteger(Request))
    return false;
  return true;
}

bool GameServer::SendNotification(TCPClientSocket* Socket, const NotificationType Notification)
{
  if (!Socket->SendInteger(ND_Notification))
    return false;
  if (!Socket->SendInteger(Notification))
    return false;
  return true;
}

bool GameServer::SendPlayerId(TCPClientSocket* Socket, const unsigned int Id)
{
  if (!Socket->SendInteger(ND_PlayerId))
    return false;
  if (!Socket->SendInteger(Id))
    return false;
  return true;
}

bool GameServer::SendPlayerType(TCPClientSocket* Socket, const unsigned int PlayerId, const PlayerType Type)
{
  if (!Socket->SendInteger(ND_PlayerType))
    return false;
  if (!Socket->SendInteger(PlayerId))
    return false;
  if (!Socket->SendInteger(Type))
    return false;
  return true;
}

bool GameServer::SendPlayerJoined(TCPClientSocket* Socket, const unsigned int PlayerId, const char* PlayerName)
{
  if (!Socket->SendInteger(ND_PlayerJoined))
    return false;
  if (!Socket->SendInteger(PlayerId))
    return false;
  if (!Socket->SendString(PlayerName))
    return false;
  return true;
}

bool GameServer::SendPlayerLeft(TCPClientSocket* Socket, const unsigned int PlayerId)
{
  if (!Socket->SendInteger(ND_PlayerLeft))
    return false;
  if (!Socket->SendInteger(PlayerId))
    return false;
  return true;
}

bool GameServer::SendPlayerReady(TCPClientSocket* Socket, const unsigned int PlayerId)
{
  if (!Socket->SendInteger(ND_PlayerReady))
    return false;
  if (!Socket->SendInteger(PlayerId))
    return false;
  return true;
}

bool GameServer::SendPlayerRequest(TCPClientSocket* Socket, const PlayerRequestType Request)
{
  if (!Socket->SendInteger(ND_PlayerRequest))
    return false;
  if (!Socket->SendInteger(Request))
    return false;
  return true;
}

bool GameServer::SendPromoteTo(TCPClientSocket* Socket, const int Type)
{
  if (!Socket->SendInteger(ND_PromoteTo))
    return false;
  if (!Socket->SendInteger(Type))
    return false;
  return true;
}

bool GameServer::SendRoomInfo(TCPClientSocket* Socket, const unsigned int RoomId, const char* RoomName, const bool RoomLocked, const int PlayerCount)
{
  if (!Socket->SendInteger(ND_RoomInfo))
    return false;
  if (!Socket->SendInteger(RoomId))
    return false;
  if (!Socket->SendString(RoomName))
    return false;
  if (!Socket->SendInteger(RoomLocked))
    return false;
  if (!Socket->SendInteger(PlayerCount))
    return false;
  return true;
}

bool GameServer::SendTime(TCPClientSocket* Socket, const unsigned int PlayerId, const unsigned long Time)
{
  if (!Socket->SendInteger(ND_PlayerTime))
    return false;
  if (!Socket->SendInteger(PlayerId))
    return false;
  if (!Socket->SendInteger(Time))
    return false;
  return true;
}

unsigned long __stdcall GameServer::ConnectionThread(void* arg)
{
  ConnectionInfo* Connection = (ConnectionInfo*)arg;
  TCPClientSocket* Client = NULL;
  try
  {
    /* Create a socket object to handle the connection */
    Client = new TCPClientSocket(Connection->SocketId);
    /* Handle the connection */
    PlayerInfo* Player = Connection->Server->OpenConnection(Client);
    Connection->Server->ReceiveData(Player);
    Connection->Server->CloseConnection(Player);
    /* Clean up */
    delete Connection;
    delete Client;
    return 0;
  }
  catch (...)
  {
    delete Connection;
    delete Client;
    return 1;
  }
}

unsigned long __stdcall GameServer::ServerThread(void* arg)
{
  GameServer* Server = (GameServer*)arg;
  while (Server->Socket.IsOpened())
  {
    /* Accept a client socket's connection request */
    SOCKET SocketId = Server->Socket.Accept();
    if (SocketId != INVALID_SOCKET)
    {
      /* Prepare thread data */
      ConnectionInfo* Connection = new ConnectionInfo;
      Connection->Server = Server;
      Connection->SocketId = SocketId;
      /* Start the reception thread */
      unsigned long ThreadId;
      HANDLE Thread = CreateThread(NULL,0,GameServer::ConnectionThread,(void*)Connection,0,&ThreadId);
      if (Thread == NULL)
      {
        delete Connection;
        closesocket(Connection->SocketId);
        return WSAGetLastError();
      }
    }
    else
    {
      int Error = WSAGetLastError();
      if (Error != WSAEWOULDBLOCK && Error != WSAETIMEDOUT)
        return Error;
    }
    Sleep(100);
  }
  return 0;
}
