# Masive Multiplayer Online Server-Client using ASIO library



In this code, we can see how ASIO handles the temporal properties of network communication
by helping us execute the code when is necessary to do so. one of the problems encountered
on this HTTP example is we do not know in advance how much data is going to be sent, which is
pretty inconvenient with services such as streaming. The solution for this issue will be
handled on this code, we will handle those network request via messages, which
will be composed of: 

- a header, the header will have and identifier of what the message is
- the size,  of the entire message including the header request in bytes 
- a body,  which will be composed of zero or more bytes. 

![message](images/message.png)

The header will always be of a fixed size and is sent first, and its so it primes the system to know 
how much data memory must be opened for the body part of the message to be received.

As the messages received by an user can be of any type, the code must be flexible, the identifier
on the header is the one meant to know which kind of message we will receive, but this can 
be one of thousand different types of request, we could declare an enum class to classify the
request and make it more strict for the system to decide what to do with each of the requests,
but doing so limits the amount of messages we will receive as we would have to declare all kind
of messages we do not even know about now, so this solution will include the use of templates,
which will make the code more versatile when receiving the data

![enum](images/enum.png)


**ASIO library can be downloaded from https://think-async.com/Asio/**

**## DISCLAIMER:**

 The original dependencies and server architecture were taken as reference from David Barr, aka javidx9
 the comments on the code were all written by me, Luis Miguel Jaime Hernandez, by taking his videos
 and instructions as reference to learn some C++ coding, please refer to OneLoneCoder for additional
 information about the olcPixelGameEngine functionality.

```diff

 License (OLC-3)
	~~~~~~~~~~~~~~~
	Copyright 2018 - 2022 OneLoneCoder.com
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

	David Barr, aka javidx9, ©OneLoneCoder 2019, 2020, 2021

```