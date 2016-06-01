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
const int GameServer::SupportedVersion = 402;
const int GameServer::Version = 405;

// Public functions ------------------------------------------------------------

GameServer::GameServer()
{
  ClientIdCounter = 0;
  RoomIdCounter = 0;

  Mutex = CreateMutex(NULL,FALSE,NULL);

  Resume();
}

GameServer::~GameServer()
{
  if (WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
  {
    list<GameServerClient*>::iterator it;
    for (it = Clients.begin(); it != Clients.end(); it++)
      delete *it;
    Clients.clear();

    list<GameServerRoom*>::iterator it2;
    for (it2 = Rooms.begin(); it2 != Rooms.end(); it2++)
      delete *it2;
    Rooms.clear();

    ReleaseMutex(Mutex);
    CloseHandle(Mutex);
  }
}

void GameServer::ChangeSeat(GameServerClient* Client, PlayerType Type)
{
  if (Client != NULL && WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
  {
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
          Room->Observers.remove(Client);
        if (Type == BlackPlayerType)
          Room->BlackPlayer = Client;
        else if (Type == WhitePlayerType)
          Room->WhitePlayer = Client;
        else if (Type == ObserverType)
            Room->Observers.push_back(Client);

        /* Notify the room's players */
        if (Room->WhitePlayer != NULL)
          Room->WhitePlayer->SendPlayerType(Client->Id,Type);
        if (Room->BlackPlayer != NULL)
          Room->BlackPlayer->SendPlayerType(Client->Id,Type);
        list<GameServerClient*>::iterator it;
        for (it = Room->Observers.begin(); it != Room->Observers.end(); it++)
          (*it)->SendPlayerType(Client->Id,Type);
      }
    }
    ReleaseMutex(Mutex);
  }
}

