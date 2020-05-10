/****************************************************************************************/
/* CSE 5462 - Lab 4                                                                     */
/*      This program implements client end of tictactoe game and uses Datagram socket   */
/*      to communicate with the server. The client and server play against each other   */
/*      and server is always player 1 and client is player 2.                           */
/*                                                                                      */
/* Name: Ishan Deep                                                                     */
/* Lname.#: deep.24                                                                     */
/*                                                                                      */
/* Created on Thu Feb 11 2020                                                           */
/*                                                                                      */
/****************************************************************************************/

/* include files go here */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/********************************** #define section *************************************/
#define VERSION 1
#define ROWS 3
#define COLUMNS 3
#define MSG_SIZE 5
// cmd line arg indexes
#define IP_ADDR 1
#define PORT 2
#define BOT_MODE 3
// CMD_CODES
#define CMD_CODE_MOVE 0
#define CMD_CODE_NEW_GAME 1
#define CMD_CODE_GAME_DECLINED 2
// RESPONSE_CODES
#define MSG_CODE_OK 0
#define MSG_CODE_INVALID_MOVE 1
#define MSG_CODE_NO_SYNC 2
#define MSG_CODE_INVALID_REQUEST 3
#define MSG_CODE_GAME_OVER 4
#define MSG_CODE_ACK 5
#define MSG_CODE_VERSION_MISMATCH 6
/****************************************************************************************/

/* C language requires that you predefine all the routines you are writing */
int checkwin(char board[ROWS][COLUMNS]);
void print_board(char board[ROWS][COLUMNS]);
void initGameBoard(char board[ROWS][COLUMNS]);
void startGame(char board[ROWS][COLUMNS], int socket_desc, char* ipAddr, long port);
void validateInput(int argc, char** argv);
void establishConnection(int socket, char *ipAddr, long portNumber);
void terminateGame(int socket_desc);
int getNextMove(uint8_t turn, int* move, int socket_desc, struct sockaddr_in remoteAddr, char board[ROWS][COLUMNS]);
int handleValidMove(uint8_t turn, int move, int socket_desc, int remotePlayerRequestedGameOver, char board[ROWS][COLUMNS], struct sockaddr_in remoteAddr);
void handleLocalPlayerInvalidMove(uint8_t choice);
int validateRemoteAddr(struct sockaddr_in recvAddr, struct sockaddr_in remoteAddr, int validateAddr);
struct msg getRemotePlayerResponse(int socket_desc, uint8_t turn, struct sockaddr_in remoteAddr, int validateAddr);
void handleRemotePlayerResponse(struct msg response, int socket_desc, uint8_t turn, struct sockaddr_in remoteAddr);
void getCellIndex(int* row, int* col, uint8_t choice);
void handleGameCompletion(int gameResult, uint8_t turn, uint8_t choice, char board[ROWS][COLUMNS]);
void handleRemotePlayerInvalidMove(uint8_t turn, uint8_t choice, int socket_desc, struct sockaddr_in remoteAddr);
int handleResponseCode(uint8_t responseCode);
int handleCmdCode(uint8_t cmdCode);
void sendInvalidRequestError(int turn, int socket_desc,  struct sockaddr_in remoteAddr);
void sendMoveRequestToServer(uint8_t msg_code, uint8_t turn, uint8_t move, int socket_desc, struct sockaddr_in remoteAddr);
int isNum(char* number);
int isMyTurn(uint8_t turn);
uint8_t recvResponseAsIntegerArray(int socket_desc, struct sockaddr_in remoteAddr);
void sendRequestAsIntegerArray(uint8_t *msgArray, int socket_desc, struct sockaddr_in remoteAddr);
void closeConnection(int socket);
struct sockaddr_in getRemotePlayerInfo(char *ipAddr, long portNumber);
int sendGameRequest(int socket_desc, struct sockaddr_in remotePlayerAddr);
void sendRequestToServer(uint8_t msg_code, uint8_t cmdCode, uint8_t turn, uint8_t move, int socket_desc, struct sockaddr_in remoteAddr);
void validateReturnCode(int rc, char* msg, int socket_desc);
void printMoveOnBoard(char cellValue);
int getBotMove(char board[ROWS][COLUMNS], uint8_t turn, char mark, int botLevel);
int getHardBotMove(char board[ROWS][COLUMNS], int turn, char mark, char opponentMark);
int isGoingToWin(char playerMark, char board[ROWS][COLUMNS], int checkIfTwoCellsFree);
int isFree(int cell, char board[ROWS][COLUMNS]);
int getFreeCell(char board[ROWS][COLUMNS]);
int checkIfCells(char cells[], int cellIndex[], int size, char playerMark, int checkIfTwoCellsFree);
void printRemotePlayerResponse(struct msg response);
void printError(char *errMsg);
char check(int cell, char board[ROWS][COLUMNS]);

