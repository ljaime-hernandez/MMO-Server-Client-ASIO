#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"

namespace netmsg
{
	namespace net
	{
		//Forward declaration of a server interface
		template<typename T>
		class server_interface;

		// The "enable shared from this" will allow us to create a pointer to this object within this object, it also allow us to 
		// make it a shared pointer rather than a raw one
		template <typename T>
		class connection : public std::enable_shared_from_this<connection<T>>
		{
		public:

			// The owner will handle the ownership requirements for both server and client depending on the connection
			enum class owner
			{
				server,
				client
			};

			// The connection will receive ownership definition, a reference of the ASIO context for connection handling, a unique 
			// socket handled by whoever is using the connection, and reference to the incoming message queue for either the client 
			// or the server interface. As the context and the incoming messages are references, the definition is imperative. The 
			// body of the connection will assign the ownership, the reason why it is not defined in the constructor header or 
			// listing is because we want to explicitly separate the critical and non critical information
			connection(owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket, tsqueue<owned_message<T>>& qIn) : m_asioContext(asioContext), m_socket(std::move(socket)), m_qMessagesIn(qIn)
			{
				m_nOwnerType = parent;

				// Construct validates which conection ownership is being created
				if (m_nOwnerType == owner::server)
				{
					// For a stronger security handshake we need to use random data, in this case we are using the chrono library 
					// which will turn the time as from now since the epoch start in jan 1, 1970 and count the time in milliseconds
					m_nHandshakeOut = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());

					// Pre-calculates the result for the handshake check when the client connects
					m_nHandshakeCheck = scramble(m_nHandshakeOut);

				}
				else
				{
					// If its a client connection, there is nothing to define as for now
					m_nHandshakeIn = 0;
					m_nHandshakeOut = 0;
				}
			};

			virtual ~connection()
			{
			};

			// ID getter
			uint32_t GetID() const
			{
				return id;
			}

		public:

			// This function will assign an id to the connection, the implementation on the server class is designed so the ID 
			// number changes every time a connection is detected 
			void ConnectToClient(netmsg::net::server_interface<T>* server, uint32_t uid = 0)
			{
				if (m_nOwnerType == owner::server)
				{
					if (m_socket.is_open())
					{
						id = uid;	
						//Used in previous version
						//ReadHeader();

						// When the client attempts to connect to the server we will make it go through some validation, the 
						// handshake data must be written out before the client can have a direct valid interaction with the server
						WriteValidation();

						// Afterwards we will issue a task for the server to wait asynchronously for the client to send the 
						// validated data back to us
						ReadValidation(server);
					}
				}
			}
			// Only called by clients
			void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
			{
				// This function can only be used by clients, which is checked right at the beginning
				if (m_nOwnerType == owner::client)
				{
					// Makes ASIO a request to connect to endpoints, then the ASIO context is primed waiting for messages from the server
					asio::async_connect(m_socket, endpoints,
						[this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
						{
							if (!ec)
							{
								// Used in previous version
								// ReadHeader();

								ReadValidation();
							}
						});
				}
			}
			// Called by both client and server
			void Disconnect()
			{
				// We can explicitly close the socket if its appropriate for ASIO to do so
				if (IsConnected())
				{
					asio::post(m_asioContext,
						[this]()
						{
							m_socket.close();
						});
				}
			}

			bool IsConnected() const
			{
				return m_socket.is_open();
			}

		public:
			void Send(const message<T>& msg)
			{
				// The context is already waiting for incoming messages, but we will use the post function on it for it to 
				// asynchronously check on the messages content, as the process is working randomly when the client or the 
				// server are interacting, then we need to previously check on the message queue even before its being written, 
				// a simple boolean will allow us to check on the content and prime the context into writing messages if needed
				asio::post(m_asioContext,
					[this, msg]()
					{
						bool bWritingMessage = !m_qMessagesOut.empty();
						m_qMessagesOut.push_back(msg);

						if (!bWritingMessage)
						{
							WriteHeader();
						}
					});
			}

		private:

			// Asynchronous task which will prime the context to read a message header
			void ReadHeader()
			{
				// For the asynchronous read we use the clients/server socket, we call the ASIO buffer which will require a 
				// size which was prestablished on the message header declaration, it also requires a space in memory to store 
				// the temporary data, so this connection type has declared a message type for temporal information
				asio::async_read(m_socket, asio::buffer(&m_msgTemporaryIn.header, sizeof(message_header<T>)),
					// The lambda function declared is used to provide the work to do when the function is called
					[this](std::error_code ec, std::size_t length)
					{
						
						if (!ec)
						{
							if (m_msgTemporaryIn.header.size > 0)
							{
								// We instantiated the message size to change if it contains information, in this case 
								// the temporary message type will change its size by checking the incoming message size
								m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size);
								// The readBody function will asynchronously prime the connection to read the body of the 
								// message, which is declared down below
								ReadBody();
							}
							else
							{
								// At this point, the body of the messages might be full, so we move the process into a queue
								AddToIncomingMessageQueue();
							}
						}
						else
						{
							// If theres errors in the connection, we will manually close the socket in the code, which will be 
							// later identified by the messageClient function declared previously, the function is declared to 
							// tidy up the deque of connections
							std::cout << "[" << id << "] Read Header Fail\n";
							m_socket.close();
						}
					});
			}

