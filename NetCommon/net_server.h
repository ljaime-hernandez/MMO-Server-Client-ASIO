#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"
#include "net_connection.h"

namespace netmsg
{
	namespace net
	{
		template <typename T>
		class server_interface
		{
		public:
			// The server acceptor is associated with the context, the endpoint will be the address on which the server will 
			// listen to connections, in this instance we are using a version 4 for the IP address
			server_interface(uint16_t port) : m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
			{

			}

			virtual ~server_interface()
			{
				Stop();
			}

			bool Start()
			{
				try
				{
					// As stated before, the ASIO context requires some work to do for the process to not stop, so this is the 
					// first step towad using it
					WaitForClientConnection();
					// We are assigning the context is own thread so it can run while othr processes are being done
					m_threadContext = std::thread([this]()
						{
							m_asioContext.run();
						});
				}
				catch (std::exception& e)
				{
					// Catches errors if something is not letting the server to listen to clients.
					std::cerr << "[SERVER] Exception: " << e.what() << "\n";
					return false;
				}

				std::cout << "[SERVER] Started!\n";
				return true;
			}

			void Stop()
			{
				// it will attempt to stop the context
				m_asioContext.stop();
				// We ensure that the context and its thread are stopped, as the context sometimes takes some more time to stop than its thread
				if (m_threadContext.joinable())
				{
					m_threadContext.join();
				}

				std::cout << "[SERVER] Stopped!\n";
			}

			// Instructs ASIO to wait for connections
			void WaitForClientConnection()
			{
				// Asynchronous function which will allow the server to accept connections, the function receives an ASIO error type which 
				// can be used to catch any errors on the connections, it will also receive a socket for the client ip to be used to stablish 
				// the connection.
				m_asioAcceptor.async_accept(
					// lambda function
					[this](std::error_code ec, asio::ip::tcp::socket socket)
					{
						if (!ec)
						{
							// If theres no errors, we will call the socket end point to retrieve its ip address
							std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";
							// Make shared is used to allocate the object, we will also tell the connection that it will be owned by the 
							// server, the connection constructor will also use the context, we move the socket used for the connection and 
							// we pass by reference the queue of incoming messages
							
							std::shared_ptr<connection<T>> newconn = std::make_shared<connection<T>>(connection<T>::owner::server, m_asioContext, std::move(socket), m_qMessagesIn);

							// gives the user server a chance to deny connection
							if (OnClientConnect(newconn))
							{
								// If the connection is allowed, we add it to the container of new connections
								m_deqConnections.push_back(std::move(newconn));
								// We will allocate an id for that new connection
								m_deqConnections.back()->ConnectToClient(this, nIDCounter++);
								std::cout << "[" << m_deqConnections.back()->GetID() << "] Connections Approved\n";

							}
							else
							{
								std::cout << "[-----] Connection Denied\n";
							}
						}
						else
						{
							std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
						}
						// Prime the ASIO context again with more work, giving an asynchronous task whch will simply wait for another connection
						WaitForClientConnection();
					});
			}

			// Sends message to an specific client, the server will store the clients connection as a shared pointer
			void MessageClient(std::shared_ptr<connection<T>> client, const message<T>& msg)
			{
				// We check if the shared pointer is valid at first
				if (client && client->IsConnected())
				{
					client->Send(msg);
				}
				else
				{
					// One of the problems on the tcp protocol is that we dont receive a message when the used disconnects, so we have to 
					// manipulate the client to check if its still communicating or not
					OnClientDisconnect(client);
					// We will delete the client
					client.reset();
					// We will delete the client from our deque of connections
					m_deqConnections.erase(
						std::remove(m_deqConnections.begin(), m_deqConnections.end(), client), m_deqConnections.end());
				}
			}


			void MessageAllClients(const message<T>& msg, std::shared_ptr<connection<T>> pIgnoreClient = nullptr)
			{
				bool bInvalidClientExists = false;
				// We will iterate through all clients to check which one is connected or not, we set a null pointer as default for the 
				// program to send messages only to those clients which are connected.
				for (auto& client : m_deqConnections)
				{
					if (client && client->IsConnected())
					{
						if (client != pIgnoreClient)
						{
							client->Send(msg);
						}
					}
					else
					{
						// this approach will not run the eraser for the deque connections inmediately but will just set a boolean as true 
						// to run the eraser only if there is disconnected clients. this will save some memory and allow the program to 
						// loose a bit of its weight
						OnClientDisconnect(client);
						client.reset();
						bInvalidClientExists = true;
					}
				}

				if (bInvalidClientExists)
				{
					m_deqConnections.erase(
						std::remove(m_deqConnections.begin(), m_deqConnections.end(), nullptr), m_deqConnections.end());
				}
			}

			// This function will be called by clients, it will decide which is the appropriate time to send messages into the queue. 
			// The function will constrain the number of messages the user will send in one go
			void Update(size_t nMaxMessages = -1, bool bWait = false)
			{
				// The wait function will put the server to "sleep" until it gets an interaction message from either a client or another 
				// server, it will save processing work on the cpu
				if (bWait)
				{
					m_qMessagesIn.wait();
				}
				size_t nMessageCount = 0;
				// Funtion will check if there are messages in the queue
				while (nMessageCount < nMaxMessages && !m_qMessagesIn.empty())
				{
					// If there is, it will pop it in the front of the queue
					auto msg = m_qMessagesIn.pop_front();
					// Pass the message to the message handler, as a reminder, the messages are shared pointers
					OnMessage(msg.remote, msg.msg);
					nMessageCount++;
				}
			}

		protected:
			// Function called when a client connects
			virtual bool OnClientConnect(std::shared_ptr<connection<T>> client)
			{
				return false;
			}

			// Called when a client seems to be disconnected
			virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client)
			{

			}

			// Called when a message arrives
			virtual void OnMessage(std::shared_ptr <connection<T>> client, message<T>& msg)
			{

			}

		public:
			virtual void OnClientValidated(std::shared_ptr<connection<T>> client)
			{

			}

		protected:
			// Thread-safe queue for incoming message packets
			tsqueue<owned_message<T>> m_qMessagesIn;

			std::deque<std::shared_ptr<connection<T>>> m_deqConnections;
			// ASIO context which will be shared across all of the connected clients
			asio::io_context m_asioContext;
			// Context requires its own thread
			std::thread m_threadContext;
			// The acceptor will be the tool we will use to get the client sockets
			asio::ip::tcp::acceptor m_asioAcceptor;
			// ID number will change and be delivered to each client connected on the server, this approach its a simplified version 
			// for identifiers, its also secure as the id confirmation is not some private information used by our system (like an IP 
			// address) which will be returned to the client or possibly other clients, it will be just a number provided by the server
			uint32_t nIDCounter = 10000;
		};
	}
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