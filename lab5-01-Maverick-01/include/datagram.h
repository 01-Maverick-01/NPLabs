/****************************************************************************************/
/* CSE 5462 - Lab 5                                                                     */
/*      This file is the header for various netwrok related methods that are used for   */
/*      tictactoe server implementation.                                                */
/*                                                                                      */
/* Name: Ishan Deep                                                                     */
/* Lname.#: deep.24                                                                     */
/*                                                                                      */
/* Created on Thu Feb 25 2020                                                           */
/*                                                                                      */
/****************************************************************************************/

#ifndef _datagram_h
#define _datagram_h

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MSG_SIZE 6

// struct resembling to protocol msg structure
struct msg
{
    uint8_t version;
    uint8_t cmdCode;
    uint8_t responseCode;
    uint8_t move;
    uint8_t turnNumber;
    uint8_t gameId;
};

struct recvd_packet_info
{
    struct sockaddr_in remoteAddr;
    struct msg recvdMsg;
    int numBytesRecvd;
    char addr_str[50];
};


int                         createSocket();
int                         bindToSocket(int socket_desc, int port, struct sockaddr_in *serverAddr);
struct recvd_packet_info    getRemotePlayerRequest(int socket_desc);
void                        validateReturnCode(int rc, char* msg, int socket_desc);
void                        closeSocket(int socket);
int                         sendResponseToRemotePlayer(int version, int move, int cmd, int turn, int returnCode, int gameId,
                                int socket_desc, struct sockaddr_in remoteAddr);

#endif