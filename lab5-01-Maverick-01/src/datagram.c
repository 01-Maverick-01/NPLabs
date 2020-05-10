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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/utility.h"
#include "../include/datagram.h"

#define INDEX_VERSION 0
#define INDEX_CMD 1
#define INDEX_RESPONSE 2
#define INDEX_MOVE 3
#define INDEX_TURN 4
#define INDEX_GAME_ID 5

/* C language requires that you predefine all the routines you are writing */
void getPortAndIP(char *dest, struct sockaddr_in addr);

//////////////////////////////////////////////////////
/// Call this method to create a new datagram socket.
//////////////////////////////////////////////////////
int createSocket()
{
    printf("Creating socket. Please wait...\n");
    int dgram_socket = socket(AF_INET, SOCK_DGRAM, 0);
    validateReturnCode(dgram_socket, "Error occurred while creating socket", -1);
    return dgram_socket;
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
/// This method is used to validate the return codes.
//////////////////////////////////////////////////////
void validateReturnCode(int rc, char* msg, int socket_desc)
{
    if (rc < 0)                                            // validate if return code is valid
        perror(msg);
}

//////////////////////////////////////////////////////////////////////////
/// Call this method to get remote player response using DATAGRAM Socket.
//////////////////////////////////////////////////////////////////////////
struct recvd_packet_info getRemotePlayerRequest(int socket_desc)
{
    uint8_t request_payload[MSG_SIZE] = {0};
    struct recvd_packet_info recvdRequest;
    socklen_t size = sizeof(recvdRequest.remoteAddr);
    int rc = recvfrom(socket_desc, &request_payload, MSG_SIZE, MSG_WAITALL, (struct sockaddr *) &recvdRequest.remoteAddr, &size);
    validateReturnCode(rc, "Error occurred while receiving remove player response. Reason", socket_desc);

    recvdRequest.numBytesRecvd = rc;
    getPortAndIP(recvdRequest.addr_str, recvdRequest.remoteAddr);

    recvdRequest.recvdMsg.version = request_payload[0];
    recvdRequest.recvdMsg.cmdCode = request_payload[1];
    recvdRequest.recvdMsg.responseCode = request_payload[2];
    recvdRequest.recvdMsg.move = request_payload[3];
    recvdRequest.recvdMsg.turnNumber = request_payload[4];
    recvdRequest.recvdMsg.gameId = request_payload[5];

    printSendRecvMsg("RECVD_MSG (%d bytes): Remote player (%s) --> '%03d:%03d:%03d:%03d:%03d:%03d'", rc, recvdRequest.addr_str, recvdRequest.recvdMsg.version, 
        recvdRequest.recvdMsg.cmdCode, recvdRequest.recvdMsg.responseCode, recvdRequest.recvdMsg.move,
        recvdRequest.recvdMsg.turnNumber, recvdRequest.recvdMsg.gameId);
    
    return recvdRequest;
}

//////////////////////////////////////////////////////////////////////////
/// Call this method to send msg to remote player using DATAGRAM Socket.
//////////////////////////////////////////////////////////////////////////
int sendResponseToRemotePlayer(int version, int move, int cmd, int turn, int returnCode, int gameId,
    int socket_desc, struct sockaddr_in remoteAddr)
{
    uint8_t msgArray[MSG_SIZE];
    msgArray[INDEX_VERSION] = (uint8_t) version;
    msgArray[INDEX_CMD] = (uint8_t) cmd;
    msgArray[INDEX_RESPONSE] = (uint8_t) returnCode;
    msgArray[INDEX_MOVE] = (uint8_t) move;
    msgArray[INDEX_TURN] = (uint8_t) turn;
    msgArray[INDEX_GAME_ID] = (uint8_t) gameId;

    int rc = sendto(socket_desc, (const uint8_t *)&msgArray, MSG_SIZE, MSG_CONFIRM, 
        (const struct sockaddr *) &remoteAddr, sizeof(remoteAddr));
    validateReturnCode(rc, "Error occurred while sending request to remove player. Reason", socket_desc);
    
    char addr[20];
    getPortAndIP(addr, remoteAddr);
    printSendRecvMsg("SENT_MSG  (%d bytes): Remote player (%s) --> '%03d:%03d:%03d:%03d:%03d:%03d'", rc, addr, msgArray[INDEX_VERSION], msgArray[INDEX_CMD],
        msgArray[INDEX_RESPONSE], msgArray[INDEX_MOVE], msgArray[INDEX_TURN], msgArray[INDEX_GAME_ID]);
    
    return rc;
}

///////////////////////////////////////////////////////////////////
/// Call this method to get remote player ip and port as a string.
///////////////////////////////////////////////////////////////////
void getPortAndIP(char *dest, struct sockaddr_in addr)
{
    sprintf(dest, "%s:%d", inet_ntoa(addr.sin_addr), htons(addr.sin_port));
}

void closeSocket(int socket)
{
    printf("Terminating game server. Please wait...\n");
    close(socket);
    printf("Game server shutdown completed.\n");
}