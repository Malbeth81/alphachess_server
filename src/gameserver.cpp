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

// Public functions ------------------------------------------------------------

GameServer::GameServer()
{
  ClientIdCounter = 0;
  RoomIdCounter = 0;
  Resume();
}

GameServer::~GameServer()
{
  /* Clean up */
  while (Clients.Size() > 0)
    delete Clients.Remove();
  while (Rooms.Size() > 0)
    delete Rooms.Remove();
}

GameServerRoom* GameServer::CreateRoom(GameServerClient* Client, string Name)
{
  if (Client != NULL)
  {
    if (RoomIdCounter == UINT_MAX)
      RoomIdCounter = 1;
    else
      RoomIdCounter++;

    /* Create a new room */
    GameServerRoom* Room = new GameServerRoom;
    Room->Id = RoomIdCounter;
    Room->Locked = false;
    Room->Paused = false;
    Room->Name = Name;
    Room->Owner = Client;
    Room->BlackPlayer = NULL;
    Room->WhitePlayer = NULL;

    /* Add to the list */
    Rooms.Add(Room);

    return Room;
  }
  return NULL;
}

GameServerRoom* GameServer::FindRoom(unsigned int Id)
{
  GameServerRoom* Room = Rooms.GetFirst();
  while (Room != NULL && Room->Id != Id)
    Room = Rooms.GetNext();
  return Room;
}

const LinkedList<GameServerClient> GameServer::GetClients()
{
  return Clients;
}

const LinkedList<GameServerRoom> GameServer::GetRooms()
{
  return Rooms;
}

void GameServer::JoinRoom(GameServerClient* Client, GameServerRoom* Room)
{
  if (Client != NULL && Room != NULL)
  {
    /* Leave the current room, if any */
    LeaveRoom(Client);

    /* Notify the player that he his joining the room */
    Client->SendNotification(JoinedRoom);

    /* Notify the room's players that a player is joining */
    if (Room->WhitePlayer != NULL)
      Room->WhitePlayer->SendPlayerJoined(Client->Id, Client->Name);
    if (Room->BlackPlayer != NULL)
      Room->WhitePlayer->SendPlayerJoined(Client->Id, Client->Name);
    GameServerClient* Ptr = Room->Observers.GetFirst();
    while (Ptr != NULL)
    {
      Ptr->SendPlayerJoined(Client->Id, Client->Name);
      Ptr = Room->Observers.GetNext();
    }

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
    Ptr = Room->Observers.GetFirst();
    while (Ptr != NULL)
    {
      Client->SendPlayerJoined(Ptr->Id, Ptr->Name);
      Ptr = Room->Observers.GetNext();
    }

    /* Add the player to the room */
    Room->Observers.Add(Client);
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
  }
}

void GameServer::LeaveRoom(GameServerClient* Client)
{
  if (Client != NULL)
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
        Room->Observers.Remove(Client);
      Client->Room = NULL;
      Client->Ready = false;

      /* Delete the room if there are no more players in it */
      if (Room->WhitePlayer == NULL && Room->BlackPlayer == NULL && Room->Observers.Size() == 0)
      {
        Rooms.Remove(Room);
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
            Room->Owner = Room->Observers.GetFirst();
          /* Notify the player that he is the new game host */
          Room->Owner->SendHostChanged(Room->Owner->Id);
        }

        /* Notify the room's players that a player left */
        if (Room->WhitePlayer != NULL)
          Room->WhitePlayer->SendPlayerLeft(Client->Id);
        if (Room->BlackPlayer != NULL)
          Room->BlackPlayer->SendPlayerLeft(Client->Id);
        GameServerClient* Ptr = Room->Observers.GetFirst();
        while (Ptr != NULL)
        {
          Ptr->SendPlayerLeft(Client->Id);
          Ptr = Room->Observers.GetNext();
        }
      }
    }
  }
}

void GameServer::RemoveClient(GameServerClient* Client)
{
  if (Client != NULL)
    Clients.Remove(Client);
}


void GameServer::SendRoomList(GameServerClient* Client)
{
  if (Client != NULL)
  {
    GameServerRoom* Ptr = Rooms.GetFirst();
    while (Ptr != NULL)
    {
      Client->SendRoomInfo(Ptr->Id,Ptr->Name,Ptr->Locked,(Ptr->BlackPlayer != NULL ? 1 : 0) + (Ptr->WhitePlayer != NULL ? 1 : 0) + Ptr->Observers.Size());
      Ptr = Rooms.GetNext();
    }
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
      Clients.Add(new GameServerClient(this, SocketId, ClientIdCounter));
    }
    else
    {
      int Error = WSAGetLastError();
      if (Error != WSAEWOULDBLOCK && Error != WSAETIMEDOUT)
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