// struct resembling to protocol msg structure
struct msg
{
    uint8_t version;
    uint8_t cmdCode;
    uint8_t responseCode;
    uint8_t move;
    uint8_t turnNumber;
};

int playUsingBot = 0;

////////////////////////////////
/// Entry point of application
////////////////////////////////
int main(int argc, char *argv[])
{
    validateInput(argc, argv);                                      // validate input, if fails then terminate

    int socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
    validateReturnCode(socket_desc, "Error occurred while creating socket", -1);

    char remotePlayerIP[16];
    long portNumber = strtol(argv[PORT], NULL, 10);
    strcpy(remotePlayerIP, argv[IP_ADDR]);
    
    char board[ROWS][COLUMNS];
    startGame(board, socket_desc, remotePlayerIP, portNumber);      // start the game
    terminateGame(socket_desc);                                     // end game
    
    return 0;
}

////////////////////////////////////
/// Call this method to start game.
////////////////////////////////////
void startGame(char board[ROWS][COLUMNS], int socket_desc, char* ipAddr, long port)
{ 
    initGameBoard(board);                                // Initialize the game board
    printf("Game started.\n");
    int gameResult;                                      // if -1->ERROR, 0->in progress, 1->someone won, 2->draw;
    int move;                                            // used for keeping track of choice user makes
    char mark;                                           // either an 'X' or an 'O'
    uint8_t turn = 0;                                    // used to keep track of turn number
    int row, column;
    
    // send game request to server
    struct sockaddr_in remotePlayerInfo = getRemotePlayerInfo(ipAddr, port);
    move = sendGameRequest(socket_desc, remotePlayerInfo);

    /* loop, first print the board, then ask player 'n' to make a move */
    do
    {
        int validMove = 0;                              // 1 is player move is valid, else 0
        mark = isMyTurn(turn) ? 'O' : 'X';              //depending on who the player is, either use X or O
        print_board(board);                             // call function to print the board on the screen

        int remotePlayerRequestedGameOver = getNextMove(turn, &move, socket_desc, remotePlayerInfo, board);
        getCellIndex(&row, &column, move);

        /* first check to see if the row/column chosen has a digit in it, if player*/
        /* selected square 8 and that square has '8' then it is valid choice       */

        if (board[row][column] == (move + '0'))
        {
            board[row][column] = mark; 
            validMove = 1;
        }
        else
        {
            validMove = 0;
            if (isMyTurn(turn))
            {
                handleLocalPlayerInvalidMove(move);
                turn--;                                 // retry if error happened in local players turn
            }
            else
            {
                gameResult = -1;                        // game is in bad state
                handleRemotePlayerInvalidMove(turn, move, socket_desc, remotePlayerInfo);
            }
        }

        if (validMove == 1)
            gameResult = handleValidMove(turn, move, socket_desc, remotePlayerRequestedGameOver, board, remotePlayerInfo);
        
        turn++;
    } while (gameResult == 0); // 0 means game is still running
}

