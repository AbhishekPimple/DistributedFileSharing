# DistributedFileSharing
This is a distributed file sharing application which is run either in server mode or client mode. Client can download, upload or sync files between other connected clients using command line interface. This application is built using socket programming in C on top of transmission control protocol (TCP).

How to run?

1. Compile this program using below command
  gcc DistFileSystem.c -o distfilesystem

2. Run the program using below command
  To run as server  
    ./distfilesystem -s <port-Num>
  where <port-Num> is the port number on which you want to run server. This program will behave as Server after running this command

  To run as client
    ./distfilesystem -c <port-Num>
  where <port-Num> is the port number on which you want to run client. This program will behave as Client after running this command
  
Utility commands
-All commands are case insensitive
 <COMMAND> : <DESCRIPTION OF COMMAND>
  For Server:
    DISPLAY : Displays IP address and port number of running server
    LIST : Displays list of connected servers with Connection ID, HOSTNAME, IP ADDRESS and PORT
    
  For Client:
    CREATOR : Displays information about creator of program
    DISPLAY : Displays IP address and port number of running server
    LIST : Displays list of current live connections  with Connection ID, HOSTNAME, IP ADDRESS and PORT
    REGISTER <IP ADDRESS> <PORT> : Register with server at <IP ADDRESS> running on port <PORT>
    CONNECT <DESTINATION> <PORT> : Connect to given destination <DESTINATION> server running on port <PORT>
    GET <ID> <FILENAME> : Get file <filename> from server at given connection ID
    PUT <ID> <FILENAME> : Send file <filename> to server at given connection ID
    SYNC : Send/Receive files from all connected clients
    TERMINATE <ID> : Terminate connection with given connection ID
    QUIT : Close all connections and terminate current process
