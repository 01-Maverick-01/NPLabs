/****************************************************************************************/
/* CSE 5462 - Lab 3 ****DO NOT USE THIS FILE****                                        */
/*      This program implements the server of tictactoe game and uses TCP Stream socket */
/*      to communicate with the server. The client and server play against each other   */
/*      and server is always player 1 and client is player 2.                           */
/*                                                                                      */
/****THIS CODE IS NOT COMPLETE AND WAS USED BY ME TO TEST BASIC CLIENT IMPLEMENTATION****/
/*                                                                                      */
/* Name: Ishan Deep                                                                     */
/* Lname.#: deep.24                                                                     */
/*                                                                                      */
/* Created on Thu Jan 30 2020                                                           */
/*                                                                                      */
/****************************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MY_PORT		9999
#define MAXBUF		1024

struct msg
{
    uint8_t version;
    uint8_t responseCode;
    uint8_t move;
    uint8_t turnNumber;
};

uint8_t recvByteResponseAsInteger(int socket_desc)  // call this method to read a 1 byte int response from remote player
{
    uint8_t response;
    while (recv(socket_desc, &response, 1, MSG_PEEK) < 1);  // wait unit 1 bytes is present in stream
    int rc = recv(socket_desc, &response, 1, 0);
    if (rc < 0)
        perror("getFrom");
    return response;
}

struct msg getRemotePlayerResponse(int socket_desc, uint8_t turn)
{
    struct msg remotePlayerResponse;    
    remotePlayerResponse.version = recvByteResponseAsInteger(socket_desc);
    remotePlayerResponse.responseCode = recvByteResponseAsInteger(socket_desc);
    remotePlayerResponse.move = recvByteResponseAsInteger(socket_desc);
    remotePlayerResponse.turnNumber = recvByteResponseAsInteger(socket_desc);
    return remotePlayerResponse;
}

void sendByteRequestAsInteger(uint8_t number, int socket_desc)  // Call this method to tranfer a 1 byte number over socket
{
    int rc = send(socket_desc, &number, sizeof(number), 0);
    if (rc < 0)
        perror("sendto");
}

void sendRequestToServer(uint8_t msg_code, uint8_t turn, uint8_t move, int socket_desc)
{
    sendByteRequestAsInteger((uint8_t)0, socket_desc);
    sendByteRequestAsInteger((uint8_t)msg_code, socket_desc);
    sendByteRequestAsInteger((uint8_t)move, socket_desc);
    sendByteRequestAsInteger((uint8_t)turn, socket_desc);
}

int main(int Count, char *Strings[])
{   int sockfd;
	struct sockaddr_in self;

	/*---Create streaming socket---*/
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		perror("Socket");
		exit(0);
	}

	/*---Initialize address/port structure---*/
	bzero(&self, sizeof(self));
	self.sin_family = AF_INET;
	self.sin_port = htons(MY_PORT);
	self.sin_addr.s_addr = INADDR_ANY;

	/*---Assign a port number to the socket---*/
    if ( bind(sockfd, (struct sockaddr*)&self, sizeof(self)) != 0 )
	{
		perror("socket--bind");
		exit(0);
	}

	/*---Make it a "listening socket"---*/
	if ( listen(sockfd, 20) != 0 )
	{
		perror("socket--listen");
		exit(0);
	}

	/*---Forever... ---*/
	while (1)
	{	int clientfd;
		struct sockaddr_in client_addr;
		socklen_t addrlen=sizeof(client_addr);

		/*---accept a connection (creating a data pipe)---*/
		clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
		printf("%s:%d connected\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		/*---Echo back anything sent---*/
		//send(clientfd, buffer, recv(clientfd, buffer, MAXBUF, 0), 0);
        for (int i=0; i<9;i++)
        {
            if (i%2==0)
            {
                int choice;
                printf("Enter your move:");
                scanf("%d", &choice);
                sendRequestToServer(0, i, (uint8_t)choice, clientfd);
            }
            else
            {
                printf("Move=%d", getRemotePlayerResponse(clientfd, i).move);
            }
            
        }
        
		/*---Close data connection---*/
		close(clientfd);
	}

	/*---Clean up (should never get here!)---*/
	close(sockfd);
	return 0;
}