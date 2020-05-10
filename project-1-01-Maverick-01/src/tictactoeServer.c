/****************************************************************************************/
/* CSE 5462 - Lab 6                                                                     */
/*      This program implements server end of tictactoe game and uses Datagram socket   */
/*      to communicate with the client. The client and server play against each other   */
/*      and server is always player 1 and client is player 2.                           */
/*      This version of the server can handle multiple clients at the same time and     */
/*      can account for lost/duplicate packets due to network issues.                   */
/*                                                                                      */
/* Name: Ishan Deep                                                                     */
/* Lname.#: deep.24                                                                     */
/*                                                                                      */
/* Created on Thu Mar 26 2020                                                           */
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
#define VERSION 4                                                       // maximum supported game version
#define MAX_GAME 256                                                    // supports a maximum of 256 games 
#define TIMEOUT 10                                                      // socket timeout for recvFrom
#define MULTICAST_IP "239.0.0.1"                                        // ip address to be used for multicast
#define MULTICAST_PORT 1818                                             // port number to be used for multicast
// cmd line arg indexes
#define PORT 1
#define BOT_LVL 2
// cmd code
#define CMD_CODE_MOVE 0
#define CMD_CODE_NEW_GAME 1
#define CMD_CODE_RESUME_GAME 2
#define CMD_CODE_REQUEST_RESUME 3
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
#define RES_CODE_RESUME_ACCEPTED 9
// board values
#define CELL_VALUE_EMPTY 0
#define CELL_VALUE_LOCAL 1
#define CELL_VALUE_REMOTE 2
/****************************************************************************************/

extern int getBotMove(char board[ROWS][COLUMNS], uint8_t turn, char mark, int botLevel);

enum GameState{
    WaitingForPlayer,
    Started,
    WaitingForAck
};

struct GameData
{
    int id;
    int turn;
    int version;
    enum GameState state;
    char board[ROWS][COLUMNS];
    time_t lastSendTime;
    struct sockaddr_in clientAddr;
};

struct ServerData
{
    struct GameData allGames[MAX_GAME];
    char mark;
    int botLvl;
    int unicastSocket;
    int multicastSocket;
    int port;
    struct sockaddr_in serverAddr;
};

/* C language requires that you predefine all the routines you are writing */
struct ServerData   validateInput(int argc, char** argv);
void                handlePlayerRequest(struct recvd_packet_info playerRequest, struct ServerData *serverData);
void                startNewGame(struct recvd_packet_info playerRequest, struct ServerData *serverData);
void                resumeGame(struct recvd_packet_info playerRequest, struct ServerData *serverData);
int                 loadGameData(int socket, struct GameData *gameData, struct recvd_packet_info playerRequest);
void                continueExistingGame(struct recvd_packet_info playerRequest, struct ServerData *serverData);
int                 getNextFreeGameSlot(struct ServerData serverData, struct recvd_packet_info playerRequest);
int                 validateVersionResCodeAndMsgSize(struct recvd_packet_info playerRequest, struct ServerData *serverData);
int                 checkwin(char board[ROWS][COLUMNS]);
void                sendInvalidMoveResCode(int socket, int gameId, struct recvd_packet_info playerRequest, struct GameData *gameData);
void                sendInvalidRequestResCode(int socket, struct recvd_packet_info playerRequest, struct GameData *gameData);
void                sendNotOKReturnCode(int socket, int gameId, int returnCode, struct recvd_packet_info playerRequest, struct GameData *gameData);
void                sendGameCompleteAck(int socket, int gameId, struct recvd_packet_info playerRequest, struct GameData *gameData);
int                 updateBoard(int move, struct ServerData *data, int gameId, char mark);
int                 getGameId(struct recvd_packet_info playerRequest, struct ServerData data);
int                 isSameAddr(struct sockaddr_in recvAddr, struct sockaddr_in remoteAddr);
int                 isAlreadyConnected(struct recvd_packet_info playerRequest, struct ServerData data);
int                 remoteAddressConnected(struct sockaddr_in remoteAddr, struct ServerData data);
void                endGame(struct GameData *gameData, struct sockaddr_in remoteAddr, char *reason);
void                handleGameOver(int checkWin, char *addr, int remotePlayerMoved, char srvrMark, char board[ROWS][COLUMNS]);
void                initGameBoard(char board[ROWS][COLUMNS]);
void                sendMsgToRemote(struct GameData *gameData, struct msg msg, int sock_desc, struct sockaddr_in remoteAddr);