///////////////////////////////////////////////////////////////////////////////////////////////
/// This method handles a valid move by the user.
///     -> Returns 1 if someone won, 2 if game ended in a draw and 0 if game is not over
///     -> If remote player sends invalid game over code, then this method will terminate game
///////////////////////////////////////////////////////////////////////////////////////////////
int handleValidMove(uint8_t turn, int move, int socket_desc, int remotePlayerRequestedGameOver, char board[ROWS][COLUMNS], 
    struct  sockaddr_in remotePlayerInfo)
{
    /* after a move, check to see if someone won! (or if there is a draw */
    int gameResult = checkwin(board);
    if (gameResult == 0 && remotePlayerRequestedGameOver)                           // game not over but remote player thinks it is
        sendInvalidRequestError(turn, socket_desc, remotePlayerInfo);
    else if (gameResult == 0 && isMyTurn(turn))                                     // game not over and local player turn
        sendMoveRequestToServer(MSG_CODE_OK, turn, move, socket_desc, remotePlayerInfo);
    else if (gameResult == 1 || gameResult == 2)                                    // game is over
    {
        handleGameCompletion(gameResult, turn, move, board);                        // display proper msg
        if (isMyTurn(turn))                                                         // if local player won then send GameOver and receive ACK
        {
            sendMoveRequestToServer(MSG_CODE_GAME_OVER, turn, move, socket_desc, remotePlayerInfo);       
            getRemotePlayerResponse(socket_desc, turn, remotePlayerInfo, 1);
        }
        else
        {
            if (remotePlayerRequestedGameOver)                                      // if remote player won, send ACK
                sendMoveRequestToServer(MSG_CODE_ACK, turn, move, socket_desc, remotePlayerInfo);
            else
                printError("Error occurred on server end. Was expecting to receive game over request.\n");                    
        }    
    }

    return gameResult;
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

/*****************************************************************/
/* brute force print out the board and all the squares/values    */
/*****************************************************************/
void print_board(char board[ROWS][COLUMNS])
{
    printf("\n\n   Current TicTacToe Game\n");
    printf("Remote Player (X)  -  You (O)\n\n");
    
    for (int i = 0; i < 3; i++)
    {
        printf("     |     |     \n");
        for (int j = 0; j < 3; j++)
        {
            printMoveOnBoard(board[i][j]);
            if (j < 2)
                printf("|");
            else
                printf("\n");        
        }
        if (i < 2)
            printf("_____|_____|_____\n");
        else
            printf("     |     |     \n\n");        
    }
}

/////////////////////////////////////////////////////////////////////
/// This method prints player moves board in their respective color.
/////////////////////////////////////////////////////////////////////
void printMoveOnBoard(char cellValue)
{
    if (cellValue == 'X')               // remote player is bold green
        printf("\033[1;32m");
    else if (cellValue == 'O')          // local player is bold blue
        printf("\033[1;34m");
    
    printf("  %c  ", cellValue);
    printf("\033[0m");                  // reset color
}

///////////////////////////////////////////////////////
/// Call this method to print error msgs in red color.
///////////////////////////////////////////////////////
void printError(char *errMsg)
{
    printf("\033[1;31m");
    printf("%s", errMsg);
    printf("\033[0m");
}

/* This method just initializes the shared state (aka board) */
void initGameBoard(char board[ROWS][COLUMNS])
{
    int count = 1;
    printf("Initialize game board. Please wait...\n");
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            board[i][j] = count + '0';
            count++;
        }
    }
    printf("Initialization complete.\n");
}

/****************************************** DATAGRAM SOCKET related routines ***************************************/

//////////////////////////////////////////////////////////////////
/// Call this method to send a new game request to remote player.
//////////////////////////////////////////////////////////////////
int sendGameRequest(int socket_desc, struct sockaddr_in remotePlayerAddr)
{
    char *ip = inet_ntoa(remotePlayerAddr.sin_addr);
    int port = ntohs(remotePlayerAddr.sin_port);  
    printf("Sending game request to remote player at '%s' using port '%d'. Please wait...\n", ip, port);
    
    sendRequestToServer(MSG_CODE_OK, CMD_CODE_NEW_GAME, 0, 0, socket_desc, remotePlayerAddr);
    int move = getRemotePlayerResponse(socket_desc, 0, remotePlayerAddr, 0).move;
    
    printf("Connection Successful.\nStarting game. Please wait...\n");
    
    return move;
}

///////////////////////////////////////////////////
/// call this method get remote player server info.
///////////////////////////////////////////////////
struct sockaddr_in getRemotePlayerInfo(char *ipAddr, long portNumber)
{
    struct sockaddr_in remotePlayerAddr;
    remotePlayerAddr.sin_family = AF_INET;
    remotePlayerAddr.sin_port = htons(portNumber);
    remotePlayerAddr.sin_addr.s_addr = inet_addr(ipAddr);
    return remotePlayerAddr;
}

//////////////////////////////////////////////////////////////////////////
/// Call this method to get remote player response using DATAGRAM Socket.
//////////////////////////////////////////////////////////////////////////
struct msg getRemotePlayerResponse(int socket_desc, uint8_t turn, struct sockaddr_in remoteAddr, int validateAddr)
{
    
