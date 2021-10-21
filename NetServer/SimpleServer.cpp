/*
	MMO Client/Server Framework using ASIO

	Copyright 2018 - 2020 OneLoneCoder.com
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:
	1. Redistributions or derivations of source code must retain the above
	copyright notice, this list of conditions and the following disclaimer.
	2. Redistributions or derivative works in binary form must reproduce
	the above copyright notice. This list of conditions and the following
	disclaimer must be reproduced in the documentation and/or other
	materials provided with the distribution.
	3. Neither the name of the copyright holder nor the names of its
	contributors may be used to endorse or promote products derived
	from this software without specific prior written permission.
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
	HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	Author
	~~~~~~
	David Barr, aka javidx9, ©OneLoneCoder 2019, 2020

*/

#include <iostream>
#include <msg_net.h>

// definition of my custom message types
enum class CustomMsgTypes : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage,
};

// definition of custom server class which inherits from our custom server interface specialized on the previous custom message types
class CustomServer : public netmsg::net::server_interface<CustomMsgTypes>
{
public:
	// the constructor of this class takes in a port number and constructs the server interface 
	CustomServer(uint16_t nPort) : netmsg::net::server_interface<CustomMsgTypes>(nPort)
	{

	}

protected:
	// send a message to the recently connected client using the ServerAccept custom message
	bool OnClientConnect(std::shared_ptr<netmsg::net::connection<CustomMsgTypes>> client)
	{
		// netmsg::net::message<CustomMsgTypes> msg;
		// msg.header.id = CustomMsgTypes::ServerAccept;
		// client->Send(msg);
		return true;
	}

	void OnClientDisconnect(std::shared_ptr<netmsg::net::connection<CustomMsgTypes>> client)
	{
		std::cout << "Removing client [" << client->GetID() << "]\n";
	}

	// checks for client messages, based on Custom types added to the message id, the client display will show specific procedures and this function wil confirm the server interaction with the client by sending back a message and printing the procedure name in the server console
	void OnMessage(std::shared_ptr<netmsg::net::connection<CustomMsgTypes>> client, netmsg::net::message<CustomMsgTypes>& msg)
	{
		switch (msg.header.id)
		{
		case CustomMsgTypes::ServerPing:
		{
			std::cout << "[" << client->GetID() << "] Server Ping\n";
			client->Send(msg);
		}
		break;
		case CustomMsgTypes::MessageAll:
		{
			// message all trigger confirmation
			std::cout << "[" << client->GetID() << "]: Message All\n";

			netmsg::net::message<CustomMsgTypes> msg;
			msg.header.id = CustomMsgTypes::ServerMessage;
			msg << client->GetID();
			// parameter on this function is designed so it ignores the client which sent the message, avoids message duplication
			MessageAllClients(msg, client);
		}
		break;
		}
	}

	void OnClientValidated(std::shared_ptr<netmsg::net::connection<CustomMsgTypes>> client) override
	{
		// Client passed validation check, so send them a message informing
		// them they can continue to communicate
		netmsg::net::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::ServerAccept;
		client->Send(msg);
	}
};

int main()
{
	CustomServer server(60000);
	server.Start();

	while (1)
	{
		server.Update(-1, true);
	}

	return 0;
}