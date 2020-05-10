/****************************************************************************************/
/* CSE 5462 - Lab 5                                                                     */
/*      This program provides defintion for various network related methods used by the */
/*      tictactoe server implementation.                                                */ 
/*                                                                                      */
/* Name: Ishan Deep                                                                     */
/* Lname.#: deep.24                                                                     */
/*                                                                                      */
/* Created on Thu Feb 25 2020                                                           */
/*                                                                                      */
/****************************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/utility.h"
#include "../include/datagram.h"

#define ERROR -1

#define INDEX_VERSION 0
#define INDEX_CMD 1
#define INDEX_RESPONSE 2
#define INDEX_MOVE 3
#define INDEX_TURN 4
#define INDEX_GAME_ID 5

/* C language requires that you predefine all the routines you are writing */
void        getPortAndIP(char *dest, struct sockaddr_in addr);
int         createDgramSocket();
int         bindToSocket(int socket_desc, int port, struct sockaddr_in *serverAddr);
void        validateReturnCode(int rc, char* msg, int socket_desc);
void        readFromSocket(int socket_desc, struct recvd_packet_info *rcvdData);

int createUnicastSocket(int port, struct sockaddr_in *serverAddr)
{
    int unicastSocket = createDgramSocket();
    if (unicastSocket < 0)
    {
        closeSocket(unicastSocket);
        return ERROR;
    }

    if (bindToSocket(unicastSocket, port, serverAddr))
    {
        closeSocket(unicastSocket);
        return ERROR;
    }
    
    return unicastSocket;
}

