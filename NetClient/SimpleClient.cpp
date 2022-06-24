#include <iostream>
#include <msg_net.h>

enum class CustomMsgTypes : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage,
}; 

class CustomClient : public netmsg::net::client_interface<CustomMsgTypes>
{
public:
	void PingServer()
	{
		netmsg::net::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::ServerPing;

		// grabs a time point from the chrono library, then we will send this time to the server, it will be then be received and the latency will be registered in an additional process on the server
		std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
		// time added to the message
		msg << timeNow;
		Send(msg);
	}

	void MessageAll()
	{
		netmsg::net::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::MessageAll;
		Send(msg);
	}
};

int main()
{
	CustomClient c;
	c.Connect("127.0.0.1", 60000);

	// windows specific approach to use asynchronous key states checked constantly on the loop for  the client to trigger functions which will interact with the server or even to other clients too
	bool key[3] = { false, false, false };
	bool old_key[3] = { false, false, false };

	bool bQuit = false;
	while (!bQuit)
	{
		if (GetForegroundWindow() == GetConsoleWindow())
		{
			key[0] = GetAsyncKeyState('1') & 0x8000;
			key[1] = GetAsyncKeyState('2') & 0x8000;
			key[2] = GetAsyncKeyState('3') & 0x8000;
		}

		// user input
		if (key[0] && !old_key[0]) c.PingServer();
		if (key[1] && !old_key[1]) c.MessageAll();
		if (key[2] && !old_key[2]) bQuit = true;

		for (int i = 0; i < 3; i++) old_key[i] = key[i];

		if (c.IsConnected())
		{
			// checks if the client has messages to be read
			if (!c.Incoming().empty())
			{
				// pops the message from the client message queue
				auto msg = c.Incoming().pop_front().msg;

				switch (msg.header.id)
				{
				// message validating the connection with the server
				case CustomMsgTypes::ServerAccept:
				{
					std::cout << "Server Accepted Connection\n";
				}
				break;

				case CustomMsgTypes::ServerPing:
				{
					// using the PingServer function, it substracts the time registered on the clients side and creates a new one to measure the latency between server/client, casted on a double primitive type for readbility
					std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
					std::chrono::system_clock::time_point timeThen;
					msg >> timeThen;
					std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n";
				}
				break;

				case CustomMsgTypes::ServerMessage:
				{
					// message sent to the server for it to be distributed with the rest of the clients connected to the same server
					uint32_t clientID;
					msg >> clientID;
					std::cout << "Hello from [" << clientID << "]\n";
				}
				break;
				}
			}
		}
		else
		{
			std::cout << "Server Down\n";
			bQuit = true;
		}
	}
	return 0;
}

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
	David Barr, aka javidx9, �OneLoneCoder 2019, 2020

*/