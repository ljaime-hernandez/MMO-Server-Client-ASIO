#pragma once
#include "net_common.h"

namespace netmsg
{
	namespace net
	{
		template<typename T>
		class client_interface
		{
		public:
			// Constructor will just associate the socket with the ASIO context
			client_interface()
			{

			}

			// When the client connection is destroyed, there should be a proper method to disconnect it from the server
			virtual ~client_interface()
			{
				Disconnect();
			}

			// Connects to server with the hostname/ip and port number
			bool Connect(const std::string& host, const uint16_t port)
			{
				try
				{
					// Resolves the hostname/ip into tangible physical address, the resolver can take an URL, ip address or 
					// string and uses it to convert the data into the connection
					asio::ip::tcp::resolver resolver(m_context);
					// The resolver will be used to create the endpoints of the client connection which will use the host 
					// physical address and will also use the port number used to connect to it
					asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

					// Creates the connection, in this instance we know that the connection is a client, the pointer is 
					// unique in this context, so as previously stated on the server declaration, it would not be able to have 
					// more than one pointer to the incoming and outgoing queues.
					m_connection = std::make_unique<connection<T>>(
						connection<T>::owner::client,
						m_context,
						asio::ip::tcp::socket(m_context),
						m_qMessagesIn);

					// If the endpoint connection works, the connection object will proceed to connect using the endpoint
					m_connection->ConnectToServer(endpoints);
					// Thread used for the ASIO context to work
					thrContext = std::thread([this]()
						{
							m_context.run();
						});
				}
				// ASIO comes with a wide setup for different error which will be useful in any kind of scenario as they are prety self explanatory
				catch (std::exception& e)
				{
					std::cerr << "Client Exception: " << e.what() << "/n";
					return false;
				}
				return true;
			}

			// Disconnects the client from the server
			void Disconnect()
			{
				if (IsConnected())
				{
					m_connection->Disconnect();
				}

				// The stop method is used on the context to properly shut it down
				m_context.stop();
				// Same for the thread
				if (thrContext.joinable())
				{
					thrContext.join();
				}
				// Destroys the connection object
				m_connection.release();
			}

			// Function will check if the client is connected to a server
			bool IsConnected()
			{
				if (m_connection)
				{
					return m_connection->IsConnected();
				}
				else
				{
					return false;
				}
			}

			void Send(const message<T>& msg)
			{
				if (IsConnected())
				{
					m_connection->Send(msg);
				}
			}

			// The client application will need access to the queue so we make a function to make it public
			tsqueue<owned_message<T>>& Incoming()
			{
				return m_qMessagesIn;
			}

		protected:
			// The client will own the ASIO context
			asio::io_context m_context;
			// The client ASIO context requires a thread to work on its own to execute its work commands
			std::thread thrContext;
			/*// client socket which will be connected to the server
			asio::ip::tcp::socket m_socket;*/
			// Single instance for the connection abject which will handle the dta transfer
			std::unique_ptr<connection<T>> m_connection;

		private:
			// Thread-safe queue of incoming messages from the server
			tsqueue<owned_message<T>> m_qMessagesIn;
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
