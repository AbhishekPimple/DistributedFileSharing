# Distributed file sharing application
This is a distributed file sharing application which can run either in server mode or client mode. Client can download, upload and sync files between other connected clients using command line interface. This application is built using socket programming in C on top of transmission control protocol (TCP).

## How to compile?
  
    gcc DistFileSystem.c -o distfilesystem
    

## How to run?
  
  ### Run as a server  
    
    ./distfilesystem -s [port-Num]
  
    where [port-Num] is the port number on which you want to run a server. This program will behave as a Server after running this command

  ### Run as  a client

    ./distfilesystem -c [port-Num]
    
    where [port-Num] is the port number on which you want to run a client. This program will behave as a Client after running this command
  

## Commands and their description
All commands are case insensitive.
 
 ### Server commands

| Command | Description |
| --- | --- |
| DISPLAY | Displays IP address and port number of running server |
| LIST | Displays list of connected servers with Connection ID, HOSTNAME, IP ADDRESS and PORT |
 
 ### Client commands

| Command | Description |
| --- | --- |
| CREATOR | Displays information about creator of program |
| DISPLAY | Displays IP address and port number of running client |
| LIST | Displays list of current live connections  with Connection ID, HOSTNAME, IP ADDRESS and PORT |
| REGISTER <IP ADDRESS> <PORT> | Register with server at <IP ADDRESS> running on port <PORT> |
| CONNECT <DESTINATION> <PORT>  | Connect to given destination <DESTINATION> server running on port <PORT> |
| GET <ID> <FILENAME>  | Get file <filename> from server at given connection ID |
| PUT <ID> <FILENAME> | Send file <filename> to server at given connection ID |
| SYNC | Send/Receive files from all connected clients i.e sync the all the clients to the same state |
| TERMINATE <ID> | Terminate connection with given connection ID |
| QUIT | Close all connections and terminate current process |