    uint8_t response[MSG_SIZE];
    struct sockaddr_in recvAddr;
    socklen_t size = sizeof(recvAddr);
    int rc;
    do
    {
        rc = recvfrom(socket_desc, &response, MSG_SIZE, MSG_WAITALL, (struct sockaddr *) &recvAddr, &size);
    } while (validateRemoteAddr(recvAddr, remoteAddr, validateAddr) != 1);
    validateReturnCode(rc, "Error occurred while receiving remove player response. Reason", socket_desc);

    struct msg remotePlayerResponse;
    remotePlayerResponse.version = response[0];
    remotePlayerResponse.cmdCode = response[1];
    remotePlayerResponse.responseCode = response[2];
    remotePlayerResponse.move = response[3];
    remotePlayerResponse.turnNumber = response[4];
    handleRemotePlayerResponse(remotePlayerResponse, socket_desc, turn, remoteAddr);
    return remotePlayerResponse;
}

/////////////////////////////////////////////////////////////////
/// This method is to validate if person sending the request is 
/// same as the one with whom the game is being played with.
/////////////////////////////////////////////////////////////////
int validateRemoteAddr(struct sockaddr_in recvAddr, struct sockaddr_in remoteAddr, int validateAddr)
{
    if (validateAddr)
    {
        if (remoteAddr.sin_addr.s_addr != recvAddr.sin_addr.s_addr || remoteAddr.sin_port != recvAddr.sin_port)
            return 0;
    }

    return 1;
}


/////////////////////////////////////////////////////////////
/// Call this method to tranfer a 1 byte number over socket.
///     if send failed, then game will terminate.
/////////////////////////////////////////////////////////////
void sendRequestAsIntegerArray(uint8_t *msgArray, int socket_desc, struct sockaddr_in remoteAddr)  
{
    int rc = sendto(socket_desc, (const uint8_t *)msgArray, MSG_SIZE, MSG_CONFIRM, 
        (const struct sockaddr *) &remoteAddr, sizeof(remoteAddr));
    validateReturnCode(rc, "Error occurred while sending request to remove player. Reason", socket_desc);
}

//////////////////////////////////////////////////////////////////
/// Call this method to send local players move to remote player.
//////////////////////////////////////////////////////////////////
void sendMoveRequestToServer(uint8_t msg_code, uint8_t turn, uint8_t move, int socket_desc, struct sockaddr_in remoteAddr)
{
    sendRequestToServer(msg_code, CMD_CODE_MOVE, turn, move, socket_desc, remoteAddr);
}

/////////////////////////////////////////////////////////////////////
/// Call this method to send local players request to remote player.
/////////////////////////////////////////////////////////////////////
void sendRequestToServer(uint8_t msg_code, uint8_t cmdCode, uint8_t turn, uint8_t move, int socket_desc, struct sockaddr_in remoteAddr)
{
    if (msg_code == MSG_CODE_ACK)
        printf("\nSending game over acknowledgement to remote player. Please wait...\n");

    uint8_t msg[MSG_SIZE];
    msg[0] = (uint8_t)VERSION;
    msg[1] = (uint8_t)cmdCode;
    msg[2] = (uint8_t)msg_code;
    msg[3] = (uint8_t)move;
    msg[4] = (uint8_t)turn;

    sendRequestAsIntegerArray(msg, socket_desc, remoteAddr);
}

////////////////////////////////////////////////////////////
/// Call this method to close connection to remote player.
////////////////////////////////////////////////////////////
void closeConnection(int socket)
{
    printf("Closing connection to remote player. Please wait...\n");
    close(socket);
    printf("Connection closed.\n");
}

/*******************************************************************************************************************/


int isNum(char* number)
{
    for (int i = 0; i < strlen(number); i++)
    {
        if (!isdigit(number[i]))
            return 0;
    }
    return 1;
}


/////////////////////////////////////////////////////////////
/// Call this method to handle invalid move by local player.
/////////////////////////////////////////////////////////////
void handleLocalPlayerInvalidMove(uint8_t choice)
{
    printError("\nError: Invalid move specified. ");
    if (choice < 1 || choice > 9)
        printError("Choice should be between 1 and 9.\n");
    else
    {
        char errMsg[100];
        sprintf(errMsg, "Cannot move to cell '%d' as it is already occupied.\n", choice);
        printError(errMsg);
    }

    getchar();                              // eat up the additional newline from last input
}


