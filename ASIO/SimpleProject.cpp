#include <iostream>
#include <vector>
#include <chrono>
// "define win32" will show as error if we dont define which windows version for will be used for the traslation unit
#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

// We use ASIO without the boost framework, so it needs to be defined as standalone
#define ASIO_STANDALONE
#include <asio.hpp>
// Will handle the movement of memory
#include <asio/ts/buffer.hpp>
// Prepares asio for network communication
#include <asio/ts/internet.hpp>


// As network connections can be unpredictable in the time we receive an specific response from a server or it may vary vastly on the amount
// of data we receive from it, then a simple but smart solution would be to instantiate an asynchronous function with ASIOs help for it to 
// read all the information it receives.
// As we dont know how much data we will receive on the http request, we will declare a vector with enough space for it to receive as much
// information as necessary
std::vector<char> vBuffer(20 * 1024);


void GrabSomeData(asio::ip::tcp::socket& socket)
{
	// We use the asynchronous version of the read_some function which will read portions of the data received by the socket, it will not cease
	// its process as there is a recursive call inside of it, it contains a lambda function which will check for errors and will read the size 
	// of the data that was read.
	socket.async_read_some(asio::buffer(vBuffer.data(), vBuffer.size()), [&](std::error_code ec, std::size_t length)
	{
		if (!ec)
		{
			std::cout << "\n\nRead " << length << " bytes\n\n";

			for (int i = 0; i < length; i++)
			{
				std::cout << vBuffer[i];
				// we use this function recursively as the way the data is received can be generally incomplete, if it is incomplete then the 
				// information will be displayed on the screen, then it will call itself again to gra the rest of the information there is to 
				// read, this process will repeat itself until there is no more data to be read and will then finish.
				GrabSomeData(socket);
			}
		}
	});
}

int main()
{
	// Used for error checking
	asio::error_code ec;
	// Creates a "context", which have all the platform specific requirements, where all the ASIO work is happening
	asio::io_context context;
	// Of the context to work, it needs a job to do, if it does not have anything to do then it will access immediately, so while it does have 
	// something useful to do, then we use an instance of type work exclusive for ASIO to start the context safely and for it to have something 
	// to do while the proper data is received
	asio::io_context::work idleWork(context);
	// We give the context its own thread, so in case it stops and wait, it does not stop the programs execution
	std::thread thrContext = std::thread([&]()
		{
			context.run();
		});
	// Gets th address of somewhere we wish to connect to on a TCP style, the endpoint or network connection is defined in an IP style and it,
	// handles the port 80, the "ec" will be used as an error handler, previously declared on the code. as for now, the endpoint will simply be
	// the address whe want to use
	asio::ip::tcp::endpoint endpoint(asio::ip::make_address("93.184.216.34", ec), 80);
	// Creates a networking socket, which will work as a hook into the operating system network drivers and will act as a doorway to the network 
	// that we will connect it to, we got to associate it with the context we instanciated earlier
	asio::ip::tcp::socket socket(context);
	// Tells the socket to try and connect using the endpoint address, adding the "ec" error checking in case it does not connect
	socket.connect(endpoint, ec);

	if (!ec)
	{
		std::cout << "Connected!" << std::endl;
	}
	else
	{
		std::cout << "Failed to connect to address:\n" << ec.message() << std::endl;
	}

	// If this method is true, it means the connection is active and alive, which means that we can send or receive information from it
	if (socket.is_open())
	{
		// We prime the context with the ability to retrieve some data before we send the first request, all the required procedure for 
		// the context to work properly was previously specified in the thread declaration and the work instance from the ASIO library
		GrabSomeData(socket);

		// For this example, we are trying to do an http request, for this request to work we are sending a very basic HTTP request
		std::string sRequest =
			"GET /index.html HTTP/1.1\r\n"
			"Host: example.com\r\n"
			"Connection: close\r\n\r\n";
		// The write_some function will try and send as much data as possible, when reading and writing data we always use the ASIO buffer, 
		// it is an array which will contain an array of bytes, in this case, we are taking the bytes of the string for the http request, and 
		// the size for the buffer to dimention the array accordingly
		socket.write_some(asio::buffer(sRequest.data(), sRequest.size()), ec);
		// The connection takes time for it to be processed, so its good practice to use the sockets wait function for it to force the delay and 
		// wait to receive the data accordingly 
		
		// We force the program to wait for 10 seconds so the context have enough time to retrieve the information it is requesting
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(10000ms);
		
		// The context is stopped and checks if it still has some data to retrieve from the request, if it has, then the thread will join the process 
		// previously started
		context.stop();
		if (thrContext.joinable()) thrContext.join();


		/* THIS SECTION IS A SMALL DEMONSTRATION ON HOW TO READ THE DATA MANUALLY, WITHOUT USING RECURSIVE FUNCTIONS OR ASYNCHRONOUS METHODS

		socket.wait(socket.wait_read);
		// after we send the request, we got to declare a variable which will read whatever information we receive from that connection through the socket with the available() function.
		size_t bytes = socket.available();
		std::cout << "Bytes available: " << bytes << std::endl;
		// if there are bytes then we will read them using a container of bytes, same as the buffer we used to send the information, we call the read_some function this time, which works pretty much the same way as the write_some, but it is to receive the data and store it accordingly
		if (bytes > 0)
		{
			std::vector<char> vBuffer(bytes);
			socket.read_some(asio::buffer(vBuffer.data(), vBuffer.size()), ec);
			// display of the information read by the buffer.
			for (auto c : vBuffer)
			{
				std::cout << c;
			}
		}
		*/
	}

	return 0;
}