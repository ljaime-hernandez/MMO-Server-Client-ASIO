in this code, we can see how asio handles the temporal properties of network communication
by helping us execute the code when is neccessary to do so. one of the problems encountered
on this http example is we dont know in advance how much data is going to be sent, which is
pretty inconvenient with services such as streaming. The solution for this issue will be
handled on this code, we will handle those network request via "messages", which
will be composed of a header, the header will have and identifier of what the message is,
header will also contain the size of the entire message including the header request in bytes. 
the message will also contain a body which will be composed of zero or more bytes. the header will
always be of a fixed size and is sent first, and its so it primes the system to know how much
data memory must be opened for the body part of the message to be received.

as the messages received by an user can be of any time, the code must be flexible, the identifier
on the header is the one meant to know which kind of message we will receive, but this can 
be one of thousand different types of request, we could declare an enum class to clasify the
request and make it more strict for the system to decide what to do with each of the requests,
but doing so limits the amount of messages we will receive as we would have to declare all kind
of messages we dont even know about now, so this solution will include the use of templates,
which will make the code more versatile when receiving the data

