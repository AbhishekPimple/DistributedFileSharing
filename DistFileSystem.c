/*
*
• apimple_proj1.c : Single file to handle the file sharing application
• Starts off as a server or a client on a given port
• Created for CSE 589 Fall 2015 Programming Assignment 1
• @author Abhishek Pimple
• @created 26th Sepetember 2015
• 
*
*/
/* Adding Sync implementation*/

#include < stdio.h > 
#include < stdlib.h > 
#include < string.h > 
#include < sys / types.h > 
#include < sys / time.h > 
#include < sys / socket.h > 
#include < sys / select.h > 
#include < netinet / in .h > 
#include < errno.h > 
#include < string.h > 
#include < unistd.h > 
#include < time.h >
#include < arpa / inet.h >

#define FDESC_STDIN 0
//BACKLOG defines maximum number of live connections server can hold
# define BACKLOG 8# define PACKET_SIZE 1400

struct server_ip_list {
  char id[2];
  char hostname[150];
  char ip_address[150];
  char port_no[50];

  //Holds socket descriptor for given hostname
  int socket_descriptor;
  //Holds connection status. 1 implies this host is connected, 0 imples this host is NOT connected
  int connected;
};

//Global Variables
char * server_ip_address, server_host_name[100];
int portNum;
char file_name[100];
int file_size, bytes_written;
char listening_port[20];
struct timeval start_time, end_time;
int listener_socket, client, len, retVal, socket_index;
static int fdmax;
int newfd = -1; //Newly accepted file descriptor
struct sockaddr_in server, other_server;
char remoteIP[INET_ADDRSTRLEN];
fd_set master;
fd_set readfds;

//Helper functions

//Displays server ip address and port number
void display() {

  printf("IP address: %s \n", server_ip_address);
  printf("Port: %d \n", portNum);
  fflush(stdout);
}

//Displays creator name, ubit name and email address
void displayCreator() {
  printf("\nName: ABHISHEK KAMALAKAR PIMPLE \n");
  printf("UBIT: apimple\n");
  printf("Email: apimple@buffalo.edu \n");
  fflush(stdout);

}

//Displays command for server
void displayHelpForServer() {
    printf("FORMAT for help is COMMAND : DESCRIPTION OF COMMAND\n\n");
    printf("CREATOR : Displays name, UBIT name and email Id of the creator of program\n");
    printf("DISPLAY : Displays IP address and port number of running server\n");
    printf("LIST : Displays list of connected servers with Connection ID, HOSTNAME, IP ADDRESS and PORT\n");
    fflush(stdout);
  }
  //Displays commands for client
void displayHelpForClient() {
  printf("FORMAT for help is COMMAND : DESCRIPTION OF COMMAND\n\n");
  printf("CREATOR : Displays name, UBIT name and email Id of the creator of program\n");
  printf("DISPLAY : Displays IP address and port number of running server\n");
  printf("LIST : Displays list of current live connections  with Connection ID, HOSTNAME, IP ADDRESS and PORT\n");
  printf("REGISTER <IP ADDRESS> <PORT> : Register with server at <IP ADDRESS> running on port <PORT>\n");
  printf("CONNECT <DESTINATION> <PORT> : Connect to given destination <DESTINATION> server running on port <PORT>\n");
  printf("GET <ID> <FILENAME> : Get file <filename> from server at given connection ID\n");
  printf("PUT <ID> <FILENAME> : Send file <filename> to server at given connection ID\n");
  printf("SYNC : Send/Receive files from all connected clients\n");
  printf("TERMINATE <ID> : Terminate connection with given connection ID\n");
  printf("QUIT : Close all connections and terminate current process\n");
  fflush(stdout);
}

//Displays server ip list OR client list
void displayServerIPList(struct server_ip_list server_list[5], int totalNumberOfServers) {
  int i = 0;
  if (totalNumberOfServers == 0) {
    printf("Server IP list contains no servers \n");
    fflush(stdout);
    return;
  } else {
    printf("\n%-5s%-35s%-20s%-8s\n", "ID", "HOSTNAME", "IP ADDRESS", "PORT");
    for (i; i < totalNumberOfServers; i++) {
      printf("%-5d%-35s%-20s%-8d\n", atoi(server_list[i].id), server_list[i].hostname, server_list[i].ip_address, atoi(server_list[i].port_no));
    }
    fflush(stdout);
  }
}

//Finds host name and sets into server_host_name which is global variable
void find_my_hostname() {
    struct in_addr ipv4addr;
    struct hostent * he;
    inet_pton(AF_INET, server_ip_address, & ipv4addr);
    he = gethostbyaddr( & ipv4addr, sizeof ipv4addr, AF_INET);
    if (he) {
      strcpy(server_host_name, he - > h_name);
    }
  }
  /*  Get Public IP address of current server by creating UDP socket with google DNS server.
  Returns Public IP address of current server.
  */