			// Asynchronous task which will prime the context to read a message body
			void ReadBody()
			{
				// The asynchronous function is called after the header is confirmed to contain information, then the ASIO context will 
				// allow us to read the body data by using the temporal assigned message
				asio::async_read(m_socket, asio::buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							AddToIncomingMessageQueue();
						}
						else
						{
							// If theres errors in the connection, we will manually close the socket in the code, which will be later 
							// identified by the messageClient function declared previously, the function is declared to tidy up the 
							// deque of connections
							std::cout << "[" << id << "] Read Body Fail\n";
							m_socket.close();
						}
					});
			}

			// Asynchronous task which will prime the context to write a message header
			void WriteHeader()
			{
				// This asynchronous function will sit and wait for messages when written, it will use the socket as a parameter, 
				// it will take the messages queue output in order y using the header, and finally it will help itself by checking 
				// on the size of the message header previously declared.
				asio::async_write(m_socket, asio::buffer(&m_qMessagesOut.front().header, sizeof(message_header<T>)),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							if (m_qMessagesOut.front().body.size() > 0)
							{
								// If the body vector of the message contains anything, then it will prime the context into reading 
								// whatever information is in it, 
								WriteBody();
							}
							else
							{
								// if the message is done with the body, then it will get rid of it, but it will also call itself 
								// again in case theres more messages in the queue
								m_qMessagesOut.pop_front();
								if (!m_qMessagesOut.empty())
								{
									WriteHeader();
								}
							}
						}
						else
						{
							// If theres errors in the connection, we will manually close the socket in the code, which will be later 
							// identified by the messageClient function declared previously, the function is declared to tidy up the 
							// deque of connections
							std::cout << "[" << id << "] Write Header Fail\n";
							m_socket.close();
						};
					});
			};

			// Asynchronous task which will prime the context to write a message body
			void WriteBody()
			{
				asio::async_write(m_socket, asio::buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().body.size()),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							m_qMessagesOut.pop_front();

							if (!m_qMessagesOut.empty())
							{
								WriteHeader();
							};
						}
						else
						{
							// If theres errors in the connection, we will manually close the socket in the code, which will be later 
							// identified by the messageClient function declared previously, the function is declared to tidy up the 
							// deque of connections
							std::cout << "[" << id << "] Write Body Fail\n";
							m_socket.close();
						}
					});
			}

			void AddToIncomingMessageQueue()
			{
				// If the owner of the connection is the server, then we will move the message into the tsqueue and push it into it by 
				// using the shared pointer of the connection itself and the temporal message
				if (m_nOwnerType == owner::server)
				{
					m_qMessagesIn.push_back({ this->shared_from_this(), m_msgTemporaryIn });
				}
				else
				{
					// If the owner of the connection is a client then we will use a null pointer to reassure that the client has just one connection
					m_qMessagesIn.push_back({ nullptr, m_msgTemporaryIn });
				}
				// When the message is finished on reading, we need to prime the context again into reading the next header available
				ReadHeader();
			}

			// Data encryption for client/server communication, specific result and specific answer is given between the users 
			// communication, this will avoid overloading data in the server processor and will avoid port sniffers from having free access
			uint64_t scramble(uint64_t nInput)
			{
				uint64_t out = nInput ^ 0xDEADBEEFC0DECAFE;
				out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0xF0F0F0F0F0F0F0) << 4;
				return out ^ 0xC0DEFACE12345678;
			};

			// Asynchronous function used by both client and server to write packets for the validation process
			void WriteValidation()
			{
				asio::async_write(m_socket, asio::buffer(&m_nHandshakeOut, sizeof(uint64_t)),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							// When the validation is sent by the client, all they can do is to sit and wait for the servers 
							// response and proper validation
							if (m_nOwnerType == owner::client)
							{
								ReadHeader();
							};
						};
						else
						{
							m_socket.close();
						};
					});
			};

			// This function receives a pointer to a server class which will inform the user or the derived class that derivate the 
			// server that the client has been validated
			void ReadValidation(netmsg::net::server_interface<T>* server = nullptr)
			{
				asio::async_read(m_socket, asio::buffer(&m_nHandshakeIn, sizeof(uint64_t)),
					[this, server](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							if (m_nOwnerType == owner::server)
							{
								if (m_nHandshakeIn == m_nHandshakeCheck)
								{
									// If the client successfully solves the algorithm then the server will properly use the server 
									// pointer to allow the connection
									std::cout << "Client Validated" << std::endl;
									server->OnClientValidated(this->shared_from_this());
									ReadHeader();
								}
								else
								{
									std::cout << "Client Disconnected (Validation Failed)" << std::endl;
									m_socket.close();
								}
							}
							else
							{
								// If the connection is a client, then it has to solve the algorithm before the server accepts the interaction
								m_nHandshakeOut = scramble(m_nHandshakeIn);

								WriteValidation();
							}
						}
						else
						{
							std::cout << "Client Disconnected (Read Validation)" << std::endl;
							m_socket.close();
						};
					});
			};

		protected:
			// Each connection will have a unique socket
			asio::ip::tcp::socket m_socket;
			// Theres going to be a single context which will be shared with the whole ASIO instance
			asio::io_context& m_asioContext;
			// This thread-safe queue will contain all the messages to be sent to the remote side of this connection
			tsqueue<message<T>> m_qMessagesOut;
			// This thread-safe queue will contain all messages that has been received from the remote side of the connection. This has a 
			// reference as the owner of this connection is expected to provide a queue
			tsqueue<owned_message<T>>& m_qMessagesIn;

			message<T> m_msgTemporaryIn;
			// The owner will decide how the connection will behave
			owner m_nOwnerType = owner::server;
			// A variable which will allocate client identifiers
			uint32_t id = 0;

			// Variables for the handshake information
			uint64_t m_nHandshakeOut = 0;
			uint64_t m_nHandshakeIn = 0;
			uint64_t m_nHandshakeCheck = 0;
		};
	};
};

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