////////////////////////////////////////////////
/// Check if it is local player's turn to play.
////////////////////////////////////////////////
int isMyTurn(uint8_t turn)
{
    return (turn % 2 == 0) ? 0 : 1;
}


//////////////////////////////////////////////////////////////////////
/// Call this method to get row and col index based on player's move.
//////////////////////////////////////////////////////////////////////
void getCellIndex(int* row, int* col, uint8_t choice)
{
    /******************************************************************/
    /** little math here. you know the squares are numbered 1-9, but  */
    /* the program is using 3 rows and 3 columns. We have to do some  */
    /* simple math to convert a 1-9 to the right row/column           */
    /******************************************************************/
    *row = (int)((choice - 1) / ROWS);
    *col = (choice - 1) % COLUMNS;
}


/////////////////////////////////////////////////////////////////
/// Call this method handle the case when game has completed.
///     This method is responsible for displaying game results.
/////////////////////////////////////////////////////////////////
void handleGameCompletion(int gameResult, uint8_t turn, uint8_t choice, char board[ROWS][COLUMNS])
{
    /* print out the board again */
    print_board(board);
    printf("\033[1;36m");                  // change color to bold cyan
    if (gameResult == 1 && isMyTurn(turn)) // means a player won!! congratulate them
        printf("==>\aYou won<==\n ");
    else if (gameResult == 1 && !isMyTurn(turn))
        printf("==>\aYou Lost<==\n ");
    else
        printf("==>\aGame draw<==\n");        // ran out of squares, it is a draw
    
    printf("\033[0m");                     // reset color
}


/////////////////////////////////////////////////////////////////
/// Call this method to handle/validate remote player response.
///     -> if validation fails then this will terminate game.
/////////////////////////////////////////////////////////////////
void handleRemotePlayerResponse(struct msg response, int socket_desc, uint8_t turn, struct sockaddr_in remoteAddr)
{
    printRemotePlayerResponse(response);
    if (response.version != VERSION)                    // if version does not match, then terminate game. Use 10 as turn number
    {
        printError("Error: Remote player game version does not match your game version.\n");
        sendMoveRequestToServer(MSG_CODE_VERSION_MISMATCH, turn, 10, socket_desc, remoteAddr);
        terminateGame(socket_desc);
    }

    int shouldTerminate = handleCmdCode(response.cmdCode);
    if (shouldTerminate == 1)                           // if remote player declined game request, then terminate game
        terminateGame(turn);
    else if (shouldTerminate == 2)
        sendInvalidRequestError(turn, socket_desc, remoteAddr);

    shouldTerminate = handleResponseCode(response.responseCode);
    if (shouldTerminate == 1)
        terminateGame(socket_desc);
    else if (shouldTerminate == 2)
        sendInvalidRequestError(turn, socket_desc, remoteAddr);

    if (response.turnNumber != turn)                    // if turn does not match, then terminate game. Use 10 as turn number
    {
        printError("Error: Game is out of sync\n");
        sendMoveRequestToServer(MSG_CODE_NO_SYNC, turn, 10, socket_desc, remoteAddr);
        terminateGame(socket_desc);
    }
}

//////////////////////////////////////////////////////
/// Call this method to print remote player response.
//////////////////////////////////////////////////////
void printRemotePlayerResponse(struct msg response)
{
    printf("\n\n************* Remote Player Response *************\n");
    printf("* Version | CmdCode | ResponseCode | Move | Turn *\n");
    printf("*------------------------------------------------*\n");
    printf("*%9d|%9d|%14d|%6d|%5d *\n", response.version, response.cmdCode, response.responseCode, 
        response.move, response.turnNumber);
    printf("**************************************************\n\n");
}