int createMulticastSocket(char *ipAddr, int port)
{
    struct sockaddr_in mcast_group;
    struct ip_mreq mreq;

    memset(&mcast_group, 0, sizeof(mcast_group));
    mcast_group.sin_family = AF_INET;
    mcast_group.sin_port = htons(port);
    mcast_group.sin_addr.s_addr = inet_addr(ipAddr);

    int multicastSocket = createDgramSocket();
    if (multicastSocket < 0)
    {
        closeSocket(multicastSocket);
        return ERROR;
    }

    int rc = bind(multicastSocket, (struct sockaddr *)&mcast_group, sizeof(mcast_group));
    validateReturnCode(rc, "Error occurred while binding to multicast socket", multicastSocket);
    if (rc < 0)
    {
        closeSocket(multicastSocket);
        return ERROR;
    }

    /* Preparatios for using Multicast */
    mreq.imr_multiaddr = mcast_group.sin_addr;
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    /* Tell the kernel we want to join that multicast group. */
    rc = setsockopt(multicastSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    validateReturnCode(rc, "Error occurred while adding membershup to multicast socket", multicastSocket);
    if (rc < 0)
    {
        closeSocket(multicastSocket);
        return ERROR;
    }
    
    return multicastSocket;
}


//////////////////////////////////////////////////////////////////////////
/// Call this method to get remote player response using DATAGRAM Socket.
//////////////////////////////////////////////////////////////////////////
struct recvd_packet_info getRemotePlayerRequest(int unicastSocket, int multicastSocket, int timeoutInSecs)
{
    fd_set set;
    FD_ZERO(&set);
    int maxDescriptor = (unicastSocket > multicastSocket) ? unicastSocket + 1 : multicastSocket + 1;
    FD_SET(unicastSocket, &set);
    FD_SET(multicastSocket, &set);
    struct timeval timeout = { timeoutInSecs, 0 }; //set timeout for 'x' seconds
    int rc = select(maxDescriptor, &set, NULL, NULL, &timeout);
    validateReturnCode(rc, "Error occurred while receiving remove player response. Reason", -1);
    
    struct recvd_packet_info recvdRequest;
    recvdRequest.timedOut = 0;
    
    if (timeout.tv_sec == 0 && timeout.tv_usec == 0)
        recvdRequest.timedOut = 1;
    else if (FD_ISSET(multicastSocket, &set))
        readFromSocket(multicastSocket, &recvdRequest);
    else if (FD_ISSET(unicastSocket, &set))
        readFromSocket(unicastSocket, &recvdRequest);

    return recvdRequest;
}

void readFromSocket(int socket_desc, struct recvd_packet_info *rcvdData)
{
    uint8_t request_payload[RECV_MSG_SIZE] = {0};
    socklen_t size = sizeof(rcvdData->remoteAddr);
    int rc = recvfrom(socket_desc, &request_payload, RECV_MSG_SIZE, MSG_WAITALL, (struct sockaddr *) &rcvdData->remoteAddr, &size);
    validateReturnCode(rc, "Error occurred while receiving remove player response. Reason", -1);

    rcvdData->numBytesRecvd = rc;
    getPortAndIP(rcvdData->addr_str, rcvdData->remoteAddr);

    rcvdData->recvdMsg.version = request_payload[0];
    rcvdData->recvdMsg.cmdCode = request_payload[1];
    rcvdData->recvdMsg.responseCode = request_payload[2];
    rcvdData->recvdMsg.move = request_payload[3];
    rcvdData->recvdMsg.turnNumber = request_payload[4];
    rcvdData->recvdMsg.gameId = request_payload[5];
    
    if (rc == RECV_MSG_SIZE)
    {
        int k = 6;
        for (int i = 0; i < ROWS; i++)
        {
            for (int j = 0; j < COLUMNS; j++)
                rcvdData->recvdMsg.board[i][j] = request_payload[k++];
        }
    }

    printSendRecvMsg("RECVD_MSG (%d bytes): Remote player (%s) --> '%03d:%03d:%03d:%03d:%03d:%03d'", rc, rcvdData->addr_str,
        rcvdData->recvdMsg.version, rcvdData->recvdMsg.cmdCode, rcvdData->recvdMsg.responseCode,
        rcvdData->recvdMsg.move, rcvdData->recvdMsg.turnNumber, rcvdData->recvdMsg.gameId);
}

//////////////////////////////////////////////////////////////////////////
/// Call this method to send msg to remote player using DATAGRAM Socket.
//////////////////////////////////////////////////////////////////////////
int sendResponseToRemotePlayer(int version, int move, int cmd, int turn, int returnCode, int gameId,
    int socket_desc, struct sockaddr_in remoteAddr)
{
    uint8_t msgArray[SEND_MSG_SIZE];
    msgArray[INDEX_VERSION] = (uint8_t) version;
    msgArray[INDEX_CMD] = (uint8_t) cmd;
    msgArray[INDEX_RESPONSE] = (uint8_t) returnCode;
    msgArray[INDEX_MOVE] = (uint8_t) move;
    msgArray[INDEX_TURN] = (uint8_t) turn;
    msgArray[INDEX_GAME_ID] = (uint8_t) gameId;

    int rc = sendto(socket_desc, (const uint8_t *)&msgArray, SEND_MSG_SIZE, MSG_CONFIRM, 
        (const struct sockaddr *) &remoteAddr, sizeof(remoteAddr));
    validateReturnCode(rc, "Error occurred while sending request to remove player. Reason", socket_desc);
    
    char addr[20];
    getPortAndIP(addr, remoteAddr);
    printSendRecvMsg("SENT_MSG  (%d bytes) : Remote player (%s) --> '%03d:%03d:%03d:%03d:%03d:%03d'", rc, addr, msgArray[INDEX_VERSION], msgArray[INDEX_CMD],
        msgArray[INDEX_RESPONSE], msgArray[INDEX_MOVE], msgArray[INDEX_TURN], msgArray[INDEX_GAME_ID]);
    
    return rc;
}

void closeSocket(int socket)
{
    if (socket >= 0)
    {
        printf("Terminating game server. Please wait...\n");
        close(socket);
        printf("Game server shutdown completed.\n");
    }
}

//////////////////////////////////////////////////////
/// Call this method to bind to a datagram socket.
//////////////////////////////////////////////////////
int bindToSocket(int socket_desc, int port, struct sockaddr_in *serverAddr)
{
    printf("Binding to socket. Please wait...\n");
    // Filling server information 
    serverAddr->sin_family    = AF_INET; // IPv4 
    serverAddr->sin_addr.s_addr = INADDR_ANY; 
    serverAddr->sin_port = htons(port); 
      
    // Bind the socket with the server address 
    int rc = bind(socket_desc, (const struct sockaddr *)serverAddr, sizeof(*serverAddr));
    validateReturnCode(rc, "Bind failed", socket_desc);
    return rc;
}

//////////////////////////////////////////////////////
/// Call this method to create a new datagram socket.
//////////////////////////////////////////////////////
int createDgramSocket()
{
    printf("Creating socket. Please wait...\n");
    int dgram_socket = socket(AF_INET, SOCK_DGRAM, 0);
    validateReturnCode(dgram_socket, "Error occurred while creating socket", -1);
    return dgram_socket;
}

//////////////////////////////////////////////////////
/// This method is used to validate the return codes.
///     in case of error, the socket is closed.
//////////////////////////////////////////////////////
void validateReturnCode(int rc, char* msg, int socket_desc)
{
    if (rc < 0)                         // validate if return code is valid
    {
        perror(msg);
        closeSocket(socket_desc);
    }
}

///////////////////////////////////////////////////////////////////
/// Call this method to get remote player ip and port as a string.
///////////////////////////////////////////////////////////////////
void getPortAndIP(char *dest, struct sockaddr_in addr)
{
    sprintf(dest, "%s:%d", inet_ntoa(addr.sin_addr), htons(addr.sin_port));
}