char * get_my_ip() {

  char * google_dns_server = "8.8.8.8";
  char * google_dns_port = "53";

  /* get peer server */
  struct addrinfo hints;
  memset( & hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo * info;
  int ret = 0;
  if ((ret = getaddrinfo(google_dns_server, google_dns_port, & hints, & info)) != 0) {
    printf("[ERROR]: getaddrinfo error: %s\n", gai_strerror(ret));
    return "NULL";
  }

  if (info - > ai_family == AF_INET6) {
    printf("[ERROR]: do not support IPv6 yet.\n");
    return "NULL";
  }

  /* create socket */
  int sock = socket(info - > ai_family, info - > ai_socktype, info - > ai_protocol);
  if (sock <= 0) {
    perror("socket");
    return "NULL";
  }

  /* connect to server */
  if (connect(sock, info - > ai_addr, info - > ai_addrlen) < 0) {
    perror("connect");
    close(sock);
    return "NULL";
  }

  /* get local socket info */
  struct sockaddr_in local_addr;
  socklen_t addr_len = sizeof(local_addr);
  if (getsockname(sock, (struct sockaddr * ) & local_addr, & addr_len) < 0) {
    perror("getsockname");
    close(sock);
    return "NULL";
  }

  /* get peer ip addr */
  char * myip, myip1[INET_ADDRSTRLEN];
  myip = (char * ) malloc(INET_ADDRSTRLEN * sizeof(char));
  if (inet_ntop(local_addr.sin_family, & (local_addr.sin_addr), myip1, sizeof(myip1)) == NULL) {
    perror("inet_ntop");
    return "NULL";
  }
  strcpy(myip, myip1);
  fflush(stdout);
  return myip;

}

// get sockaddr, IPv4
void * get_in_addr(struct sockaddr * sa) {
  return &(((struct sockaddr_in * ) sa) - > sin_addr);
}

/*Ref: http://stackoverflow.com/questions/5457608/how-to-remove-a-character-from-a-string-in-c*/
void removeChar(char * str, char garbage) {

  char * src, * dst;
  for (src = dst = str; * src != '\0'; src++) { * dst = * src;
    if ( * dst != garbage) dst++;
  } * dst = '\0';
}

//initialize server ip list with default values
void initialize_server_ip_list(struct server_ip_list server_ip_list_init[5]) {

  //Initially No clients and socket descriptor are present.
  //So all connected and socket descriptor are set to 0 and -1 respectively.
  int i = 0;
  for (i; i < 5; i++) {
    server_ip_list_init[i].connected = 0;
    server_ip_list_init[i].socket_descriptor = -1;
  }
}

//Helps in creating socket, binding and listening
void setupThisServer(int portNum) {

  //Create server socket
  if ((listener_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Error in creating socket:");
    exit(-1);
  }

  server.sin_family = AF_INET;
  server.sin_port = htons(portNum);
  server.sin_addr.s_addr = INADDR_ANY; /* Get public IP */
  bzero( & server.sin_zero, 8);

  len = sizeof(struct sockaddr_in);
  //Bind IP and Port to server 
  if ((bind(listener_socket, (struct sockaddr * ) & server, len)) == -1) {
    perror("Error in Bind: ");
    exit(-1);
  }

  if (listen(listener_socket, BACKLOG) == -1) {
    perror("Error in Listening: ");
    exit(-1);
  }

  //Clear all bits in readfds and master
  FD_ZERO( & readfds);
  FD_ZERO( & master);

  //Set descriptors for STDIN data input and Listerning socket which we have just created.
  FD_SET(FDESC_STDIN, & master);
  FD_SET(listener_socket, & master);

  //Keeping track of biggest file descriptor
  fdmax = listener_socket;
}

//This function registers client with server.
void registerThisClient(char splitCommand[100][100], struct server_ip_list client_list[5], char listening_port[20]) {
  int rv, sockfd, i;
  struct addrinfo hints, * servinfo, * p;

  char buffer[20];
  char cmd[100], data_for_server[100];

  //Ref: Beej's guide to network programming.pdf
  memset( & hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(splitCommand[1], splitCommand[2], & hints, & servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(1);
  }

  int sprintf_ret = sprintf(client_list[0].id, "%d", 1);
  strcpy(client_list[0].ip_address, splitCommand[1]);
  strcpy(client_list[0].port_no, splitCommand[2]);

  for (p = servinfo; p != NULL; p = p - > ai_next) {
    if ((sockfd = socket(p - > ai_family, p - > ai_socktype, p - > ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }
    if (connect(sockfd, p - > ai_addr, p - > ai_addrlen) == -1) {
      close(sockfd);
      perror("client: connect");
      continue;
    }
    break;
  }

  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    exit(1);
  }

  //Enter server entry in client list
  sprintf(client_list[0].id, "%d", 1);
  strcpy(client_list[0].ip_address, splitCommand[1]);
  strcpy(client_list[0].port_no, splitCommand[2]);
  client_list[0].socket_descriptor = sockfd;
  client_list[0].connected = 1;
  struct in_addr ipv4addr;
  struct hostent * he;

  inet_pton(AF_INET, client_list[0].ip_address, & ipv4addr);
  he = gethostbyaddr( & ipv4addr, sizeof ipv4addr, AF_INET);
  if (he) {
    //Copying hostname in server ip list structure
    strcpy(client_list[0].hostname, he - > h_name);
  }
  //Ref: http://stackoverflow.com/questions/7315936/which-of-sprintf-snprintf-is-more-secure
  snprintf(buffer, sizeof(buffer), "%d", portNum);
  sprintf(data_for_server, "%s|%s", splitCommand[0], buffer);
  printf("data_for_server : %s", data_for_server);
  //Sending "REGISTER|<Client Port Number>" to server so that server can parse the string and identify client listening port number
  if (send(sockfd, data_for_server, strlen(data_for_server), 0) == -1) {
    perror("send");
  }
}

//Sends SYNC command to server
int sendSyncCommandtoserver(struct server_ip_list client_list[5], int totalNumberOfServers) {

  int i = 1;
  char data_for_server[100];
  int sockfd;

  if (client_list[0].id == 0) {
    printf("You have to register with server first before you can execute sync command \n");
    return 0;
  }
  //Get socket descriptor of timberlake server
  sockfd = client_list[0].socket_descriptor;
  //printf("socket descriptor in sync command : %d \n",sockfd);
  //printf("HOSTNAME in sync command : %s \n", client_list[0].hostname);

  sprintf(data_for_server, "%s\n", "SYNC");
  //printf("data_for_server : %s", data_for_server);

  //Send SYNC command to server so that server can take necessary action
  if (send(sockfd, data_for_server, strlen(data_for_server), 0) == -1) {
    perror("Error in send");
    return 0;
  }

  return 1;
}

//used to connect clients with other connected clients
int connectThisClient(char splitCommand[100][100], struct server_ip_list client_list[5], int totalNumberOfClients, struct server_ip_list server_list_onClient[5], int totalNumberOfServers) {

  struct addrinfo hints, * servinfo, * p;
  int rv, sockfd, i, can_connect = 0, found_iphost = 0, found_port = 0, server_index, is_ip = 1;

  char cmd[100], server_send[100];
  if (totalNumberOfClients > 3) {
    printf("\nConnection to more than 3 clients is NOT allowed\n");
    return -1;
  }

  if (!(strcmp(splitCommand[1], server_ip_address))) {
    printf("\n You can not connect to your Own IP address. Please enter new IP address.");
    return -1;
  }
  //Check whether client is registered with server or not by looking into server_ip_list on client
  for (i = 0; i < totalNumberOfServers; i++) {

    is_ip = strcmp(splitCommand[1], server_list_onClient[i].ip_address);
    if (is_ip == 0 || (strcmp(splitCommand[1], server_list_onClient[i].hostname) == 0)) {
      found_iphost = 1;
      if ((strcmp(splitCommand[2], server_list_onClient[i].port_no) == 0)) {
        found_port = 1;
        server_index = i; //Holds the index of row which matches the client this client wants to connect to.
        break;
      }
    }

  }

  if (found_iphost == 0) {
    printf("\n The client you want to connect is NOT registered with server. \n");
    return -1;
  }

  if (found_port == 0) {
    printf("\n The client you want to connect is NOT listening on port number you have provided. \n");
    return -1;
  }

  //check if client has already made connection with given client
  for (i = 1; i < totalNumberOfClients; i++) {
    if ((strcmp(splitCommand[1], client_list[i].ip_address) == 0) || (strcmp(splitCommand[1], client_list[i].hostname) == 0)) {
      can_connect = 1;
      break;

    }
  }
  if (can_connect == 1) {
    printf("\nHey, you have already connected to this client. :) \n");
    return -1;
  }

  //Ref: beej
  if (is_ip != 0) {
    struct hostent * he;
    struct in_addr * * addr_list;

    // get that client info
    if ((he = gethostbyname(splitCommand[1])) == NULL) {
      herror("gethostbyname");
      return 2;
    }

    addr_list = (struct in_addr * * ) he - > h_addr_list;
    strcpy(splitCommand[1], inet_ntoa( * addr_list[0]));
    //printf("converted Ip address is %s",splitCommand[1]);
  }
  memset( & hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(splitCommand[1], splitCommand[2], & hints, & servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(1);
  }

  // loop through all the results and connect to the first we can
  for (p = servinfo; p != NULL; p = p - > ai_next) {
    if ((sockfd = socket(p - > ai_family, p - > ai_socktype,
        p - > ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }

    if (connect(sockfd, p - > ai_addr, p - > ai_addrlen) == -1) {
      close(sockfd);
      perror("client: connect");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    exit(1);
  } else if (sockfd) {
    printf("\n Client is successfully connected to given client!! \n");

    int numbytes, sprintf_ret;
    char buf[20];
    strcpy(client_list[totalNumberOfClients].hostname, server_list_onClient[server_index].hostname);
    strcpy(client_list[totalNumberOfClients].ip_address, server_list_onClient[server_index].ip_address);
    strcpy(client_list[totalNumberOfClients].port_no, server_list_onClient[server_index].port_no);

    sprintf(client_list[totalNumberOfClients].id, "%d", (totalNumberOfClients + 1));

    //Changing the connection status 
    client_list[totalNumberOfClients].connected = 1;
    client_list[totalNumberOfClients].socket_descriptor = sockfd;

    //Return connection ID of connnected client.
    return i;

  }

}

//Use to put file into other client
int putThisFile(char dest_id[10], char file_name[100], struct server_ip_list client_list[5], int totalNumberOfClients) {
  //printf("Got PUT request for filename : %s from %s dest_id \n", file_name, dest_id);
  FILE * fp;
  struct timeval start_time, end_time;
  gettimeofday( & start_time, NULL);

  char buffer[PACKET_SIZE], packet_send[PACKET_SIZE], first_packet[PACKET_SIZE], send_file_name[100], buf[100], dest_hostname[100], stats_string[1000];
  int file_size, sock_des_send, i, no_read;

  memset(packet_send, '\0', PACKET_SIZE);
  memset(first_packet, '\0', PACKET_SIZE);

  // extract just filename from path
  strcpy(buf, file_name);
  strcpy(send_file_name, "NULL");
  char partitionCharacter[2] = "/";
  char * arg;
  int argc_1 = 0;
  arg = (char * ) malloc(100 * sizeof(char));

  arg = strtok(buf, partitionCharacter);

  while (arg != NULL) {
    fflush(stdout);
    strcpy(send_file_name, arg);
    arg = strtok(NULL, partitionCharacter);
  }

  int ctr = 0, to_read = 0;
  //Using rb as files can be non-text files too.
  //Ref: http://stackoverflow.com/questions/2174889/whats-the-differences-between-r-and-rb-in-fopen
  fp = fopen(file_name, "rb");
  if (!fp) {
    printf("\n fopenn error \n");
    return -1;
  }
  int found_id = 0;
  //Find socket descriptor from client list for given dest_id
  for (i = 1; i < totalNumberOfClients; i++) {
    if ((strcmp(client_list[i].id, dest_id) == 0)) {
      sock_des_send = client_list[i].socket_descriptor;
      strcpy(dest_hostname, client_list[i].hostname);
      found_id = 1;
      break;
    }
  }
  if (found_id == 0) {
    return -1;
  }
  // seek to end of file
  fseek(fp, 0, SEEK_END);
  // get current file pointer
  file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  int sprintf_returnVal;
  //only file name is given as input
  if ((strcmp(send_file_name, "NULL")) != 0) {
    sprintf_returnVal = sprintf(packet_send, "filename-%s;filesize-%d", send_file_name, file_size);
  } else {
    sprintf_returnVal = sprintf(packet_send, "filename-%s;filesize-%d", file_name, file_size);
  }

  to_read = PACKET_SIZE - sprintf_returnVal - 1;
  no_read = fread(buffer, 1, to_read, fp);
  sprintf(first_packet, "%s;%s", packet_send, buffer);
  int first_send = no_read + sprintf_returnVal + 1;
  int numBytesSent = send(sock_des_send, first_packet, first_send, 0);
  if (numBytesSent == -1) {
    perror("send");
  }

  while (no_read) {

    memset(packet_send, '\0', PACKET_SIZE);
    /* Read and display data */
    no_read = fread(packet_send, 1, PACKET_SIZE, fp);
    if (no_read == 0) {
      //Nothing.. so break out of while loop 
      break;
    }
    numBytesSent = send(sock_des_send, packet_send, no_read, 0);

    if (numBytesSent == -1) {
      perror("send");
    }
  }
  //Closing file pointer as we are done with the file.
  fclose(fp);

  gettimeofday( & end_time, NULL);

  //Ref: http://stackoverflow.com/questions/2150291/how-do-i-measure-a-time-interval-in-c
  long diff = ((end_time.tv_sec - start_time.tv_sec) * 1000000) + (end_time.tv_usec - start_time.tv_usec);
  long double time_taken = (long double) diff / 1000000;
  long double rate = (long double)(file_size * 8) / time_taken;
  printf("\nUpload Complete\n");
  printf("\n Uploading file operation details \n");
  printf("\n Tx: %s -> %s \n File Size :%d bytes \n Time Taken: %Lf seconds \n Tx Rate:%Lf bits/second\n", server_host_name, client_list[i].hostname, file_size, time_taken, rate);

  if (send(client_list[0].socket_descriptor, stats_string, sprintf_returnVal, 0) == -1) {
    perror("send");
  }

  return 1;

}

//Parse string to analyze command
void parseGetString(char peer_send[PACKET_SIZE], char source_id[10], struct server_ip_list client_list[5], int totalNumberOfClients) {

  //printf("peer send string is : %s \n", peer_send);
  char partitionCharacter[2] = " |";
  char * arg, buff[PACKET_SIZE], file_name[100];
  arg = (char * ) malloc(strlen(peer_send) * sizeof(char));
  strcpy(buff, peer_send);
  arg = strtok(buff, partitionCharacter);
  int argc_1 = 0;

  while (arg != NULL) {
    argc_1 += 1;
    if (argc_1 == 2) {
      strcpy(file_name, arg);
    }
    arg = strtok(NULL, partitionCharacter);
  }

  //Because download 2 <filename> from client_id 1 is equivalent to upload <client-id = 1> filename command from client-id 2 
  int ret = putThisFile(source_id, file_name, client_list, totalNumberOfClients);
  if (ret != 1) {
    printf("\nUpload file failed");
  }

}

//Use to get file from other connnected clients
void getThisFile(char splitCommand[100][100], int totalNumberOfArguments, struct server_ip_list client_list[5], int totalNumberOfClients) {

  int i, sock_des_send, j, found_id = 0;
  char download_string[100];
  for (i = 1; i < totalNumberOfArguments;) {

    if (!strcasecmp(splitCommand[0], "GET")) {
      int sprintf_returnVal = sprintf(download_string, "GET|%s", splitCommand[i + 1]);
      found_id = 0;

      //Get socket descriptor of destination ID
      for (j = 1; j < totalNumberOfClients; j++) {
        if ((strcmp(client_list[j].id, splitCommand[i]) == 0)) {
          sock_des_send = client_list[j].socket_descriptor;
          found_id = 1;
          break;
        }

      }
      if (found_id == 0) {
        printf("\nDestination ID : %s is not present in client list\n", splitCommand[i]);
      } else {
        int sent_size = send(sock_des_send, download_string, sprintf_returnVal, 0);
        if (sent_size == -1) {
          perror("Error in sending \n");
        }

      }

      i += 2; //Done
    }

  }
}

//Get files for sync command
int getThisFilesForSync(char hostnameFiles[100][100], struct server_ip_list client_list[5], int totalNumberOfClients) {
  //printf("In getThisFilesForSync function \n");
  int sock_des_send, j;

  int i = 0;
  for (j = 1; j < totalNumberOfClients; j++) {
    char getSync_string[100];
    sock_des_send = client_list[j].socket_descriptor;
    //printf("Sending request for file : %s to id: %s \n",hostnameFiles[i+1],client_list[j].id);
    int sprintf_returnVal = sprintf(getSync_string, "GETSYNC|%s", hostnameFiles[i + 1]);
    int sent_size = send(sock_des_send, getSync_string, sprintf_returnVal, 0);
    if (sent_size == -1) {
      perror("Error in sending \n");
      return 0;
    }
    i += 1;

  }
  return 1;
}

//Used to receive files from other client
int receiveThisFile(char peer_send[PACKET_SIZE], char file_name[100], int * file_size, int * bytes_written, int noofbytes) {

  char packet_write[PACKET_SIZE];
  FILE * fp = NULL;
  //printf("In receiveThisFile for filename : %s \n", file_name);

  memset(packet_write, '\0', PACKET_SIZE);

  if ((strstr(peer_send, "filename-") != NULL)) {
    //find filename and  filesize

    //initialize things for new file
    * file_size = 0; * bytes_written = 0;
    char s[2] = ";";
    char s2[2] = "-", token2[PACKET_SIZE][PACKET_SIZE];
    char * token, * token3;
    int ctr = 0, token2_ctr = 0;
    token = (char * ) malloc(noofbytes * sizeof(char));
    token3 = (char * ) malloc(noofbytes * sizeof(char));
    token = strtok(peer_send, s);

    while (token != NULL) {
      //printf( "token is %s\n", token );
      fflush(stdout);
      strcpy(token2[token2_ctr], token);
      token2_ctr++;

      //last string will be blank
      if ((strcmp(token, " ") == 0))
        break;
      token = strtok(NULL, s);
    }
    int x = noofbytes - strlen(token2[0]) - strlen(token2[1]) - 2; //no of bytes to write(2 delimitators)

    token3 = strtok(token2[0], s2);
    token3 = strtok(NULL, s2);
    if (token3) {
      strcpy(file_name, token3);
      token3 = strtok(token2[1], s2);
      token3 = strtok(NULL, s2);; * file_size = atoi(token3);
    }
    strcpy(packet_write, token2[2]);

    fp = fopen(file_name, "wb");
    if (fp) {
      fwrite(packet_write, 1, x, fp);
    } else {
      printf("\nCould not open file");
      return -1;
    } * bytes_written += x;

  } else {
    //write in file

    fp = fopen(file_name, "ab"); //open in append mode
    if (!fp) {
      printf("\n Could not open file");
      return -1;
    }

    if ( * bytes_written < * file_size) { * bytes_written += noofbytes;
      if ( * bytes_written > * file_size) { * bytes_written -= noofbytes;
        int write_last = * file_size - * bytes_written;
        peer_send[write_last] = '\0'; // remove extra characters if any
        fwrite(peer_send, 1, write_last, fp); * bytes_written += write_last;
      } else {
        fwrite(peer_send, 1, noofbytes, fp);
      }
    }

  }

  //Close file
  fclose(fp);
  return 1;
}

//Sends sync command to clients
int sendSyncCommandtoClients(char data[100], struct server_ip_list server_list[5], int totalNumberOfServers) {

  if ((strstr(data, "SYNC") != NULL) || (strstr(data, "SYNC") != NULL)) {
    char previous_string[500];
    strcpy(previous_string, "SYNC");
    int j;
    //printf("Sending this string to client \n %s",previous_string);
    for (j = 0; j < totalNumberOfServers; j++) {
      if (send(server_list[j].socket_descriptor, previous_string, strlen(previous_string), 0) < 0) {
        perror("send");
        return 0;
      }
    }
    return 1;
  }

  return 0;
}

int getAllFilesForSync(struct server_ip_list client_list[5], int totalNumberOfClients) {
  //printf("In getAllFilesForSync function \n");
  int i = 1;
  char hostnamefiles[100][100];
  //printf("Received totalNumberOfClients : %d \n", totalNumberOfClients);
  for (i; i < totalNumberOfClients; i++) {
    //Download <hostname>.txt files from all connected clients where <hostname> is connected client specific.
    char fname[50];
    //address = getSubString(client_list[i].hostname);
    char * ptr;
    ptr = strtok(client_list[i].hostname, ".");
    strcpy(fname, ptr);
    strcat(fname, ".txt");
    strcpy(hostnamefiles[i], fname);

  }

  /* for(i=1 ; i < totalNumberOfClients; i++){
       printf("Hostnames file : %s\n", hostnamefiles[i]);
   }*/

  int returnValue = getThisFilesForSync(hostnamefiles, client_list, totalNumberOfClients);
  return returnValue;
}

int registerThisServer(char data[100], struct server_ip_list server_list[5], int totalNumberOfServers) {

  if ((strstr(data, "REGISTER") != NULL) || (strstr(data, "register") != NULL)) {
    //Extract Port number from data
    char * arg;
    int argc_1 = 0, newfd;
    char partitionCharacter[2] = "|";
    arg = (char * ) malloc(100 * sizeof(char));
    //for strtok everywhere took help from recitation slides
    arg = strtok(data, partitionCharacter);

    //Update server_ip_list
    while (arg != NULL) {
      if (argc_1 == 1) {
        strcpy(server_list[totalNumberOfServers].port_no, arg);
      }
      argc_1 += 1;
      arg = strtok(NULL, partitionCharacter);
    }

    sprintf(server_list[totalNumberOfServers].id, "%d", (totalNumberOfServers + 1));

    //Ref: beej

    socklen_t len;
    struct sockaddr_storage addr;

    int port;

    len = sizeof addr;
    getpeername(server_list[totalNumberOfServers].socket_descriptor, (struct sockaddr * ) & addr, & len);

    if (addr.ss_family == AF_INET) {
      struct sockaddr_in * s = (struct sockaddr_in * ) & addr;
      port = ntohs(s - > sin_port);
      inet_ntop(AF_INET, & s - > sin_addr, server_list[totalNumberOfServers].ip_address, sizeof server_list[totalNumberOfServers].ip_address);
    }

    struct in_addr ipv4addr;
    struct hostent * he;

    inet_pton(AF_INET, server_list[totalNumberOfServers].ip_address, & ipv4addr);
    he = gethostbyaddr( & ipv4addr, sizeof ipv4addr, AF_INET);
    if (he) {
      strcpy(server_list[totalNumberOfServers].hostname, he - > h_name);
    }

    int i, j, n;
    char connected_clients[100];
    char previous_string[500];
    strcpy(previous_string, "NULL");
    //do for host name
    totalNumberOfServers += 1;

    //Send updated server_ip_list to all connected clients
    for (i = 0; i < totalNumberOfServers; i++) {
      sprintf(connected_clients, "%s|%s|%s|%s:", server_list[i].id, server_list[i].hostname, server_list[i].ip_address, server_list[i].port_no);
      if (strcmp(previous_string, "NULL") == 0) {
        strcpy(previous_string, connected_clients);
      } else {
        strcat(previous_string, connected_clients);
      }

    }
    //printf("Sending this string to client \n %s",previous_string);
    for (j = 0; j < totalNumberOfServers; j++) {
      if (send(server_list[j].socket_descriptor, previous_string, strlen(previous_string), 0) < 0) {
        perror("send");
      }
    }
  }

  return totalNumberOfServers;
}

int terminateThisClient(char id_terminate[10], struct server_ip_list client_list[5], int totalNumberOfClients) {
  int found_id = -1, i;
  for (i = 0; i < totalNumberOfClients; i++) {
    if ((strcmp(client_list[i].id, id_terminate) == 0)) {
      //Change connected status of client to 0 from 1.
      client_list[i].connected = 0;
      found_id = 1;
      break;
    }
  }
  if (found_id == -1) {
    printf("\n Client ID you want to terminate does not exist in client list \n");
  }

  return found_id;
}

int deleteServerFromServerIpList(struct server_ip_list server_list[5], int totalNumberOfServers, int id) {
  int i;

  for (i = id; i < totalNumberOfServers - 1; i++) {
    //Copy next row to present row. 
    //Better way to do this is to have linked list of server IP list. 
    //Kind of inefiicient for large server ip list. But fast and easy for small server IP list. 

    strcpy(server_list[i].id, server_list[i + 1].id);
    strcpy(server_list[i].hostname, server_list[i + 1].hostname);
    strcpy(server_list[i].port_no, server_list[i + 1].port_no);
    strcpy(server_list[i].ip_address, server_list[i + 1].ip_address);
    server_list[i].socket_descriptor = server_list[i + 1].socket_descriptor;
    server_list[i].connected = server_list[i + 1].connected;
  }

  //Reduce totalNumberOfServers
  totalNumberOfServers -= 1;

  return totalNumberOfServers;

}

void runAsServer(int portNum) {
  struct server_ip_list server_list[5];
  int argc_1, lastChar;
  int numReceived;
  char cmd[100];
  char data[4000];
  int totalNumberOfServers = 0;
  initialize_server_ip_list(server_list);
  setupThisServer(portNum);
  argc_1 = 0;
  while (1) {
    // printf("Starting infinite while loop \n");
    // printf("Copying master to readfds \n");
    readfds = master;
    printf("[PA1]$ ");
    fflush(stdout);
    retVal = select(fdmax + 1, & readfds, NULL, NULL, NULL);
    if (retVal == -1) {
      perror("Error in select: ");
      exit(-1);
    } else {
      argc_1 = 0;
      for (socket_index = 0; socket_index <= fdmax; socket_index++) {
        //printf("Inside the for loop. socket_index: %d and fdmax: %d \n", socket_index, fdmax);
        //printf("Server socket fd is :%d \n", listener_socket);
        if (FD_ISSET(socket_index, & readfds)) {
          //printf("FD was set for fd = %d \n", socket_index);
          if (socket_index == FDESC_STDIN) {
            //Ref: From recitation PDF
            fgets(cmd, 100, stdin);
            fflush(stdout);
            char splitCommand[100][100];
            char * arg;
            arg = (char * ) malloc(strlen(cmd) * sizeof(char));
            arg = strtok(cmd, " ");
            while (arg != NULL) {
              //printf( "Arg is %s\n", arg );
              fflush(stdout);
              strcpy(splitCommand[argc_1], arg);
              argc_1 += 1;
              arg = strtok(NULL, " ");
            }
            //Last character of last string has \n
            lastChar = strlen(splitCommand[argc_1 - 1]) - 1;
            splitCommand[argc_1 - 1][lastChar] = '\0';

            //printf("Current command is: %s", splitCommand[0]);
            if (!strcasecmp(splitCommand[0], "HELP")) {
              displayHelpForServer();
            } else
            if (!strcasecmp(splitCommand[0], "CREATOR")) {
              displayCreator();
            } else
            if (!strcasecmp(splitCommand[0], "DISPLAY")) {
              display();
            } else
            if (!strcasecmp(splitCommand[0], "LIST")) {
              displayServerIPList(server_list, totalNumberOfServers);
            } else {
              printf("Inavlid Command. Type HELP command for valid commands.\n");
              fflush(stdout);
            }
          } else if (socket_index == listener_socket) {
            //Handle New Connection
            //printf("Handling new connection \n");
            int len = sizeof(struct sockaddr_in);
            int temp = socket_index;
            // printf("Before accept: i: %d \n", i);
            newfd = accept(listener_socket, (struct sockaddr * ) & other_server, & len);

            //  printf("After accept: i: %d \n", i);
            socket_index = temp;
            if (newfd == -1) {
              perror("Error in accepting new connection: ");
              exit(-1);
            } else {
              server_list[totalNumberOfServers].socket_descriptor = newfd;

              if (newfd > fdmax) {
                //printf("Current fdmax is %d \n",fdmax);
                //printf("fdmax is changed to %d from %d when socket_index:%d \n", newfd, fdmax, socket_index);
                fdmax = newfd;
              }
              // printf("Master : add \n");
              FD_SET(newfd, & master);
              /*printf("Got new connection from %s on " "socket %d\n", inet_ntop(AF_INET,
                  get_in_addr((struct sockaddr*)&other_server),
                  remoteIP, INET_ADDRSTRLEN),
                  newfd);*/
            }
          } else {
            if (newfd > 0) {
              numReceived = recv(newfd, data, sizeof(data), 0);
              if (numReceived < 0) {
                perror("Error in receiving \n");
              } else {
                data[numReceived] = '\0';
                //If server has received REGISTER command from client
                if ((strstr(data, "REGISTER") != NULL) || (strstr(data, "register") != NULL)) {
                  totalNumberOfServers = registerThisServer(data, server_list, totalNumberOfServers);

                }
                //reset newfd to -1
                newfd = -1;

              }

            } else {
              //Handle data from Client
              //printf("Trying to receive data from client \n");
              int i;
              for (i = 0; i < totalNumberOfServers; i++) {
                if (server_list[i].socket_descriptor == socket_index) {
                  numReceived = recv(server_list[i].socket_descriptor, data, PACKET_SIZE, 0);

                  if (numReceived == -1) {
                    perror("recv");
                    exit(1);
                  } else if (numReceived == 0) {
                    //This means client is disconnected from server. Show appropriate message and do some clean up work here.
                    printf("Server is disconnected from client\n");

                    //Clean UP work
                    close(server_list[i].socket_descriptor);
                    FD_CLR(server_list[i].socket_descriptor, & master);

                    //Update Server IP List
                    server_list[i].socket_descriptor = -1;
                    server_list[i].connected = 0;
                    totalNumberOfServers = deleteServerFromServerIpList(server_list, totalNumberOfServers, i);

                    printf("\nServer list after client disconnection is \n");
                    displayServerIPList(server_list, totalNumberOfServers);

                  } else if (numReceived > 0) {
                    data[numReceived] = '\0';
                    //If server has received SYNC command from client
                    if ((strstr(data, "SYNC") != NULL) || (strstr(data, "SYNC") != NULL)) {
                      //printf("SYNC command received from client \n");
                      //Call function to send sync command to all servers in server_ip_list of server    
                      sendSyncCommandtoClients(data, server_list, totalNumberOfServers);
                    }
                  }

                }

              }
            }

          }
        }

      }
    }
  }
}

void runAsClient(int portNum) {

  int numReceived;
  int peer_connection_id;
  char cmd[100];
  char data[4000];
  int argc_1, lastChar;
  setupThisServer(portNum);
  struct server_ip_list server_list_onClient[5], client_list[5];
  initialize_server_ip_list(server_list_onClient);
  initialize_server_ip_list(client_list);
  int totalNumberOfClients = 1; //Because we are running the client. This must be present in client list
  int totalNumberOfServers = 0;
  int client_id, port;
  int isItForSync = 0;
  while (1) {
    // printf("Starting infinite while loop \n");
    // printf("Copying master to readfds \n");
    readfds = master;
    printf("[PA1]$ ");
    fflush(stdout);
    retVal = select(fdmax + 1, & readfds, NULL, NULL, NULL);
    if (retVal == -1) {
      perror("Error in select: ");
      exit(-1);
    }
    argc_1 = 0;
    isItForSync = 0;
    for (socket_index = 0; socket_index <= fdmax; socket_index++) {
      //printf("Inside the for loop. i: %d and fdmax: %d \n", i, fdmax);
      //printf("Server socket fd is :%d \n", server_socket);
      if (FD_ISSET(socket_index, & readfds)) {
        //printf("FD was set for fd = %d \n", i);
        if (socket_index == FDESC_STDIN) {

          //Ref: From recitation PDF
          fgets(cmd, 100, stdin);
          fflush(stdout);
          char splitCommand[100][100];
          char * arg;
          arg = (char * ) malloc(strlen(cmd) * sizeof(char));
          arg = strtok(cmd, " ");
          while (arg != NULL) {
            //printf( "Arg is %s\n", arg );
            fflush(stdout);
            strcpy(splitCommand[argc_1], arg);
            argc_1 += 1;
            arg = strtok(NULL, " ");
          }
          //Last character of last string has \n
          lastChar = strlen(splitCommand[argc_1 - 1]) - 1;
          splitCommand[argc_1 - 1][lastChar] = '\0';

          //printf("Current command is: %s", splitCommand[0]);
          if (!strcasecmp(splitCommand[0], "HELP")) {
            displayHelpForClient();
          } else
          if (!strcasecmp(splitCommand[0], "CREATOR")) {
            displayCreator();
          } else
          if (!strcasecmp(splitCommand[0], "DISPLAY")) {
            display();
          } else
          if (!strcasecmp(splitCommand[0], "REGISTER")) {
            //Register a new node at the server.
            registerThisClient(splitCommand, client_list, listening_port);
            if (client_list[0].socket_descriptor > fdmax) {
              //Update fdmax is current fd is greater than fdmax
              fdmax = client_list[0].socket_descriptor;
            }
            FD_SET(client_list[0].socket_descriptor, & master);
          } else
          if (!strcasecmp(splitCommand[0], "CONNECT")) {
            peer_connection_id = connectThisClient(splitCommand, client_list, totalNumberOfClients, server_list_onClient, totalNumberOfServers);
            if (peer_connection_id != -1) {
              //Client is connected.
              if (client_list[peer_connection_id].socket_descriptor > fdmax) {
                //Updating fdmax
                fdmax = client_list[peer_connection_id].socket_descriptor;
              }

              FD_SET(client_list[peer_connection_id].socket_descriptor, & master);
              //Increment totalNumberOfClients
              totalNumberOfClients += 1;
            }

          } else
          if (!strcasecmp(splitCommand[0], "GET")) {
            //GET <Connection Number> <File>
            //Get file <File> from client with connection ID as <Connection Number>
            if (!(strcmp(splitCommand[1], "1"))) {
              printf("You can not get file from server \n");
            } else {
              getThisFile(splitCommand, argc_1, client_list, totalNumberOfClients);
            }

          } else
          if (!strcasecmp(splitCommand[0], "PUT")) {

            //PUT <Connection Number> <File Name>
            //Put file to connectt with connection ID as <Connection Number>
            if (!(strcmp(splitCommand[1], "1"))) {
              printf("You can not put file on server \n");
            } else {
              int is_put_success = putThisFile(splitCommand[1], splitCommand[2], client_list, totalNumberOfClients);

              if (is_put_success != 1) {
                printf("\n PUT Error. Can not PUT this file to client. \n");

              }
            }

          } else
          if (!strcasecmp(splitCommand[0], "SYNC")) {
            //it will send SYNC command to server
            //Server identifies it as a sync command and then sync command to every server in server ip list.
            //when client gets sync command from server, they GET everr <hostname>.txt for hostnames present in their server_list_onClient
            //struct server_ip_list newClientList[5];

            int isSuccessSend = sendSyncCommandtoserver(client_list, totalNumberOfClients);

            //
            //SYNC commands will get/put every file from it'c connected client.

          } else
          if (!strcasecmp(splitCommand[0], "LIST")) {
            //List all active connections with a client.
            displayServerIPList(client_list, totalNumberOfClients);
          } else

          if (!strcasecmp(splitCommand[0], "TERMINATE")) {
            //TERMINATE <Connection-ID>
            //Terminates connection with client with given onnnection ID 

            if (!(strcmp(splitCommand[1], "1"))) {
              printf("\nCannot terminate connection with server\n");
            } else {
              int close_sockfd_index = terminateThisClient(splitCommand[1], client_list, totalNumberOfClients);
              if (close_sockfd_index != -1) {
                //Client is successfully terminated. Do some clean up work for terminated client.{
                close(client_list[close_sockfd_index].socket_descriptor);
                FD_CLR(client_list[close_sockfd_index].socket_descriptor, & master);
                //remove from array
                totalNumberOfClients = deleteServerFromServerIpList(client_list, totalNumberOfClients, close_sockfd_index);
              }
            }
          } else

          if (!strcasecmp(splitCommand[0], "QUIT")) {
            //Exit from the program. Bye Bye!!!
            //Do some clean Up work
            //Exit
            int k;
            for (k = 0; k < totalNumberOfClients; k++) {
              close(k);
            }
            printf("\nClosed %d connections \n", totalNumberOfClients - 1);
            exit(0);

          } else {
            printf("Inavlid Command. Type HELP command for valid commands.\n");
            fflush(stdout);
          }
        } else if (socket_index == listener_socket) {
          //Handle New Connection
          //printf("Handling new connection \n");
          int len = sizeof(struct sockaddr_in);
          int temp = socket_index;
          // printf("Before accept: i: %d \n", i);
          newfd = accept(listener_socket, (struct sockaddr * ) & other_server, & len);
          //  printf("After accept: i: %d \n", i);
          socket_index = temp;
          if (newfd == -1) {
            perror("Error in accepting new connection: ");
            exit(-1);
          } else {
            // printf("Master : add \n");
            FD_SET(newfd, & master);
            if (newfd > fdmax) {
              //printf("Current fdmax is %d \n",fdmax);
              //printf("fdmax is changed to %d from %d when i:%d \n", newfd, fdmax, i);
              fdmax = newfd;
            }
            fflush(stdout);

            //Ref: Beej
            socklen_t len;
            struct sockaddr_storage remoteAddr;
            char remote_ip[30];
            len = sizeof(remoteAddr);
            getpeername(newfd, (struct sockaddr * ) & remoteAddr, & len);

            if (remoteAddr.ss_family == AF_INET) {
              struct sockaddr_in * s = (struct sockaddr_in * ) & remoteAddr;
              port = ntohs(s - > sin_port);
              inet_ntop(AF_INET, & s - > sin_addr, remote_ip, sizeof(remote_ip));
            }

            //update client list
            client_list[totalNumberOfClients].connected = 1;
            client_list[totalNumberOfClients].socket_descriptor = newfd;

            strcpy(client_list[totalNumberOfClients].ip_address, remote_ip);
            int i;
            for (i = 0; i < totalNumberOfServers; i++) {
              if (!(strcmp(server_list_onClient[i].ip_address, remote_ip))) {
                strcpy(client_list[totalNumberOfClients].ip_address, remote_ip);
                strcpy(client_list[totalNumberOfClients].hostname, server_list_onClient[i].hostname);
                strcpy(client_list[totalNumberOfClients].port_no, server_list_onClient[i].port_no);
              }
            }

            sprintf(client_list[totalNumberOfClients].id, "%d", (totalNumberOfClients + 1));

            //Increase totalNumberOfClients counter by 1
            totalNumberOfClients += 1;

            printf("Successfully connected \n");
          }
        } else if (client_list[0].socket_descriptor == socket_index) {
          //Handle data from server
          char input_data[PACKET_SIZE];
          totalNumberOfServers = 0;
          if ((numReceived = recv(socket_index, input_data, PACKET_SIZE, 0)) < 0) {
            perror("Error in receiving \n");
          }

          input_data[numReceived] = '\0';
          char endCharacter[2] = ":";
          char partitionCharacter[2] = "|", splitCommand2[100][100];
          //printf("Input data received from server : %s \n", input_data);
          char * arg, * arg2;
          int ctr = 0, arg2_ctr = 0;
          arg = (char * ) malloc(strlen(input_data) * sizeof(char));
          arg = strtok(input_data, endCharacter);
          //printf("\n ARG data in runAsServer is : %s\n", arg);
          if (strcmp(arg, "SYNC")) {
            //Client has received updated server IP list from server
            while (arg != NULL) {
              fflush(stdout);
              strcpy(splitCommand2[arg2_ctr], arg);
              arg2_ctr += 1;
              if ((strcmp(arg, " ") == 0)) {
                break;
              }
              arg = strtok(NULL, endCharacter);
            }
            int i;
            for (i = 0; i < arg2_ctr; i++) {
              arg2 = strtok(splitCommand2[i], partitionCharacter);
              ctr = 0;
              while (arg2 != NULL) {

                if (ctr == 0) {
                  //printf("In ID: %d \n", totalNumberOfServers + 1);
                  sprintf(server_list_onClient[totalNumberOfServers].id, "%d", (totalNumberOfServers + 1));

                }
                if (ctr == 1) {
                  //printf("In Hostname : %s\n", arg2);
                  strcpy(server_list_onClient[totalNumberOfServers].hostname, arg2);

                }
                if (ctr == 2) {
                  //printf("In IP : %s\n", arg2);
                  strcpy(server_list_onClient[totalNumberOfServers].ip_address, arg2);

                }
                if (ctr == 3) {
                  //printf("In port : %s\n", arg2);
                  strcpy(server_list_onClient[totalNumberOfServers].port_no, arg2);

                }

                ctr += 1;
                arg2 = strtok(NULL, partitionCharacter);
              }
              totalNumberOfServers += 1;

            }
            //Display Server IP list on client terminal once it is received from server successfully
            printf("\nServer IP List from server received by client Successfully\n");
            displayServerIPList(server_list_onClient, totalNumberOfServers);
          } else {
            //Client has received SYNC command from server

            //Get every <hostname>.txt file from all clients present in client_list, EXCEPT that from server 
            //printf("\n Client has received SYNC command from server \n");
            int runSync = getAllFilesForSync(client_list, totalNumberOfClients);

          }

        } else {
          //Phew... Now handle data from other connected clients too.
          char peer_send[PACKET_SIZE], source_hostname[100];

          int receive_id;
          int i;
          for (i = 1; i < totalNumberOfClients; i++) {

            if (socket_index == client_list[i].socket_descriptor) {

              memset(peer_send, '\0', PACKET_SIZE);
              numReceived = recv(client_list[i].socket_descriptor, peer_send, PACKET_SIZE, 0);
              receive_id = atoi(client_list[i].id);
              strcpy(source_hostname, client_list[i].hostname);

              if (numReceived == -1) {
                perror("Error in Receiveing \n");
                exit(1);
              } else if (numReceived == 0) {
                printf("Client got disconnected from other client.\n");

                //Clean up few things for that socket.

                //CLose socket
                close(client_list[i].socket_descriptor);
                //Clear master fd set
                FD_CLR(client_list[i].socket_descriptor, & master);
                //Make socket descriptor and connnected status to -1 and 0 respeectively as it was previously before client connection.
                client_list[i].socket_descriptor = -1;
                client_list[i].connected = 0;

                printf("\nNew client list after client disconnetion is\n");
                //delete entry from array
                totalNumberOfClients = deleteServerFromServerIpList(client_list, totalNumberOfClients, i);

                displayServerIPList(client_list, totalNumberOfClients);
              } else {
                //REF: http://stackoverflow.com/questions/12722904/how-to-use-struct-timeval-to-get-the-execution-time at both places
                //This is for receiving file from other clients.

                peer_send[numReceived] = '\0';
                if ((strstr(peer_send, "GET|") != NULL)) {
                  parseGetString(peer_send, client_list[i].id, client_list, totalNumberOfClients);
                }
                if ((strstr(peer_send, "GETSYNC|") != NULL)) {
                  //printf("Client got request for GetSync from id : %s \n",client_list[i].id);
                  isItForSync = 1;
                  parseGetString(peer_send, client_list[i].id, client_list, totalNumberOfClients);
                } else {
                  if (isItForSync == 0) {
                    if ((strstr(peer_send, "filename-") != NULL)) {
                      //start timer
                      printf("\nStarted receiving file\n");
                      gettimeofday( & start_time, NULL);
                      bytes_written = 0;
                      file_size = 0;
                    }

                    int ret_val = receiveThisFile(peer_send, file_name, & file_size, & bytes_written, numReceived);
                    if (ret_val != 1) {
                      //printf("\nError in receiving the file from client \n");
                    }

                    if (bytes_written != 0 && bytes_written == file_size) {
                      gettimeofday( & end_time, NULL);
                      //Calculate Tx and Rx.
                      //Ref:http://stackoverflow.com/questions/2150291/how-do-i-measure-a-time-interval-in-c
                      long diff = ((end_time.tv_sec - start_time.tv_sec) * 1000000) + (end_time.tv_usec - start_time.tv_usec);
                      long double time_taken = (long double) diff / 1000000;
                      long double rate = (long double)(file_size * 8) / time_taken;

                      printf("\nFile %s is successfully downloaded from %s \n", file_name, client_list[i].hostname);
                      printf("\n Receiving file operation details \n");

                      printf("\n Rx: %s -> %s \n File Size :%d bytes \n Time Taken: %Lf seconds \n Rx Rate:%Lf bits/second\n", client_list[i].hostname, server_host_name, file_size, time_taken, rate);

                    }
                  } else {
                    //Ref:http://www.cplusplus.com/reference/ctime/localtime/

                    time_t rawtime;
                    struct tm * timeinfo;

                    time( & rawtime);
                    timeinfo = localtime( & rawtime);
                    printf("Start %s at %s", file_name, asctime(timeinfo));
                    //gettimeofday(&start_time, NULL);
                    bytes_written = 0;
                    file_size = 0;

                    int ret_val = receiveThisFile(peer_send, file_name, & file_size, & bytes_written, numReceived);
                    if (ret_val != 1) {
                      //printf("\nError in receiving the file from client \n");
                    }

                    if (bytes_written != 0 && bytes_written == file_size) {
                      time( & rawtime);
                      timeinfo = localtime( & rawtime);
                      printf("End %s at %s", file_name, asctime(timeinfo));
                      //gettimeofday(&end_time, NULL);
                      //Calculate Tx and Rx.
                      //Ref:http://stackoverflow.com/questions/2150291/how-do-i-measure-a-time-interval-in-c
                      /*long diff=((end_time.tv_sec - start_time.tv_sec)*1000000) +(end_time.tv_usec - start_time.tv_usec);
    										long double time_taken=(long double)diff/1000000;
    										long double rate=(long double)(file_size*8)/time_taken;
    
    										printf("\nFile %s is successfully downloaded from %s \n",file_name,client_list[i].hostname);
    										printf("\n Receiving file operation details \n");
    							
    										printf("\n Rx: %s -> %s \n File Size :%d bytes \n Time Taken: %Lf seconds \n Rx Rate:%Lf bits/second\n", client_list[i].hostname,server_host_name,file_size,time_taken,rate);
    */

                    }
                    isItForSync = 0;
                  }

                }
              }
            }

          }
        }
      }

    }

  }
}
int main(int argc, char * * argv) {
  int flag = 0;
  server_ip_address = get_my_ip();
  find_my_hostname();

  //printf("My hostname is :%s \n",server_host_name);
  //printf("My IP is :%s \n",server_ip_address);
  // printf("My IP is :%s \n",get_my_ip());
  //initiall use -help option to how to run program. This is different from client and server related HELP option.
  if (argc == 2 && !strcasecmp(argv[1], "-help")) {
    printf("Run as: ./<your file> <type of server> <port number> \n");
    printf("<your file> : object file \n");
    printf("<type of server : s = for server, c = for client \n");
    printf("<port number> : Port number on which you want your server/client to run \n");
    flag = 1;
  } else if (argc != 3) {
    printf("Inavalid number of arguments. Run as ./assignment1 -help for valid options \n");
    return 0;
  }

  if (!strcasecmp(argv[1], "s")) {
    portNum = atoi(argv[2]);
    strcpy(listening_port, argv[2]);
    /* Behaving as Server running on port portNum */
    runAsServer(portNum);
  } else if (!strcasecmp(argv[1], "c")) {
    portNum = atoi(argv[2]);
    strcpy(listening_port, argv[2]);
    /* Behaving as Client running on port portNum */
    runAsClient(portNum);
  } else if (!flag) {
    printf("Not valid run command. Run as ./assignment1 -help for valid options \n");
  }

  return 0;

}