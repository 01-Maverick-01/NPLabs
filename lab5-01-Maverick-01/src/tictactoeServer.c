/****************************************************************************************/
/* CSE 5462 - Lab 5                                                                     */
/*      This program implements server end of tictactoe game and uses Datagram socket   */
/*      to communicate with the client. The client and server play against each other   */
/*      and server is always player 1 and client is player 2.                           */
/*      This version of the server can handle multiple clients at the same time.        */
/*                                                                                      */
/* Name: Ishan Deep                                                                     */
/* Lname.#: deep.24                                                                     */
/*                                                                                      */
/* Created on Thu Feb 25 2020                                                           */
/*                                                                                      */
/****************************************************************************************/

/* include files go here */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../include/utility.h"
#include "../include/datagram.h"

/********************************** #define section *************************************/
#define MAX_VERSION 2
#define MIN_VERSION 1
#define MAX_GAME 256
// cmd line arg indexes
#define PORT 1
#define BOT_LVL 2
// cmd code
#define CMD_CODE_MOVE 0
#define CMD_CODE_NEW_GAME 1
// response codes
#define RES_CODE_OK 0
#define RES_CODE_INVALID_MOVE 1
#define RES_CODE_NO_SYNC 2
#define RES_CODE_INVALID_REQ 3
#define RES_CODE_GAME_OVER 4
#define RES_CODE_GAME_OVER_ACK 5
#define RES_CODE_VERSION_MISMATCH 6
#define RES_CODE_SERVER_BUSY 7
#define RES_CODE_GAME_ID_MISMATCH 8
/****************************************************************************************/

extern int getBotMove(char board[ROWS][COLUMNS], uint8_t turn, char mark, int botLevel);

enum GameState{
    WaitingForPlayer,
    Started,
    Completed,
    Error
};

struct GameData
{
    int id;
    int turn;
    int version;
    enum GameState state;
    char board[ROWS][COLUMNS];
    time_t lastRequestTime;
    struct sockaddr_in clientAddr;
};

struct ServerData
{
    struct GameData allGames[MAX_GAME];
    char mark;
    int botLvl;
    int socket;
    int port;
    struct sockaddr_in serverAddr;
};

/* C language requires that you predefine all the routines you are writing */
struct ServerData   validateInput(int argc, char** argv);
void                handlePlayerRequest(struct recvd_packet_info playerRequest, struct ServerData *serverData);
void                startNewGame(struct recvd_packet_info playerRequest, struct ServerData *serverData);
void                continueExistingGame(struct recvd_packet_info playerRequest, struct ServerData *serverData);
int                 getNextFreeGameSlot(struct ServerData serverData, struct recvd_packet_info playerRequest);
int                 validateVersionResCodeAndMsgSize(struct recvd_packet_info playerRequest, struct ServerData *serverData);
int                 checkwin(char board[ROWS][COLUMNS]);
void                sendInvalidMoveResCode(int socket, int gameId, struct recvd_packet_info playerRequest);
void                sendInvalidRequestResCode(int socket, struct recvd_packet_info playerRequest);
void                sendNotOKReturnCode(int socket, int gameId, int returnCode, struct recvd_packet_info playerRequest);
void                sendGameCompleteAck(int socket, int gameId, struct recvd_packet_info playerRequest);
int                 updateBoard(int move, struct ServerData *data, int gameId, char mark);
int                 getGameId(struct recvd_packet_info playerRequest, struct ServerData data);
int                 isSameAddr(struct sockaddr_in recvAddr, struct sockaddr_in remoteAddr);
int                 isAlreadyConnected(struct recvd_packet_info playerRequest, struct ServerData data);
int                 remoteAddressConnected(struct sockaddr_in remoteAddr, struct ServerData data);
void                endGame(int gameId, struct ServerData *data, struct sockaddr_in remoteAddr);
void                handleGameOver(int checkWin, char *addr, int remotePlayerMoved, char srvrMark, char board[ROWS][COLUMNS]);
void                initGameBoard(char board[ROWS][COLUMNS]);

