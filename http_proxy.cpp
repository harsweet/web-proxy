// Include files for C And C++ socket programming and stuff 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <iostream>             // For Input-Output Operations 
#include <sys/socket.h> 
#include <arpa/inet.h>	
#include <unistd.h>
#include <netdb.h>
#include <time.h>            // Used for seeding srand


using namespace std;       // To make the code much simpler and avoid writing std again and again


#define MAX_MESSAGE_LENGTH 6000
#define MYPORTNUM 50000
#define NO 0
#define YES 1

// Buffer sizes to deal with the main server
#define SERVER_BUFFER_SIZE 1000
#define BIG_BUFFER_SIZE 200000



int childsockfd;

//This is a signal handler to do graceful exit if needed 
void catcher(int sig)
{
	close(childsockfd);
	exit(0);
}



// Method that takes in a character array and replaces all the Occurences of tobeReplaced with replacement
int smart_replace(char *dest, const char *old_string, const char *new_string, const int MAX_BUF_SIZE_REPLACE)
{
	char *old_string_ptr = dest; // initially
	char temporary_buffer[MAX_BUF_SIZE_REPLACE];

	int occurrence_found = -1; // default

	int skip_old_string = strlen(old_string);
	while (1)
	{
		old_string_ptr = strstr(old_string_ptr, old_string);
		if (!old_string_ptr)
		{
			break;
		}
		occurrence_found = 0;
		bzero(temporary_buffer, MAX_BUF_SIZE_REPLACE);
		// FIRST PART addition: part before finding old string
		int amount_first_part = old_string_ptr - dest;
		bcopy(dest, temporary_buffer, amount_first_part);
		// SECOND PART addition: substitute the old stirng the new one
		strcat(temporary_buffer, new_string); // append new substitued substring
		// THIRD PART addition: append the remaining original string
		strcat(temporary_buffer, old_string_ptr + skip_old_string); // append the remaining withot affecting anything else
		bcopy(temporary_buffer, dest, MAX_BUF_SIZE_REPLACE);
		old_string_ptr = dest + amount_first_part + skip_old_string; // MAKE SURE THAT WE SKIP though the previous last replaced substring
	}

	return occurrence_found;
}














// Some the additional methods that I have added
// Used for extracting the string containing the host name from a bigger string containing the request
string hostExtracter(string request)
{
	int host_position = request.find("Host:");
	int new_start = host_position + 6;  // Based on the difference between word "Host" and hostname starting

	int r_length = request.length();

	string short_request = request.substr(new_start, r_length);    // Removing the portion before the host name

	int host_end = short_request.find("\n");   // Getting the end of hostname


	// Then also removing the portion from both the front and the back to return the hostname
	string the_host = request.substr(new_start, host_end - 1);   

	return the_host;
}