///////////////////////////////////////////////////////////////////////////////////////////
/// Call this method to handle response code received from the remote player.
/// Returns 0 if game should continue and 1 if server encountered error(terminate game) or
///     2 if invalid request was received from remote player
///////////////////////////////////////////////////////////////////////////////////////////
int handleResponseCode(uint8_t responseCode)
{
    switch (responseCode)
    {
        case MSG_CODE_OK: return 0;
        case MSG_CODE_INVALID_MOVE: printError("Error: last move was invalid.\n"); return 1;
        case MSG_CODE_NO_SYNC: printError("Error: Game is out of sync\n"); return 1;
        case MSG_CODE_INVALID_REQUEST: printError("Error: Invalid request\n"); return 1;
        case MSG_CODE_GAME_OVER: printf("Game complete.\n"); return 0;
        case MSG_CODE_ACK: printf("Remote player acknowledges game completion\n"); return 1;
        case MSG_CODE_VERSION_MISMATCH: printError("Error: Remote player game version does not match your game version.\n"); return 1;
        default: printError("Error: Unknown response code received.\n"); return 2;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
/// Call this method to handle cmd code received from the remote player.
/// Returns 0 if game should continue and 1 if server encountered error(terminate game) or
///     2 if invalid request was received from remote player
///////////////////////////////////////////////////////////////////////////////////////////
int handleCmdCode(uint8_t cmdCode)
{
    switch (cmdCode)
    {
        case CMD_CODE_MOVE: return 0;
        case CMD_CODE_NEW_GAME: printError("Error: Remote player cannot request a new game.\n"); return 1;
        case CMD_CODE_GAME_DECLINED: printError("Error: Remote player declined game request.\n"); return 1;
        default: printError("Error: Unknown cmd code received.\n"); return 2;
    }
}

void handleRemotePlayerInvalidMove(uint8_t turn, uint8_t choice, int socket_desc, struct sockaddr_in remoteAddr)
{
    printError("Error: Remote player used invalid move.\n");
    sendMoveRequestToServer(MSG_CODE_INVALID_MOVE, turn, choice, socket_desc, remoteAddr);
    terminateGame(socket_desc);
}

/////////////////////////////////////////////////////////////////////////////////
/// Call this method to get next players move.
///     if next player is remote player, then get his move using socket.
///         in case of turn 0, we already have the move played by remote player.
///     if next player is local player, then use scanf to get his move.
/////////////////////////////////////////////////////////////////////////////////
int getNextMove(uint8_t turn, int* move, int socket_desc, struct sockaddr_in remoteAddr, char board[ROWS][COLUMNS])
{
    int remotePlayerRequestedGameOver = 0;
    if (isMyTurn(turn))
    {
        if (playUsingBot)
            *move = getBotMove(board, turn, 'O', playUsingBot);
        else
        {
            printf("Your turn, enter a number:  ");     // print out player so you can pass game
            scanf("%d", move);                          // using scanf to get the choice
        }
    }
    else
    {
        if (turn == 0)                              // if it is first move then use server move from the received ack
            return remotePlayerRequestedGameOver;

        printf("Waiting for remote player's turn. Please wait...\n");
        struct msg remotePlayerResponse = getRemotePlayerResponse(socket_desc, turn, remoteAddr, 1);
        *move = remotePlayerResponse.move;
        remotePlayerRequestedGameOver = (remotePlayerResponse.responseCode == MSG_CODE_GAME_OVER) ? 1 : 0;
    }

    return remotePlayerRequestedGameOver;
}

//////////////////////////////////////////////////////////////////
/// This method is used to send invalid request to remote player.
//////////////////////////////////////////////////////////////////
void sendInvalidRequestError(int turn, int socket_desc,  struct sockaddr_in remoteAddr)
{
    sendMoveRequestToServer(MSG_CODE_INVALID_REQUEST, turn, 10, socket_desc, remoteAddr);
    terminateGame(socket_desc);
}

///////////////////////////////////////////////
/// This method terminates the game execution.
///////////////////////////////////////////////
void terminateGame(int socket_desc)
{
    printf("Game will terminate now. Please wait for 5 seconds...\n");    
    sleep(5);                                     // sleep for response to reach server
    closeConnection(socket_desc);
    printf("Game closed successfully.\n");
    exit(0);
}

/////////////////////////////////////////////////////////////
/// Method to validate command line arguments
///     If validation fails, then program will be teminated.
/////////////////////////////////////////////////////////////
void validateInput(int argc, char** argv)
{
    // cmdLine to execute: ftpc <remote-IP> <remote-port> <local-file-to-transfer>
    if (!(argc == 3 || argc == 4))
    {
        fprintf(stderr, "%s\nusage: tictactoeOriginal <remote-IP> <remote-port> [Optional: <use-bot>]\n   %s\n   %s\n   %s\n", 
            "ERROR: Invalid input specified", "<remote-IP>    : ip address of the remote player", 
            "<remote-port>  : port address of the remote player.",
            "<use-bot>      : Specify 1 to use easy bot, 2 for medium and 3 to use hard bot. NOTE: This is an optional parameter.");
        exit(0);
    }

    // validate ip address
    struct sockaddr_in sa;
    if (inet_pton(AF_INET, argv[IP_ADDR], &(sa.sin_addr)) == 0)
    {
        fprintf(stderr, "ERROR: Invalid input specified: ip address '%s' is invalid. Please use IPv4 format.\n", argv[IP_ADDR]);
        exit(0);
    }

    // validate port number
    if (!isNum(argv[PORT]))
    {
        fprintf(stderr, "ERROR: Invalid input specified: port number '%s' is invalid. Port number can only be a number.\n", argv[PORT]);
        exit(0);
    }

    if (argc == 4)
    {
        if (!(atoi(argv[BOT_MODE]) == 3 || atoi(argv[BOT_MODE]) == 2 || atoi(argv[BOT_MODE]) == 1))
        {
            fprintf(stderr, "ERROR: Invalid input specified: Specify 1 to use easy bot, 2 for medium and 3 to use hard bot.\n");
            exit(0);
        }
        playUsingBot = atoi(argv[BOT_MODE]);
    }
}

//////////////////////////////////////////////////////
/// This method is used to validate the return codes.
///     If validation fails, game will terminate.
//////////////////////////////////////////////////////
void validateReturnCode(int rc, char* msg, int socket_desc)
{
    if (rc < 0)                                            // validate if return code is valid
    {
        perror(msg);
        if (socket_desc < 0)
            exit(0);
        else
            terminateGame(socket_desc);
    } else if (rc < MSG_SIZE && socket_desc > 0) {         // terminate if less number of bytes received
        char err[150];
        sprintf(err, "Error: Invalid number of bytes received from remote player.\nExpected: %d, Received: %d\n", MSG_SIZE, rc);
        printError(err);
        terminateGame(socket_desc);
    }
}


/****************************************** Game bot related routines ***************************************/

char check(int cell, char board[ROWS][COLUMNS])
{
    int row, col;
    getCellIndex(&row, &col, cell);
    return board[row][col];
}

int getBotMove(char board[ROWS][COLUMNS], uint8_t turn, char mark, int botLevel)
{
    char opponentMark = (mark == 'X') ? 'O' : 'X';
    if (botLevel == 1)                                      // if using easy bot, return any free cell 
        return getFreeCell(board);
    else if (botLevel == 2)
    {
        int isWinning = isGoingToWin(mark, board, 0);       // if using medium bot, do the following:
        if (isWinning > 0)                                      // 1. First, try to play a move if it will lead to instant win
            return isWinning;                                   // 2. Else, if opponent is winning, then block his win
        isWinning = isGoingToWin(opponentMark, board, 0);       // 3. Else, try to increase you win chance by playing in row/col/dia 
        if (isWinning > 0)                                      //          with one cell occupied by you and other two are free 
            return isWinning;                                   // 4. Else, play any empty cell
        isWinning = isGoingToWin(mark, board, 1);           
        if (isWinning > 0)                                  // NOTE: Medium bot can be defeated by using fork
            return isWinning;
        return getFreeCell(board);
    }
    else
        return getHardBotMove(board, turn, mark, opponentMark);
}

int getHardBotMove(char board[ROWS][COLUMNS], int turn, char mark, char opponentMark)
{
    if (turn == 0)                                       // if bot is first player, play cell 1 in turn 0.
        return 1;
    else if (turn == 1)                                  // if bot is second player, try to play cell 5 if not blocked else play cell 1.
        return (isFree(5, board)) ? 5 : 1;
    else if (turn == 2)                                  // if bot is first player, try to create a fork opportunity.
        return isFree(1, board) ? 1 : (isFree(9, board) ? 9 : (isFree(7, board) ? 7 : 3));
    else if (turn == 3)
    {
        int isFirstPlayerWinning = isGoingToWin(opponentMark, board, 0);
        if (isFirstPlayerWinning > 0)                    // if opponent is winning, then block him.
            return isFirstPlayerWinning;
        else if (check(5, board) != opponentMark)        // if opponent is not winning, then block his fork opportunity.
            return isFree(2, board) ? 2 : (isFree(4, board) ? 4 : (isFree(6, board) ? 6 : 8));
        else
            return isFree(9, board) ? 9 : (isFree(7, board) ? 7 : 3);
    }
    else
    {
        int isWinning = isGoingToWin(mark, board, 0);    // if bot is winning, play winning cell.
        if (isWinning > 0)
            return isWinning;
        isWinning = isGoingToWin(opponentMark, board, 0);// if opponent is winning, block him.
        if (isWinning > 0)
            return isWinning;
        isWinning = isGoingToWin(mark, board, 1);        // if no one is winning, then choose a row/col/diag with 2 free cells
        if (isWinning > 0)
            return isWinning;
        return getFreeCell(board);                       // get any cell that is not blocked
    }
}

int getFreeCell(char board[ROWS][COLUMNS])
{
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLUMNS; j++)
        {
            int cellIndex = i*3 + j + 1;
            if (isFree(cellIndex, board))
                return cellIndex;
        }
    }

    printError("Error, no more free cells left.\n");
    exit(0);
}