////////////////////////////////
/// Entry point of application
////////////////////////////////
int main(int argc, char *argv[])
{
    struct ServerData data = validateInput(argc, argv);              // validate input, if fails then terminate
    data.socket = createSocket();
    if (data.socket < 0)
        exit(EXIT_FAILURE);
    
    if (bindToSocket(data.socket, data.port, &data.serverAddr) < 0)
        exit(EXIT_FAILURE);

    printf("TicTacToe game server is running. Waiting for remote players to connect...\n");
    while (1)
    {
        struct recvd_packet_info playerRequest = getRemotePlayerRequest(data.socket);
        handlePlayerRequest(playerRequest, &data);
    }
    return 0;
}

/////////////////////////////////////////////////////////////
/// Method to validate command line arguments
///     If validation fails, then program will be teminated.
///     Returns an initialized ServerData struct
/////////////////////////////////////////////////////////////
struct ServerData validateInput(int argc, char** argv)
{
    // cmdLine to execute: tictactoeServer <local-port> <use-bot>
    if (argc != 3)
    {
        fprintf(stderr, "%s\nusage: tictactoeServer <local-port> <bot-lvl>\n   %s\n   %s\n", 
            "ERROR: Invalid input specified", "<local-port>  : port address of the on which server is running.",
            "<bot-lvl>      : Specify 1 to use easy bot, 2 for medium and 3 to use hard bot.");
        exit(EXIT_FAILURE);
    }

    // validate port number
    if (!isNum(argv[PORT]) || atoi(argv[PORT]) > 65535 || atoi(argv[PORT]) < 1024)
    {
        fprintf(stderr, 
            "ERROR: Invalid input specified: port number '%s' is invalid. Port number can only be between 1024 and 65535.\n", argv[PORT]);
        exit(EXIT_FAILURE);
    }

    if (argc == 3)
    {
        if (!(atoi(argv[BOT_LVL]) == 3 || atoi(argv[BOT_LVL]) == 2 || atoi(argv[BOT_LVL]) == 1))
        {
            fprintf(stderr, "ERROR: Invalid input (%s) specified: Specify 1 to use easy bot, 2 for medium and 3 to use hard bot.\n", argv[BOT_LVL]);
            exit(EXIT_FAILURE);
        }
    }

    struct ServerData serverData;
    memset(&serverData.serverAddr, 0, sizeof(serverData.serverAddr));
    serverData.botLvl = atoi(argv[BOT_LVL]);
    serverData.port = atoi(argv[PORT]);
    serverData.mark = 'X';
    for (int i = 0; i < MAX_GAME; i++)
    {
        serverData.allGames[i].id = -1;
        serverData.allGames[i].state = WaitingForPlayer;
        serverData.allGames[i].version = 0;
    }

    return serverData;
}

