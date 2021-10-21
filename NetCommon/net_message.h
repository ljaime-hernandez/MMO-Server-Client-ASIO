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

#pragma once
#include "net_common.h"

namespace netmsg
{
	namespace net
	{

		// as stated in the readme file, the message header will have an identifier of type template, it will also contain a variable of 32 bits unsigned which will send the size of the message as a whole
		template <typename T>
		struct message_header
		{
			T id{};
			uint32_t size = 0;
		};

		// as the header is a template then we must declare the message as a template itself. As stated in the readme document, this does not have to be that way, the id can be changed to an enum class or to an integer but the template will provide more versatility on the code to receive and send any kind of requests
		template <typename T>
		struct message
		{
			message_header<T> header{};
			// we use standard bevtors for the body as it is good handling arrays of data and the size will make the vector flexible and not waste memory
			std::vector<uint8_t> body;

			// this function will return the required size for the message by retrieving the sum of the message header in bytes and the body vector
			size_t size() const
			{
				return body.size();
			}

			// this override for the ostream will allow both server and user to see the description of the message, which in this case includes the id and the size of the message (which include the header as stated before). The id is going to be casted into an integer as the template must be defined before it is shown on the screen, this wont be needed on the header size as it is already an unsigned integer
			friend std::ostream& operator << (std::ostream& os, const message<T>& msg)
			{
				os << "ID: " << int(msg.header.id) << " Size: " << msg.header.size;
				return os;
			}

			// function used to push the body data into the message buffer, the template is used in case the user is sending data in any kind of type, as it may vary and we dont want to make functions specifying each kind of data received
			template<typename DataType>
			friend message<T>& operator << (message<T>& msg, const DataType& data)
			{
				// we check if the type provided is standar for our code to read it, the static_assert is modern c++ but in future we might receive a datatype that we dont know, so an error message is displayed in case its not readable
				static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed");

				// current size of body vector, as this will be the start point from where we will insert the additional data
				size_t i = msg.body.size();

				// we resize the body vector by the size of the data being pushed, previous body size + new size for the receiving datatype
				msg.body.resize(msg.body.size() + sizeof(DataType));

				// we then copy the data received into the end of the body vector, this on the space we created previously
				std::memcpy(msg.body.data() + i, &data, sizeof(DataType));

				// we will finally call the size function to update again the new size
				msg.header.size = msg.size();

				//returns the target message so it can be chained
				return msg;
			}

			// operator overloaded for extracting the data from the body vector
			template<typename DataType>
			friend message<T>& operator >> (message<T>& msg, DataType& data)
			{
				// again we check if the data from the body vector is trivial
					static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pulled");
				
				// we cache location towards the end of the vector where the pulled data starts
					size_t i = msg.body.size() - sizeof(DataType);

				// physically copy the data from the vector into the user variable
					std::memcpy(&data, msg.body.data() + i, sizeof(DataType));

				// resize the vector after the byte reading, then reset the end position of it
				msg.body.resize(i);

				// we will finally call the size function to update again the new size
				msg.header.size = msg.size();

				//returns the target message so it can be chained
				return msg;
			}
		};

		template <typename T>
		class connection;

		// encapsulates the regular message but it also has a shared pointer for the connection object
		template <typename T>
		struct owned_message
		{
			std::shared_ptr<connection<T>> remote = nullptr;
			message<T> msg;

			// overloaded the << operator for the output to work on this object
			friend std::ostream& operator<<(std::ostream& os, const owned_message<T>& msg)
			{
				os << msg.msg;
				return os;
			}
		};
	}
}