int isFree(int cell, char board[ROWS][COLUMNS])
{
    int row, col;
    getCellIndex(&row, &col, cell);
    if (board[row][col] != 'X' && board[row][col] != 'O')
        return 1;

    return 0;
}

int isGoingToWin(char playerMark, char board[ROWS][COLUMNS], int checkIfTwoCellsFree)
{
    // check for rows
    for (int i = 0; i < ROWS; i++)
    {
        int cellIndexes[COLUMNS] = { i*3+1, i*3+2, i*3+3 };
        if (checkIfCells(board[i], cellIndexes, COLUMNS, playerMark, checkIfTwoCellsFree) > 0)
            return checkIfCells(board[i], cellIndexes, COLUMNS, playerMark, checkIfTwoCellsFree);
    }

    // check for columns
    for (int j = 0; j < COLUMNS; j++)
    {
        char cells[ROWS];
        int cellIndexes[ROWS];
        for (int i = 0; i < ROWS; i++)
        {
            cellIndexes[i] = i*3 + j + 1;
            cells[i] = board[i][j];
        }
        if (checkIfCells(cells, cellIndexes, ROWS, playerMark, checkIfTwoCellsFree) > 0)
            return checkIfCells(cells, cellIndexes, ROWS, playerMark, checkIfTwoCellsFree);
    }

    if (ROWS == COLUMNS)                                           // check for diagonal in case of square board
    {
        // check for left diagonal
        char cells[ROWS];
        int cellIndexes[ROWS];
        for (int i = 0,j = 0; i < ROWS && j < COLUMNS; i++,j++)
        {
            cellIndexes[i] = i*3 + j + 1;
            cells[i] = board[i][j];
        }
        if (checkIfCells(cells, cellIndexes, ROWS, playerMark, checkIfTwoCellsFree) > 0)
                return checkIfCells(cells, cellIndexes, ROWS, playerMark, checkIfTwoCellsFree);

        // check for right diagonal
        for (int i = 0,j = 2; i < ROWS && j >= 0; i++,j--)
        {
            cellIndexes[i] = i*3 + j + 1;
            cells[i] = board[i][j];
        }
        if (checkIfCells(cells, cellIndexes, ROWS, playerMark, checkIfTwoCellsFree) > 0)
                return checkIfCells(cells, cellIndexes, ROWS, playerMark, checkIfTwoCellsFree);
    }

    return 0;
}

int checkIfCells(char cells[], int cellIndex[], int size, char playerMark, int checkIfTwoCellsFree)
{
    int count = 0;
    int freeCellCount = 0;
    int freeCellIndex = -1;
    for (int i = 0; i < size; i++)
    {
        if (cells[i] != 'X' && cells[i] != 'O')
        {
            freeCellIndex = cellIndex[i];
            freeCellCount++;
        }
        else if (cells[i] == playerMark)
            count++;
    }
    if (freeCellIndex > 0 && count == 2)
            return freeCellIndex;
    else if (freeCellIndex > 0 && freeCellCount == 2 && checkIfTwoCellsFree == 1)
        return freeCellIndex;
    
    return 0;
}

/*************************************************************************************************************/