////////////////////////////////
/// Entry point of application
////////////////////////////////
int main(int argc, char *argv[])
{
    struct ServerData data = validateInput(argc, argv);              // validate input, if fails then terminate
    data.unicastSocket = createUnicastSocket(data.port, &data.serverAddr);
    data.multicastSocket = createMulticastSocket(MULTICAST_IP, MULTICAST_PORT);
    if (data.unicastSocket < 0 || data.multicastSocket < 0)
        exit(EXIT_FAILURE);

    printf("TicTacToe game server is running. Waiting for remote players to connect...\n");
    while (1)
    {
        struct recvd_packet_info playerRequest = getRemotePlayerRequest(data.unicastSocket, data.multicastSocket, TIMEOUT);
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
    serverData.mark = SERVER_MARK;
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
    if (!playerRequest.timedOut)
    {
        if (validateVersionResCodeAndMsgSize(playerRequest, serverData))
        {
            switch(playerRequest.recvdMsg.cmdCode)
            {
                case CMD_CODE_MOVE: continueExistingGame(playerRequest, serverData); break;
                case CMD_CODE_NEW_GAME: startNewGame(playerRequest, serverData); break;
                case CMD_CODE_RESUME_GAME: resumeGame(playerRequest, serverData); break;
                case CMD_CODE_REQUEST_RESUME:
                    printGameMsg("Remote player '%s' is requesting to resume game.", playerRequest.addr_str);
                    if (getNextFreeGameSlot(*serverData, playerRequest) < 0)
                    {
                        printGameErr("Remote player's (%s) game request declined as no free slot available", playerRequest.addr_str);
                        sendNotOKReturnCode(serverData->unicastSocket, 0, RES_CODE_SERVER_BUSY, playerRequest, NULL);
                    }
                    else
                        sendNotOKReturnCode(serverData->unicastSocket, 0, RES_CODE_RESUME_ACCEPTED, playerRequest, NULL);
                    break;
                default:
                    printGameErr("Remote player (%s) sent invalid cmd code(%d)", playerRequest.addr_str, playerRequest.recvdMsg.cmdCode);
                    sendInvalidRequestResCode(serverData->unicastSocket, playerRequest, NULL);
                    break;
            }
        }
        else
            endGame(&serverData->allGames[playerRequest.recvdMsg.gameId], playerRequest.remoteAddr, "");
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/// Method to resume a failed game.
///     If not free slot is available, then server sends a ServerBusy return code to client
/////////////////////////////////////////////////////////////////////////////////////////////
void resumeGame(struct recvd_packet_info playerRequest, struct ServerData *serverData)
{
    int nextFreeSlot = getNextFreeGameSlot(*serverData, playerRequest);
    if (nextFreeSlot >= 0)
    {
        printGameMsg("Resuming new game with remote player (%s)", playerRequest.addr_str);
        struct GameData *gameData = &serverData->allGames[nextFreeSlot];
        if (loadGameData(serverData->unicastSocket, gameData, playerRequest))
        {
            gameData->id = nextFreeSlot;
            gameData->state = Started;
            gameData->version = playerRequest.recvdMsg.version;
            gameData->turn = playerRequest.recvdMsg.turnNumber ? playerRequest.recvdMsg.turnNumber : -1;
            memcpy(&gameData->clientAddr, &playerRequest.remoteAddr, sizeof(playerRequest.remoteAddr));
            printGameMsg("Game board from remote player '%s' is loaded", playerRequest.addr_str);
            print_board(gameData->board, playerRequest.addr_str, SERVER_MARK);
            int checkWin = checkwin(gameData->board);                       // check if game is already over.
            if (checkWin == 0)
            {
                int move = getBotMove(gameData->board, gameData->turn+1, serverData->mark, serverData->botLvl);
                if (updateBoard(move, serverData, gameData->id, serverData->mark))
                {
                    checkWin = checkwin(gameData->board);
                    if (checkWin == 1 || checkWin == 2)
                    {
                        struct msg msg = { gameData->version, CMD_CODE_MOVE, RES_CODE_GAME_OVER, move, gameData->turn, gameData->id };
                        sendMsgToRemote(gameData, msg, serverData->unicastSocket, gameData->clientAddr);
                        handleGameOver(checkWin, playerRequest.addr_str, 0, serverData->mark, gameData->board);
                        gameData->state = WaitingForAck;
                    }
                    else
                    {
                        struct msg msg = { gameData->version, CMD_CODE_MOVE, RES_CODE_OK, move, gameData->turn, gameData->id };
                        sendMsgToRemote(gameData, msg, serverData->unicastSocket, gameData->clientAddr);
                    }
                }
            }
            else
            {
                if (playerRequest.recvdMsg.responseCode != RES_CODE_GAME_OVER)
                    printGameMsg("Game completed but Remote player (%s) did not send game over.", playerRequest.addr_str);
                gameData->turn++;
                sendGameCompleteAck(serverData->unicastSocket, gameData->id, playerRequest, gameData);
                handleGameOver(checkWin, playerRequest.addr_str, 1, serverData->mark, gameData->board);
                endGame(gameData, playerRequest.remoteAddr, "Game completed.");
            }
        }
    }
    else
    {
        printGameErr("Remote player's (%s) game request declined as no free slot available", playerRequest.addr_str);
        sendNotOKReturnCode(serverData->unicastSocket, 0, RES_CODE_SERVER_BUSY, playerRequest, NULL);
    }
}

int loadGameData(int socket, struct GameData *gameData, struct recvd_packet_info playerRequest)
{
    int turn = -1, numX = 0, numO = 0;
    initGameBoard(gameData->board);
    if (playerRequest.recvdMsg.turnNumber == 0)                        // incase if client sent empty board
        return 1;
    
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLUMNS; j++)
        {
            if (playerRequest.recvdMsg.board[i][j] != CELL_VALUE_EMPTY)
            {
                if (playerRequest.recvdMsg.board[i][j] == CELL_VALUE_LOCAL)
                {
                    gameData->board[i][j] = SERVER_MARK;
                    numX++;
                }
                else if (playerRequest.recvdMsg.board[i][j] == CELL_VALUE_REMOTE)
                {
                    numO++;
                    gameData->board[i][j] = CLIENT_MARK;
                }
                else
                {
                    printGameErr("Resume request of remote player '%s' as board state is not proper.", playerRequest.addr_str);
                    sendInvalidRequestResCode(socket, playerRequest, gameData);
                    return 0;
                }
                turn++;
            }
        }
    }
    if (numX != numO)
    {
        printGameErr("Resume request of remote player '%s' as board state is not proper.", playerRequest.addr_str);
        sendInvalidRequestResCode(socket, playerRequest, gameData);
        return 0;
    }
    else if (turn != playerRequest.recvdMsg.turnNumber)
    {
        printGameErr("Remote player's (%s) game is out of sync", playerRequest.addr_str);
        sendNotOKReturnCode(socket, gameData->id, RES_CODE_NO_SYNC, playerRequest, gameData);
        return 0;
    }
    
    return turn == playerRequest.recvdMsg.turnNumber;
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
        struct GameData *gameData = &serverData->allGames[nextFreeSlot];
        initGameBoard(gameData->board);
        gameData->id = nextFreeSlot;
        gameData->turn = -1;
        gameData->state = Started;
        gameData->version = playerRequest.recvdMsg.version;
        memcpy(&gameData->clientAddr, &playerRequest.remoteAddr, sizeof(playerRequest.remoteAddr));

        int move = getBotMove(gameData->board, 0, serverData->mark, serverData->botLvl);
        if (updateBoard(move, serverData, nextFreeSlot, serverData->mark))
        {
            struct msg msg = { gameData->version, CMD_CODE_MOVE, 0, move, 0, nextFreeSlot };
            sendMsgToRemote(gameData, msg, serverData->unicastSocket, playerRequest.remoteAddr);
        }
    }
    else
    {
        printGameErr("Remote player's (%s) game request declined as no free slot available", playerRequest.addr_str);
        sendNotOKReturnCode(serverData->unicastSocket, 0, RES_CODE_SERVER_BUSY, playerRequest, NULL);
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
        sendNotOKReturnCode(serverData->unicastSocket, 0, RES_CODE_GAME_ID_MISMATCH, playerRequest, NULL);
        return;
    }

    int gameId = getGameId(playerRequest, *serverData);
    struct GameData *gameData = &serverData->allGames[gameId];
    if (playerRequest.recvdMsg.responseCode == RES_CODE_GAME_OVER_ACK)
    {
        if (gameData->state != WaitingForAck)
        {
            printGameErr("Remote player (%s) sent game over ack before game was actually over.", playerRequest.addr_str);
            sendInvalidRequestResCode(serverData->unicastSocket, playerRequest, gameData);
        }
        endGame(gameData, playerRequest.remoteAddr, "Game completed.");
        return;
    }

    if (gameData->turn + 1 != playerRequest.recvdMsg.turnNumber)
    {
        printGameErr("Remote player's (%s) game is out of sync", playerRequest.addr_str);
        sendNotOKReturnCode(serverData->unicastSocket, gameId, RES_CODE_NO_SYNC, playerRequest, gameData);
        return;
    }

    if (updateBoard(playerRequest.recvdMsg.move, serverData, gameData->id, 'O'))
    {
        int checkWin = checkwin(gameData->board);
        if (checkWin == 0)
        {
            int move = getBotMove(gameData->board, gameData->turn+1, serverData->mark, serverData->botLvl);
            updateBoard(move, serverData, gameData->id, serverData->mark);
            checkWin = checkwin(gameData->board);
            if (checkWin == 1 || checkWin == 2)
            {
                struct msg msg = { gameData->version, CMD_CODE_MOVE, RES_CODE_GAME_OVER, move, gameData->turn, gameData->id };
                sendMsgToRemote(gameData, msg, serverData->unicastSocket, gameData->clientAddr);
                handleGameOver(checkWin, playerRequest.addr_str, 0, serverData->mark, gameData->board);
                gameData->state = WaitingForAck;
            }
            else
            {
                struct msg msg = { gameData->version, CMD_CODE_MOVE, RES_CODE_OK, move, gameData->turn, gameData->id };
                sendMsgToRemote(gameData, msg, serverData->unicastSocket, gameData->clientAddr);
            }
        }
        else
        {
            if (playerRequest.recvdMsg.responseCode != RES_CODE_GAME_OVER)
                printGameMsg("Game completed but Remote player (%s) did not send game over.", playerRequest.addr_str);
            gameData->turn++;
            sendGameCompleteAck(serverData->unicastSocket, gameId, playerRequest, gameData);
            handleGameOver(checkWin, playerRequest.addr_str, 1, serverData->mark, gameData->board);
            endGame(gameData, playerRequest.remoteAddr, "Game completed.");
        }
    }
    else
    {
        sendInvalidMoveResCode(serverData->unicastSocket, gameId, playerRequest, gameData);
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
    int gameId = playerRequest.recvdMsg.gameId;
    struct GameData *gameData = &data.allGames[gameId];
    return isSameAddr(playerRequest.remoteAddr, gameData->clientAddr) ? 1 : 0;
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
    for (int i = 0; i < MAX_GAME; i++)
    {
        struct GameData *gameData = &serverData.allGames[i];
        if (gameData->state == WaitingForPlayer)
            return i;
        else if (difftime(time(NULL), gameData->lastSendTime) >= TIMEOUT)
        {
            endGame(gameData, gameData->clientAddr, "Game timed out.\n");
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
    if (playerRequest.recvdMsg.version != VERSION)
    {
        printGameErr("Remote player's (%s) game version(%d) is not supported", playerRequest.addr_str, playerRequest.recvdMsg.version);
        sendNotOKReturnCode(serverData->unicastSocket, playerRequest.recvdMsg.gameId, RES_CODE_VERSION_MISMATCH, playerRequest, NULL);
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
            sendInvalidRequestResCode(serverData->unicastSocket, playerRequest, NULL);
            isValid = 0; break;
    }

    return isValid;
}

//////////////////////////////////////////////////////////////////////////////////
/// This method is to send a RES_CODE_INVALID_REQ response code to remote player.
//////////////////////////////////////////////////////////////////////////////////
void sendInvalidRequestResCode(int socket, struct recvd_packet_info playerRequest, struct GameData *gameData)
{
    sendNotOKReturnCode(socket, 0, RES_CODE_INVALID_REQ, playerRequest, gameData);
}

//////////////////////////////////////////////////////////////////////////////////
/// This method is to send a RES_CODE_INVALID_MOVE response code to remote player.
//////////////////////////////////////////////////////////////////////////////////
void sendInvalidMoveResCode(int socket, int gameId, struct recvd_packet_info playerRequest, struct GameData *gameData)
{
    printGameErr("Remote player (%s) played invalid move(%d)", playerRequest.addr_str, playerRequest.recvdMsg.move); 
    sendNotOKReturnCode(socket, gameId, RES_CODE_INVALID_MOVE, playerRequest, gameData);
}

////////////////////////////////////////////////////////////////////////////////////
/// This method is to send a RES_CODE_GAME_OVER_ACK response code to remote player.
////////////////////////////////////////////////////////////////////////////////////
void sendGameCompleteAck(int socket, int gameId, struct recvd_packet_info playerRequest, struct GameData *gameData)
{
    printGameMsg("Sending game over ack to remote player (%s)", playerRequest.addr_str);
    sendNotOKReturnCode(socket, gameId, RES_CODE_GAME_OVER_ACK, playerRequest, gameData);
}

/////////////////////////////////////////////////////////////////////////
/// This method is to send a any non OK response code to remote player.
/////////////////////////////////////////////////////////////////////////
void sendNotOKReturnCode(int sock_desc, int gameId, int responseCode, struct recvd_packet_info playerRequest, struct GameData *gameData)
{
    if (gameData != NULL)
    {
        struct msg msg = { playerRequest.recvdMsg.version, 0, responseCode, 0, 0, gameId };
        sendMsgToRemote(gameData, msg, sock_desc, playerRequest.remoteAddr);
    }
    else
        sendResponseToRemotePlayer(playerRequest.recvdMsg.version, 0, 0, 0, responseCode, gameId, sock_desc, playerRequest.remoteAddr);
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
void endGame(struct GameData *gameData, struct sockaddr_in remoteAddr, char *reason)
{
    if (isSameAddr(remoteAddr, gameData->clientAddr))
    {
        if (strlen(reason) > 0)
            printGameMsg("Game session with id = '%d' terminated. Reason: %s\n", gameData->id, reason);

        gameData->id = -1;
        gameData->turn = -1;
        gameData->state = WaitingForPlayer;
        memset(&gameData->clientAddr, 0, sizeof(gameData->clientAddr));
        initGameBoard(gameData->board);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
/// This is a wrapper method for 'SendResponseToRemotePlayer' function in datagram.h
///     Calling this method will send msg to remote player and also update the value for last sent msg. 
////////////////////////////////////////////////////////////////////////////////////////////////////////
void sendMsgToRemote(struct GameData *gameData, struct msg msg, int sock_desc, struct sockaddr_in remoteAddr)
{
    uint8_t version = msg.version;
    uint8_t cmdCode = msg.cmdCode;
    uint8_t responseCode = msg.responseCode;
    uint8_t move = msg.move;
    uint8_t turnNumber = msg.turnNumber;
    uint8_t gameId = msg.gameId;
    sendResponseToRemotePlayer(version, move, cmdCode, turnNumber, responseCode, gameId, sock_desc, gameData->clientAddr);
    gameData->lastSendTime = time(NULL);
}