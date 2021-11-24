#include "thread.h"
#include "socketserver.h"
#include <stdlib.h>
#include <time.h>
#include <list>
#include <vector>
#include <algorithm>

using namespace Sync;

// This thread handles the server operations
class ServerThread : public Thread
{
private:
    SocketServer &server;

public:
    Event connEvent;
    Event shutdownEvent;
    bool *shutdown;
    ServerThread(SocketServer &server, Event &e, bool *s, Event &e2)
        : server(server)
    {
        connEvent = *new Event(e);
        shutdown = s;
        shutdownEvent = *new Event(e2);
    }

    ~ServerThread()
    {
        terminationEvent.Wait();
        // Cleanup
        //...
    }

    virtual long ThreadMain()
    {
        std::cout << "Thread Created" << std::endl;
        // Wait for a client socket connection
        ByteArray data;
        std::string inStr;
        std::string outStr;
        Socket *newConnection = new Socket(server.Accept());
        connEvent.Trigger();
        // A reference to this pointer
        Socket &socket = *newConnection;
        FlexWait incomingData(2, &socket, &shutdownEvent);
        while (!*shutdown)
        {
            incomingData.Wait();
            if (*shutdown)
            {
                break;
            }
            socket.Read(data);
            inStr = data.ToString();
            if (inStr == "done") //Close client terminal
            {
                break;
            }
            else if (inStr == "Close") //shutdown server
            {
                *shutdown = true;
                shutdownEvent.Trigger(); //trigger this to allow the server to exit
                break;
            }
            outStr = "";
            for (int i = 0; i < inStr.length(); i++)
            {
                outStr += toupper(inStr[i]);
            }
            data = *new ByteArray(outStr);
            socket.Write(data);
        }
        //if the code reaches here, shutdown is initiated.
        socket.Write(*new ByteArray("Close")); //send the shutdown signal

        return 1;
    }
};

int main(void)
{
    std::cout << "I am a server." << std::endl;
    // Create our server
    SocketServer server(3000);
    Event connEvent;
    Event shutdownEvent;
    // Need a thread to perform  server operations
    bool shutdown = false;
    FlexWait serverWaiter(2, &server, &shutdownEvent);
    FlexWait eventWaiter(1, &connEvent);
    while (!shutdown)
    {
        serverWaiter.Wait(); //wait until there is an attempt to connect, then..
        if (!shutdown)
        {
            ServerThread *serverThread = new ServerThread(server, connEvent, &shutdown, shutdownEvent);
        }
        else
        {
            break;
        }
        eventWaiter.Wait();
        connEvent.Reset();
    }

    server.Shutdown(); // Shut down and clean up the server
}