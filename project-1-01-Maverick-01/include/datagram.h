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
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define RECV_MSG_SIZE 15
#define SEND_MSG_SIZE 6

// struct resembling to protocol msg structure
struct msg
{
    uint8_t version;
    uint8_t cmdCode;
    uint8_t responseCode;
    uint8_t move;
    uint8_t turnNumber;
    uint8_t gameId;
    char board[ROWS][COLUMNS];
};

struct recvd_packet_info
{
    struct sockaddr_in remoteAddr;
    struct msg recvdMsg;
    int numBytesRecvd;
    int timedOut;
    char addr_str[50];
};


int                         createUnicastSocket(int port, struct sockaddr_in *serverAddr);
int                         createMulticastSocket(char *ipAddr, int port);
struct recvd_packet_info    getRemotePlayerRequest(int unicastSocket, int multicastSocket, int timeoutInSecs);
void                        closeSocket(int socket);
int                         lastRecvTimedOut(int numBytesRecvd);
int                         sendResponseToRemotePlayer(int version, int move, int cmd, int turn, int returnCode, int gameId,
                                int socket_desc, struct sockaddr_in remoteAddr);

#endif