/* This method just initializes the shared state (aka board) */
void initGameBoard(char board[ROWS][COLUMNS])
{
    int count = 1;
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            board[i][j] = count + '0';
            count++;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Method to handle a request received by remote player.
///     If request is invalid, then it server sends Invalid_Request return code to client
//////////////////////////////////////////////////////////////////////////////////////////
void handlePlayerRequest(struct recvd_packet_info playerRequest, struct ServerData *serverData)
{
    if (validateVersionResCodeAndMsgSize(playerRequest, serverData))
    {
        switch(playerRequest.recvdMsg.cmdCode)
        {
            case CMD_CODE_MOVE: continueExistingGame(playerRequest, serverData); break;
            case CMD_CODE_NEW_GAME: startNewGame(playerRequest, serverData); break;
            default:
                printGameErr("Remote player (%s) sent invalid cmd code(%d)", playerRequest.addr_str, playerRequest.recvdMsg.cmdCode);
                sendInvalidRequestResCode(serverData->socket, playerRequest);
                endGame(playerRequest.recvdMsg.gameId, serverData, playerRequest.remoteAddr);
                break;
        }
    }
    else
        endGame(playerRequest.recvdMsg.gameId, serverData, playerRequest.remoteAddr);
}

/////////////////////////////////////////////////////////////////////////////////////////////
/// Method to start a new game.
///     If not free slot is available, then server sends a ServerBusy return code to client
/////////////////////////////////////////////////////////////////////////////////////////////
void startNewGame(struct recvd_packet_info playerRequest, struct ServerData *serverData)
{
    int nextFreeSlot = getNextFreeGameSlot(*serverData, playerRequest);
    if (nextFreeSlot >= 0)
    {
        printGameMsg("Starting new game with remote player (%s)", playerRequest.addr_str);
        initGameBoard(serverData->allGames[nextFreeSlot].board);
        serverData->allGames[nextFreeSlot].id = nextFreeSlot;
        serverData->allGames[nextFreeSlot].turn = -1;
        serverData->allGames[nextFreeSlot].state = Started;
        serverData->allGames[nextFreeSlot].version = playerRequest.recvdMsg.version;
        serverData->allGames[nextFreeSlot].lastRequestTime = time(NULL);
        memcpy(&serverData->allGames[nextFreeSlot].clientAddr, &playerRequest.remoteAddr, sizeof(playerRequest.remoteAddr));

        int move = getBotMove(serverData->allGames[nextFreeSlot].board, 0, serverData->mark, serverData->botLvl);
        if (updateBoard(move, serverData, nextFreeSlot, serverData->mark))
        {
            sendResponseToRemotePlayer(serverData->allGames[nextFreeSlot].version, move, CMD_CODE_MOVE, 0, 0, nextFreeSlot, 
                serverData->socket, playerRequest.remoteAddr);
        }
    }
    else
    {
        printGameErr("Remote player's (%s) game request declined as no free slot available", playerRequest.addr_str);
        sendNotOKReturnCode(serverData->socket, 0, RES_CODE_SERVER_BUSY, playerRequest);
    }
}

///////////////////////////////////////////////////
/// Method to update game board based on a move.
///     Returns 0 if update failed, else return 1
///////////////////////////////////////////////////
int updateBoard(int move, struct ServerData *data, int gameId, char mark)
{
    int row, col;
    getCellIndex(&row, &col, move);
    if (data->allGames[gameId].board[row][col] == (move + '0'))
    {
        data->allGames[gameId].board[row][col] = mark; 
        data->allGames[gameId].turn++;
        return 1;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Method to continue an existing game with remote player
///     if remote player is not connected or gameId sent by client is invalid, then server sends Game_Id_Mismatch res code
///     if remote player sent an invalid turn number, then server sends Invalid_Turn response code to remote player
///     if remote player played an invalid move, then server sends Invalid_Move response code
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void continueExistingGame(struct recvd_packet_info playerRequest, struct ServerData *serverData)
{
    if (!isAlreadyConnected(playerRequest, *serverData))
    {
        printError("Remote player (%s) does not have an active game. Rejecting received request...", playerRequest.addr_str);
        sendNotOKReturnCode(serverData->socket, 0, RES_CODE_GAME_ID_MISMATCH, playerRequest);
        return;
    }
    int gameId = getGameId(playerRequest, *serverData);
    struct GameData *gameData = &serverData->allGames[gameId];
    if (playerRequest.recvdMsg.responseCode == RES_CODE_GAME_OVER_ACK)
    {
        if (gameData->state != Completed)
        {
            printGameErr("Remote player (%s) sent game over ack before game was actually over.", playerRequest.addr_str);
            sendInvalidRequestResCode(serverData->socket, playerRequest);
        }
        
        endGame(gameData->id, serverData, playerRequest.remoteAddr);
        return;
    }
    if (gameData->turn + 1 != playerRequest.recvdMsg.turnNumber)
    {
        printGameErr("Remote player's (%s) game is out of sync", playerRequest.addr_str);
        endGame(gameId, serverData, playerRequest.remoteAddr);
        sendNotOKReturnCode(serverData->socket, gameId, RES_CODE_NO_SYNC, playerRequest);
        return;
    }
    if (updateBoard(playerRequest.recvdMsg.move, serverData, gameData->id, 'O'))
    {
        gameData->lastRequestTime = time(NULL);
        int checkWin = checkwin(gameData->board);
        if (checkWin == 0)
        {
            int move = getBotMove(gameData->board, gameData->turn+1, serverData->mark, serverData->botLvl);
            updateBoard(move, serverData, gameData->id, serverData->mark);
            checkWin = checkwin(gameData->board);
            if (checkWin == 1 || checkWin == 2)
            {
                sendResponseToRemotePlayer(gameData->version, move, CMD_CODE_MOVE, gameData->turn,
                    RES_CODE_GAME_OVER, gameData->id, serverData->socket, gameData->clientAddr);
                handleGameOver(checkWin, playerRequest.addr_str, 0, serverData->mark, gameData->board);
                gameData->state = Completed;
            }
            else
            {
                sendResponseToRemotePlayer(gameData->version, move, CMD_CODE_MOVE, gameData->turn,
                    RES_CODE_OK, gameData->id, serverData->socket, gameData->clientAddr);
            }
        }
        else
        {
            gameData->state = Completed;
            if (playerRequest.recvdMsg.responseCode != RES_CODE_GAME_OVER)
                printGameMsg("Game completed but Remote player (%s) did not send game over.", playerRequest.addr_str);
            sendGameCompleteAck(serverData->socket, gameId, playerRequest);
            handleGameOver(checkWin, playerRequest.addr_str, 1, serverData->mark, gameData->board);
            endGame(gameId, serverData, playerRequest.remoteAddr);
        }
    }
    else
    {
        sendInvalidMoveResCode(serverData->socket, gameId, playerRequest);
        endGame(gameId, serverData, playerRequest.remoteAddr);
    }
}

//////////////////////////////////////////////
/// Method to display game complete messages.
//////////////////////////////////////////////
void handleGameOver(int checkWin, char *addr, int remotePlayerMoved, char srvrMark, char board[ROWS][COLUMNS])
{
    if (checkWin == 1 && remotePlayerMoved)
        printGameMsg("Game with player '%s' ended in loss.", addr);
    else if (checkWin == 1 && !remotePlayerMoved)
        printGameMsg("Game with player '%s' ended in victory.", addr);
    else if (checkWin == 2)
        printGameMsg("Game with player '%s' ended in draw.", addr);

    print_board(board, addr, srvrMark);
}

///////////////////////////////////////////////////////////////////////
/// Method to get gameId based on remote player's version and request.
///////////////////////////////////////////////////////////////////////
int getGameId(struct recvd_packet_info playerRequest, struct ServerData data)
{
    if (playerRequest.recvdMsg.version > 1)
        return playerRequest.recvdMsg.gameId;

    return remoteAddressConnected(playerRequest.remoteAddr, data);
}

//////////////////////////////////////////////////////////////////////////////////
/// Method to check if remote player is connected (based on ip address and port).
///     if connected, then returns the game number else -1
//////////////////////////////////////////////////////////////////////////////////
int remoteAddressConnected(struct sockaddr_in remoteAddr, struct ServerData data)
{
    for (int i = 0; i < MAX_GAME; i++)
    {
        if (data.allGames[i].id >= 0 && isSameAddr(remoteAddr, data.allGames[i].clientAddr))
            return i;
    }

    return -1;
}

//////////////////////////////////////////////////
/// Method to check if remote player is connected
///     if connected, then returns 0 else -1
//////////////////////////////////////////////////
int isAlreadyConnected(struct recvd_packet_info playerRequest, struct ServerData data)
{
    if (playerRequest.recvdMsg.version == MIN_VERSION)
        return remoteAddressConnected(playerRequest.remoteAddr, data) >= 0 ? 1 : 0;
    else
    {
        int gameId = playerRequest.recvdMsg.gameId;
        return (isSameAddr(playerRequest.remoteAddr, data.allGames[gameId].clientAddr)) ? 1 : 0;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////
/// This method is to check if two address have same port and ip address
/////////////////////////////////////////////////////////////////////////
int isSameAddr(struct sockaddr_in recvAddr, struct sockaddr_in remoteAddr)
{
    if (remoteAddr.sin_addr.s_addr == recvAddr.sin_addr.s_addr && remoteAddr.sin_port == recvAddr.sin_port)
        return 1;
    
    return 0;
}

///////////////////////////////////////////////////////////////////////
/// This method is to get the next free game slot.
///     returns the next free game slot or -1 if no slot is available.
///////////////////////////////////////////////////////////////////////
int getNextFreeGameSlot(struct ServerData serverData, struct recvd_packet_info playerRequest)
{
    if (!isAlreadyConnected(playerRequest, serverData))
    {
        for (int i = 0; i < MAX_GAME; i++)
        {
            if (serverData.allGames[i].id == -1)
                return i;
        }
    }
    return -1;
}

///////////////////////////////////////////////////////////////////////////////////////
/// This method is to validate Version, Response Code and MsgSIze of received request.
///     returns 0 if validation fails else returns 1
///////////////////////////////////////////////////////////////////////////////////////
int validateVersionResCodeAndMsgSize(struct recvd_packet_info playerRequest, struct ServerData *serverData)
{
    // validate version
    if (playerRequest.recvdMsg.version < MIN_VERSION || playerRequest.recvdMsg.version > MAX_VERSION)
    {
        printGameErr("Remote player's (%s) game version(%d) is not supported", playerRequest.addr_str, playerRequest.recvdMsg.version);
        sendNotOKReturnCode(serverData->socket, playerRequest.recvdMsg.gameId, RES_CODE_VERSION_MISMATCH, playerRequest);
        return 0;
    }

    // validate msg size
    if ((playerRequest.recvdMsg.version == 1 && playerRequest.numBytesRecvd != 5) || 
        (playerRequest.recvdMsg.version == 2 && playerRequest.numBytesRecvd != 6))
    {
        printGameErr("Remote player (%s) sent msg of invalid size(%d)", playerRequest.addr_str, playerRequest.numBytesRecvd);
        return 0;
    }

    int isValid = 0;
    switch (playerRequest.recvdMsg.responseCode)        // validate response codes
    {
        case RES_CODE_OK: 
            isValid = 1; break;
        case RES_CODE_INVALID_MOVE: 
            printGameErr("Remote player's (%s) last move was invalid", playerRequest.addr_str); 
            isValid = 0; break;
        case RES_CODE_NO_SYNC: 
            printGameErr("Remote player's (%s) game is out of sync", playerRequest.addr_str);
            isValid = 0; break;
        case RES_CODE_INVALID_REQ: 
            printGameErr("Remote player (%s) denied last msg sent as it did not follow protocol", playerRequest.addr_str); 
            isValid = 0; break;
        case RES_CODE_GAME_OVER: 
            printGameMsg("Remote player (%s) requested game completion.", playerRequest.addr_str); 
            isValid = 1; break;
        case RES_CODE_GAME_OVER_ACK: 
            printGameMsg("Remote player (%s) acknowledges game completion.", playerRequest.addr_str); 
            isValid = 1; break;
        case RES_CODE_VERSION_MISMATCH: 
            printGameErr("Remote player (%s) does support game version", playerRequest.addr_str); 
            isValid = 0; break;
        case RES_CODE_GAME_ID_MISMATCH: 
            printGameErr("Remote player (%s) denied last msg's game id", playerRequest.addr_str);
            isValid = 0; break;
        default: 
            printGameErr("Remote player (%s) sent invalid/unknown res code(%d)", playerRequest.addr_str, playerRequest.recvdMsg.responseCode); 
            sendInvalidRequestResCode(serverData->socket, playerRequest);
            isValid = 0; break;
    }

    return isValid;
}

//////////////////////////////////////////////////////////////////////////////////
/// This method is to send a RES_CODE_INVALID_REQ response code to remote player.
//////////////////////////////////////////////////////////////////////////////////
void sendInvalidRequestResCode(int socket, struct recvd_packet_info playerRequest)
{
    sendNotOKReturnCode(socket, 0, RES_CODE_INVALID_REQ, playerRequest);
}

//////////////////////////////////////////////////////////////////////////////////
/// This method is to send a RES_CODE_INVALID_MOVE response code to remote player.
//////////////////////////////////////////////////////////////////////////////////
void sendInvalidMoveResCode(int socket, int gameId, struct recvd_packet_info playerRequest)
{
    printGameErr("Remote player (%s) played invalid move(%d)", playerRequest.addr_str, playerRequest.recvdMsg.move); 
    sendNotOKReturnCode(socket, gameId, RES_CODE_INVALID_MOVE, playerRequest);
}

////////////////////////////////////////////////////////////////////////////////////
/// This method is to send a RES_CODE_GAME_OVER_ACK response code to remote player.
////////////////////////////////////////////////////////////////////////////////////
void sendGameCompleteAck(int socket, int gameId, struct recvd_packet_info playerRequest)
{
    printGameMsg("Sending game over ack to remote player (%s)", playerRequest.addr_str);
    sendNotOKReturnCode(socket, gameId, RES_CODE_GAME_OVER_ACK, playerRequest);
}

/////////////////////////////////////////////////////////////////////////
/// This method is to send a any non OK response code to remote player.
/////////////////////////////////////////////////////////////////////////
void sendNotOKReturnCode(int socket, int gameId, int responseCode, struct recvd_packet_info playerRequest)
{
    sendResponseToRemotePlayer(playerRequest.recvdMsg.version, 0, 0, 0, responseCode, gameId, socket, playerRequest.remoteAddr);
}

//////////////////////////////////////////////////////////////////////////////////
/// Call this method to check game results.
/// Returns 0 if game is not complete, 1 if someone won and 2 if game is draw
//////////////////////////////////////////////////////////////////////////////////
int checkwin(char board[ROWS][COLUMNS])
{
    /************************************************************************/
    /* brute force check to see if someone won, or if there is a draw       */
    /* return a 0 if the game is 'over' and return -1 if game should go on  */
    /************************************************************************/
    if ((board[0][0] == board[0][1] && board[0][1] == board[0][2]) ||           // row matches
            (board[1][0] == board[1][1] && board[1][1] == board[1][2]) ||
            (board[2][0] == board[2][1] && board[2][1] == board[2][2]))
        return 1;
    else if ((board[0][0] == board[1][0] && board[1][0] == board[2][0]) ||      // column
            (board[0][1] == board[1][1] && board[1][1] == board[2][1]) ||
            (board[0][2] == board[1][2] && board[1][2] == board[2][2]))
        return 1;
    else if ((board[0][0] == board[1][1] && board[1][1] == board[2][2]) ||      // diagonal
            (board[2][0] == board[1][1] && board[1][1] == board[0][2]))
        return 1;
    else if (board[0][0] != '1' && board[0][1] != '2' && board[0][2] != '3' &&
            board[1][0] != '4' && board[1][1] != '5' && board[1][2] != '6' &&
            board[2][0] != '7' && board[2][1] != '8' && board[2][2] != '9')
        return 2;                                                               // Return of 2 means game over
    else
        return 0;                                                               // return of 0 means keep playing
}

/////////////////////////////////////////////
/// This method is to end an exisiting game.
/////////////////////////////////////////////
void endGame(int gameId, struct ServerData *data, struct sockaddr_in remoteAddr)
{
    struct GameData *gameData = &data->allGames[gameId];
    if (isSameAddr(remoteAddr, gameData->clientAddr))
    {
        gameData->id = -1;
        gameData->turn = -1;
        gameData->state = WaitingForPlayer;
        memset(&gameData->clientAddr, 0, sizeof(gameData->clientAddr));
        initGameBoard(gameData->board);
    }
}