GameServerRoom* GameServer::CreateRoom(GameServerClient* Client, string Name)
{
  GameServerRoom* Room = NULL;
  if (Client != NULL && WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
  {
    if (RoomIdCounter == UINT_MAX)
      RoomIdCounter = 1;
    else
      RoomIdCounter++;

    /* Create a new room */
    Room = new GameServerRoom;
    Room->Id = RoomIdCounter;
    Room->Private = false;
    Room->Paused = false;
    Room->Started = false;
    Room->StartTimestamp = 0;
    Room->Name = Name;
    Room->Owner = Client;
    Room->BlackPlayer = NULL;
    Room->WhitePlayer = NULL;

    /* Add to the list */
    Rooms.push_back(Room);

    ReleaseMutex(Mutex);
  }
  return Room;
}

void GameServer::EndGame(GameServerRoom* Room)
{
  if (Room != NULL && WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
  {
    if (Room->Started)
      /* Notify observers */
      NotifyObservers(RoomGameEnded, Room);

    /* Update room */
    Room->Started = false;
    Room->StartTimestamp = 0;

    ReleaseMutex(Mutex);
  }
}

GameServerClient* GameServer::FindPlayer(unsigned int Id)
{
  GameServerClient* Result = NULL;
  if (WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
  {
    list<GameServerClient*>::iterator it;
    for (it = Clients.begin(); it != Clients.end(); it++)
      if ((*it)->Id == Id)
        Result = (*it);
    ReleaseMutex(Mutex);
  }
  return Result;
}

GameServerRoom* GameServer::FindRoom(unsigned int Id)
{
  GameServerRoom* Result = NULL;
  if (WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
  {
    list<GameServerRoom*>::iterator it;
    for (it = Rooms.begin(); it != Rooms.end(); it++)
      if ((*it)->Id == Id)
        Result = (*it);
    ReleaseMutex(Mutex);
  }
  return Result;
}

list<GameServerClientInfo*>* GameServer::GetClients()
{
  list<GameServerClientInfo*>* List = new list<GameServerClientInfo*>;
  if (WaitForSingleObject(Mutex,1000) == WAIT_OBJECT_0)
  {
    list<GameServerClient*>::iterator it;
    for (it = Clients.begin(); it != Clients.end(); it++)
    {
      GameServerClientInfo* Info = new GameServerClientInfo;
      Info->Id = (*it)->Id;
      Info->Name = (*it)->Name;
      Info->Ready = (*it)->Ready;
      Info->RoomId = ((*it)->Room != NULL ? (*it)->Room->Id : 0);
      Info->Synchronised = (*it)->Synchronised;
      if ((*it)->Room != NULL && (*it) == (*it)->Room->WhitePlayer)
        Info->Type = WhitePlayerType;
      else if ((*it)->Room != NULL && (*it) == (*it)->Room->BlackPlayer)
        Info->Type = BlackPlayerType;
      else
        Info->Type = ObserverType;
      Info->Version = (*it)->Version;
      Info->ConnectionTime = (*it)->ConnectionTime();
      List->push_back(Info);
    }
    ReleaseMutex(Mutex);
  }
  return List;
}

list<GameServerRoomInfo*>* GameServer::GetRooms()
{
  list<GameServerRoomInfo*>* List = new list<GameServerRoomInfo*>;
  if (WaitForSingleObject(Mutex,1000) == WAIT_OBJECT_0)
  {
    list<GameServerRoom*>::iterator it;
    for (it = Rooms.begin(); it != Rooms.end(); it++)
    {
      GameServerRoomInfo* Info = new GameServerRoomInfo;
      Info->Id = (*it)->Id;
      Info->Name = (*it)->Name;
      Info->Private = (*it)->Private;
      Info->Paused = (*it)->Paused;
      Info->Started = (*it)->Started;
      if ((*it)->Started)
      {
        unsigned int TickCount = GetTickCount();
        Info->Time = (TickCount > (*it)->StartTimestamp ? TickCount - (*it)->StartTimestamp : UINT_MAX - (*it)->StartTimestamp + TickCount);
      }
      else
        Info->Time = 0;
      Info->Players = (*it)->Observers.size()+((*it)->BlackPlayer != NULL ? 1 : 0)+((*it)->WhitePlayer != NULL ? 1 : 0);
      List->push_back(Info);
    }
    ReleaseMutex(Mutex);
  }
  return List;
}

void GameServer::JoinRoom(GameServerClient* Client, GameServerRoom* Room)
{
  if (Client != NULL && Room != NULL && WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
  {
    /* Notify the player that he his joining the room */
    Client->SendNotification(JoinedRoom);

    /* Notify the room's players that a player is joining */
    if (Room->WhitePlayer != NULL)
      Room->WhitePlayer->SendPlayerJoined(Client->Id, Client->Name);
    if (Room->BlackPlayer != NULL)
      Room->BlackPlayer->SendPlayerJoined(Client->Id, Client->Name);
    list<GameServerClient*>::iterator it;
    for (it = Room->Observers.begin(); it != Room->Observers.end(); it++)
      (*it)->SendPlayerJoined(Client->Id, Client->Name);

    /* Send info on the room to the player */
    if (Room->WhitePlayer != NULL)
    {
      Client->SendPlayerJoined(Room->WhitePlayer->Id, Room->WhitePlayer->Name);
      Client->SendPlayerType(Room->WhitePlayer->Id, WhitePlayerType);
    }
    if (Room->BlackPlayer != NULL)
    {
      Client->SendPlayerJoined(Room->BlackPlayer->Id, Room->BlackPlayer->Name);
      Client->SendPlayerType(Room->BlackPlayer->Id, BlackPlayerType);
    }
    for (it = Room->Observers.begin(); it != Room->Observers.end(); it++)
      Client->SendPlayerJoined((*it)->Id, (*it)->Name);

    /* Add the player to the room */
    Room->Observers.push_back(Client);
    Client->Room = Room;
    Client->Ready = false;
    if (Room->Owner == Client)
      Client->Synchronised = true;
    else
    {
      Room->Owner->SendNetworkRequest(GameData);
      Client->Synchronised = false;
    }

    /* Notify the player if he is the room owner */
    if (Room->Owner == Client)
      Client->SendHostChanged(Client->Id);

    ReleaseMutex(Mutex);
  }
}

void GameServer::LeaveRoom(GameServerClient* Client)
{
  if (Client != NULL && WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
  {
    /* Find the room the player is in */
    GameServerRoom* Room = Client->Room;
    if (Room != NULL)
    {
      /* Notify the player that he left the room */
      Client->SendNotification(LeftRoom);

      /* Remove the player from the room */
      if (Room->WhitePlayer == Client)
        Room->WhitePlayer = NULL;
      else if (Room->BlackPlayer == Client)
        Room->BlackPlayer = NULL;
      else
        Room->Observers.remove(Client);
      Client->Room = NULL;
      Client->Ready = false;

      /* Delete the room if there are no more players in it */
      if (Room->WhitePlayer == NULL && Room->BlackPlayer == NULL && Room->Observers.size() == 0)
      {
        if (Room->Started)
          /* Notify observers */
          NotifyObservers(RoomGameEnded, Room);

        Rooms.remove(Room);
        delete Room;
      }
      else
      {
        /* Change the room's owner */
        if (Room->Owner == Client)
        {
          if (Room->WhitePlayer != NULL)
            Room->Owner = Room->WhitePlayer;
          else if (Room->BlackPlayer != NULL)
            Room->Owner = Room->BlackPlayer;
          else
            Room->Owner = *(Room->Observers.begin());
          /* Notify the player that he is the new game host */
          if (Room->Owner != NULL)
            Room->Owner->SendHostChanged(Room->Owner->Id);
        }

        /* Notify the room's players that a player left */
        if (Room->WhitePlayer != NULL)
          Room->WhitePlayer->SendPlayerLeft(Client->Id);
        if (Room->BlackPlayer != NULL)
          Room->BlackPlayer->SendPlayerLeft(Client->Id);
        list<GameServerClient*>::iterator it;
        for (it = Room->Observers.begin(); it != Room->Observers.end(); it++)
          (*it)->SendPlayerLeft(Client->Id);
      }
    }
    ReleaseMutex(Mutex);
  }
}

void GameServer::RemoveClient(GameServerClient* Client)
{
  if (Client != NULL && WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
  {
    Clients.remove(Client);
    ReleaseMutex(Mutex);
  }
}

void GameServer::SendGameData(GameServerClient* Client, unsigned char* Data, unsigned long DataSize)
{
  if (Client != NULL && WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
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
      list<GameServerClient*>::iterator it;
      for (it = Room->Observers.begin(); it != Room->Observers.end(); it++)
      {
        if (!(*it)->Synchronised)
          (*it)->SendGameData(Data,DataSize);
        (*it)->Synchronised = true;
      }
    }
    ReleaseMutex(Mutex);
  }
}

void GameServer::SendMessage(GameServerClient* Client, char* Message)
{
  if (Client != NULL && Message != NULL && WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
  {
    GameServerRoom* Room = Client->Room;
    if (Room != NULL)
    {
      /* Forward to the entire room */
      if (Room->WhitePlayer != NULL)
        Room->WhitePlayer->SendMessage(Client->Id,Message);
      if (Room->BlackPlayer != NULL)
        Room->BlackPlayer->SendMessage(Client->Id,Message);
      list<GameServerClient*>::iterator it;
      for (it = Room->Observers.begin(); it != Room->Observers.end(); it++)
        (*it)->SendMessage(Client->Id,Message);
    }
    ReleaseMutex(Mutex);
  }
}

void GameServer::SendMove(GameServerRoom* Room, unsigned long Data)
{
  if (Room != NULL && WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
  {
    /* Forward to the entire room */
    if (Room->WhitePlayer != NULL)
      Room->WhitePlayer->SendMove(Data);
    if (Room->BlackPlayer != NULL)
      Room->BlackPlayer->SendMove(Data);
    list<GameServerClient*>::iterator it;
    for (it = Room->Observers.begin(); it != Room->Observers.end(); it++)
      (*it)->SendMove(Data);
    ReleaseMutex(Mutex);
  }
}

void GameServer::SendNotification(GameServerRoom* Room, NotificationType Notification)
{
  if (Room != NULL && WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
  {
    /* Forward to the entire room */
    if (Room->WhitePlayer != NULL)
      Room->WhitePlayer->SendNotification(Notification);
    if (Room->BlackPlayer != NULL)
      Room->BlackPlayer->SendNotification(Notification);
    list<GameServerClient*>::iterator it;
    for (it = Room->Observers.begin(); it != Room->Observers.end(); it++)
      (*it)->SendNotification(Notification);
    ReleaseMutex(Mutex);
  }
}

void GameServer::SendPromotion(GameServerRoom* Room, int Type)
{
  if (Room != NULL && WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
  {
    /* Forward to the entire room */
    if (Room->WhitePlayer != NULL)
      Room->WhitePlayer->SendPromoteTo(Type);
    if (Room->BlackPlayer != NULL)
      Room->BlackPlayer->SendPromoteTo(Type);
    list<GameServerClient*>::iterator it;
    for (it = Room->Observers.begin(); it != Room->Observers.end(); it++)
      (*it)->SendPromoteTo(Type);
    ReleaseMutex(Mutex);
  }
}

void GameServer::SendRequest(GameServerClient* Client, PlayerRequestType Request)
{
  if (Client != NULL && WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
  {
    GameServerRoom* Room = Client->Room;
    if (Room != NULL)
    {
      /* Forward to the opposing player */
      if (Client == Room->WhitePlayer && Room->BlackPlayer != NULL)
        Room->BlackPlayer->SendPlayerRequest(Request);
      if (Client == Room->BlackPlayer && Room->WhitePlayer != NULL)
        Room->WhitePlayer->SendPlayerRequest(Request);
    }
    ReleaseMutex(Mutex);
  }
}

void GameServer::SendRoomList(GameServerClient* Client)
{
  if (Client != NULL && WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
  {
    list<GameServerRoom*>::iterator it;
    for (it = Rooms.begin(); it != Rooms.end(); it++)
      Client->SendRoomInfo((*it)->Id,(*it)->Name,(*it)->Private,((*it)->BlackPlayer != NULL ? 1 : 0) + ((*it)->WhitePlayer != NULL ? 1 : 0) + (*it)->Observers.size());
    ReleaseMutex(Mutex);
  }
}

void GameServer::SendTime(GameServerRoom* Room, unsigned int Id, unsigned long Time)
{
  if (Room != NULL && WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
  {
    /* Forward to the entire room */
    if (Room->WhitePlayer != NULL && Room->WhitePlayer->Id != Id)
      Room->WhitePlayer->SendTime(Id,Time);
    if (Room->BlackPlayer != NULL && Room->BlackPlayer->Id != Id)
      Room->BlackPlayer->SendTime(Id,Time);
    list<GameServerClient*>::iterator it;
    for (it = Room->Observers.begin(); it != Room->Observers.end(); it++)
      (*it)->SendTime(Id,Time);
    ReleaseMutex(Mutex);
  }
}

void GameServer::SetName(GameServerClient* Client, char* PlayerName)
{
  if (Client != NULL && PlayerName != NULL && WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
  {
    Client->Name = PlayerName;
    GameServerRoom* Room = Client->Room;
    if (Room != NULL)
    {
      /* Forward to the entire room */
      if (Room->WhitePlayer != NULL)
        Room->WhitePlayer->SendName(Client->Id, Client->Name);
      if (Room->BlackPlayer != NULL)
        Room->BlackPlayer->SendName(Client->Id, Client->Name);
      list<GameServerClient*>::iterator it;
      for (it = Room->Observers.begin(); it != Room->Observers.end(); it++)
        (*it)->SendName(Client->Id, Client->Name);
    }
    else
      Client->SendName(Client->Id, Client->Name);
    ReleaseMutex(Mutex);
  }
}

void GameServer::SetReady(GameServerClient* Client)
{
  if (Client != NULL && WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
  {
    /* Update player */
    Client->Ready = true;

    GameServerRoom* Room = Client->Room;
    if (Room != NULL)
    {
      /* Notify the room's players */
      if (Room->WhitePlayer != NULL)
        Room->WhitePlayer->SendPlayerReady(Client->Id);
      if (Room->BlackPlayer != NULL)
        Room->BlackPlayer->SendPlayerReady(Client->Id);
      list<GameServerClient*>::iterator it;
      for (it = Room->Observers.begin(); it != Room->Observers.end(); it++)
        (*it)->SendPlayerReady(Client->Id);
      if (Room->WhitePlayer != NULL && Room->WhitePlayer->Ready && Room->BlackPlayer != NULL && Room->BlackPlayer->Ready)
      {
        /* Update room players */
        Room->Started = true;
        Room->StartTimestamp = GetTickCount();
        Room->WhitePlayer->Ready = false;
        Room->BlackPlayer->Ready = false;

        /* Notify the room's players */
        Room->WhitePlayer->SendNotification(GameStarted);
        Room->BlackPlayer->SendNotification(GameStarted);
        list<GameServerClient*>::iterator it;
        for (it = Room->Observers.begin(); it != Room->Observers.end(); it++)
          (*it)->SendNotification(GameStarted);

        /* Notify observers */
        NotifyObservers(RoomGameStarted, Room);
      }
    }
    ReleaseMutex(Mutex);
  }
}

// Private functions -----------------------------------------------------------

unsigned int GameServer::Run()
{
  /* Open a socket for incoming connections */
  TCPServerSocket* Socket = new TCPServerSocket;
  Socket->SetBlocking(false);
  Socket->Open(Port);

  while (IsActive() && Socket->IsOpened())
  {
    /* Accept the next connection request */
    SOCKET SocketId = Socket->Accept();
    if (IsActive() && SocketId != INVALID_SOCKET)
    {
      if (ClientIdCounter == UINT_MAX)
        ClientIdCounter = 1;
      else
        ClientIdCounter++;
      GameServerClient* Client = new GameServerClient(this, SocketId, ClientIdCounter);
      if (WaitForSingleObject(Mutex,INFINITE) == WAIT_OBJECT_0)
      {
        Clients.push_back(Client);
        ReleaseMutex(Mutex);
      }
    }
    else
    {
      int Error = WSAGetLastError();
      if (Error != WSAEWOULDBLOCK)
        break;
    }
    Sleep(100);
  }

  /* Close the server socket */
  Socket->Close();

  /* Clean up */
  delete Socket;

  return 0;
}