// Main program for server 
int main()
{

	string replaced_str, replacement_str;

	cout << endl << endl;

	cout << "Enter the Word to be replaced: " ;
	getline(cin, replaced_str);
	

	cout << endl;
	

	cout << "Enter the Replacement: " ;
	getline(cin, replacement_str);

	
	
	

	static struct sigaction act;
	char messagein[MAX_MESSAGE_LENGTH];            // DECLARATION's
	char messageout[MAX_MESSAGE_LENGTH];
	int parentsockfd;
	int i, len, bytes, answer;
	pid_t pid;

	int socket_desc, childsockfd;             
	struct sockaddr_in server, client;                // DECLARATION's
	char client_message[MAX_MESSAGE_LENGTH];

	string the_host;                                         // DECLARATION's


	// Set up a signal handler to catch some weird termination conditions. 
	act.sa_handler = catcher;
	sigfillset(&(act.sa_mask));
	sigaction(SIGPIPE, &act, NULL);

	// Initialize server sockaddr structure 
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(MYPORTNUM);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	// create a transport-level endpoint using TCP 
	parentsockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (parentsockfd == -1)
	{
		fprintf(stderr, "Client-side socket() call failed!\n");
		exit(1);
	}

	// bind a specific address and port to the end point 
	if (bind(parentsockfd, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) == -1)
	{
		fprintf(stderr, "Client-side bind() call failed!\n");
		exit(1);
	}

	// start listening for incoming connections from clients
	if (listen(parentsockfd, 5) == -1)
	{
		fprintf(stderr, "Client-side listen() call failed!\n");
		exit(1);
	}

	/* initialize message strings just to be safe (null-terminated) */
	bzero(messagein, MAX_MESSAGE_LENGTH);
	bzero(messageout, MAX_MESSAGE_LENGTH);
	bzero(client_message, MAX_MESSAGE_LENGTH);

	fprintf(stderr, "Hello there! I am a Proxy!!\n");
	fprintf(stderr, "I am running on TCP port %d for you...\n\n", MYPORTNUM);

	// Modified the code covered in the tutorials to now connect to the server using the host name
	// Code inspired IBM C++ Docs

	// Main loop: server loops forever listening for requests 
	for (;;)
	{
		// accept a connection 
		childsockfd = accept(parentsockfd, NULL, NULL);
		if (childsockfd == -1)
		{
			fprintf(stderr, "client side accept() call failed!\n");
			exit(1);
		}

		// try to create a child process to deal with this new client 

		// This makes it possible for us to open multiple tabs and the proxy still works 
		pid = fork();

		// use process id (pid) returned by fork to decide what to do next 
		if (pid < 0)
		{
			fprintf(stderr, "client-side fork() call failed! OMG!!\n");
			exit(1);
		}
		else if (pid == 0)
		{
			// this is the child process doing this part 

			// don't need the parent listener socket that was inherited 
			close(parentsockfd);


			//Receive a message from client
			int recvStatus;
			while (1)
			{
				recvStatus = recv(childsockfd, client_message, 5000, 0);
				if (recvStatus == -1)
				{
					
					break;
				}

				printf("Client says: %s\n", client_message);

				// Getting the host
				the_host = hostExtracter(client_message);

				// Some Initialisations related to talking to the main server

				int main_server_sock;
				struct sockaddr_in main_server;
				char message_to_server[1000], server_reply[SERVER_BUFFER_SIZE];
				struct hostent *hp;

				char buffer[BIG_BUFFER_SIZE];

				bzero(buffer, BIG_BUFFER_SIZE);
				bzero(message_to_server, SERVER_BUFFER_SIZE);
				bzero(server_reply, SERVER_BUFFER_SIZE);

				//Create socket to talk to the main server
				main_server_sock = socket(AF_INET, SOCK_STREAM, 0);
				if (main_server_sock == -1)
				{
					printf("Could not create socket to connect with the server");
				}
				puts("Server Socket created");


				// Now comes connecting to the host server
				hp = gethostbyname(the_host.c_str());

				memcpy(&(main_server.sin_addr.s_addr), hp->h_addr, hp->h_length);
				main_server.sin_family = hp->h_addrtype;
				main_server.sin_port = htons(80);

				// Connect to remote server
				if (connect(main_server_sock, (struct sockaddr *)&main_server, sizeof(main_server)) < 0)
				{
					perror("Connection to Main server failed. Error");
					return 1;
				}

				printf("Connected to the Main Server\n");

				// After connecting to the server, passing on the client message to the main server
				strcpy(message_to_server, client_message);

				if (send(main_server_sock, message_to_server, strlen(message_to_server), 0) < 0)
				{
					puts("Message Relay to the Main Server failed");
					return 1;
				}

				printf("Message Relayed to the Main Server \n \n");

				// Now comes the receiving part from the main server

#define THRESHOLD 3000
				int net_server_reply_len = 0;
				char *temp_pointer = buffer;
				while (1)
				{

					// I made this condition to relay the content recieved from the main server 
					// After modifying to the client
					// Have a THRESHODLD so that the buffer is not overwhelmed
					if (net_server_reply_len > THRESHOLD)  
					{
						smart_replace(buffer, replaced_str.c_str(), replacement_str.c_str(), BIG_BUFFER_SIZE);            // Making the modifications
						

						send(childsockfd, buffer, net_server_reply_len, 0); // Realaying the content to the client
						net_server_reply_len = 0;
						bzero(buffer, BIG_BUFFER_SIZE);
						temp_pointer = buffer; // start writing from starting again!
					}


					int server_reply_len = recv(main_server_sock, temp_pointer, 100, 0);  // Receiving data from the main servr

					//Receive a reply from the server
					if (server_reply_len < 0)
					{
						puts("Main server recv failed");
						break;
					}
					else if (server_reply_len > 0)     // Else updating the buffer
					{
						puts("Server temp reply :");
						puts(temp_pointer);
						temp_pointer = temp_pointer + server_reply_len;
						net_server_reply_len += server_reply_len;
					}
					else // For the case when everthing has been received and the main-server has nothing more to sen
					{	
                        // Making the modifications
						smart_replace(buffer, replaced_str.c_str(), replacement_str.c_str(), BIG_BUFFER_SIZE);            // Making the modifications            // Making the modifications
						
						// Sending to the client 
						send(childsockfd, buffer, net_server_reply_len, 0);
						net_server_reply_len = 0;
						bzero(buffer, BIG_BUFFER_SIZE);
						temp_pointer = buffer; 

						printf("EVERYTHING DONE \n");
						break;
					}
				}
				
				// Closing the connection with the main server
				close(main_server_sock);
			}

			// PROXYCODE ENDS HERE

			exit(0);
		} // end of then part for child
		else
		{
			// the parent process is the one doing this part 
			fprintf(stderr, "Server created child process %d to handle that client\n", pid);
			fprintf(stderr, "Main server process going back to listening for new clients now...\n\n");

			// parent doesn't need the childsockfd 
			// Closing the socket made for a specific tab
			close(childsockfd);
		}
	}
}
