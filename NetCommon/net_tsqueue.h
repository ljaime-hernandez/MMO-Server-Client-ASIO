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

		// thread-safe queue
		template<typename T>
		class tsqueue
		{
		public:
			tsqueue() = default;
			tsqueue(const tsqueue<T>&) = delete;
			virtual ~tsqueue()
			{
				clear();
			}

		public:
			const T& front()
			{
				std::scoped_lock lock(muxQueue);
				return deqQueue.front();
			}

			const T& back()
			{
				std::scoped_lock lock(muxQueue);
				return deqQueue.back();
			}

			void push_back(const T& item)
			{
				std::scoped_lock lock(muxQueue);
				deqQueue.emplace_back(std::move(item));

				// the notify_one function will send a wake up signal for the server to process the incoming information, this is done in both push front or push back tsqueue as this will contain the message information
				std::unique_lock<std::mutex> ul(muxBlocking);
				cvBlocking.notify_one();
			}

			void push_front(const T& item)
			{
				std::scoped_lock lock(muxQueue);
				deqQueue.emplace_front(std::move(item));

				// the notify_one function will send a wake up signal for the server to process the incoming information, this is done in both push front or push back tsqueue as this will contain the message information
				std::unique_lock<std::mutex> ul(muxBlocking);
				cvBlocking.notify_one();
			}

			bool empty()
			{
				std::scoped_lock lock(muxQueue);
				return deqQueue.empty();
			}

			size_t count()
			{
				std::scoped_lock lock(muxQueue);
				return deqQueue.size();
			}

			void clear()
			{
				std::scoped_lock lock(muxQueue);
				deqQueue.clear();
			}

			T pop_front()
			{
				std::scoped_lock lock(muxQueue);
				auto t = std::move(deqQueue.front());
				deqQueue.pop_front();
				return t;
			}

			T pop_back()
			{
				std::scoped_lock lock(muxQueue);
				auto t = std::move(deqQueue.back());
				deqQueue.pop_back();
				return t;
			}

			// the server will be locked in this funtion, waiting for any interaction to make it work again. on Windows processors, the threads sometimes are erroneously utilized, this is inconvenient for servers that are just waiting for something to happen, but this code is designed so if that would happen. then it would just simply turn back into this loop
			void wait()
			{
				while (empty())
				{
					std::unique_lock<std::mutex> ul(muxBlocking);
					// sends the thread to sleep until a signal is received
					cvBlocking.wait(ul);
				}
			}

		protected:
			// mutex will protect the double ended queue by locking the process on course until the function is finished, this will be done for each function on this class
			std::mutex muxQueue;
			std::deque<T> deqQueue;
			//
			std::condition_variable cvBlocking;
			std::mutex muxBlocking;
		};
	}
}