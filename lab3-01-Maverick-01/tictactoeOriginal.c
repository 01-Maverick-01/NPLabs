/****************************************************************************************/
/* CSE 5462 - Lab 3                                                                     */
/*      This program implements the client end tictactoe game and uses TCP Stream socket*/
/*      to communicate with the server. The client and server play against each other   */
/*      and server is always player 1 and client is player 2.                           */
/*                                                                                      */
/* Name: Ishan Deep                                                                     */
/* Lname.#: deep.24                                                                     */
/*                                                                                      */
/* Created on Thu Jan 30 2020                                                           */
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
#define ROWS 3
#define COLUMNS 3
#define IP_ADDR 1
#define PORT 2
#define VERSION 0
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
void startGame();
// newly added routines
void validateInput(int argc, char* argv[]);
void establishConnection(int socket, char *ipAddr, long portNumber);
void terminateGame(int socket_desc);
int getNextMove(uint8_t turn, int* move, int socket_desc);
int handleValidMove(uint8_t turn, int move, int socket_desc, int remotePlayerRequestedGameOver, char board[ROWS][COLUMNS]);
void handleLocalPlayerInvalidMove(uint8_t choice);
struct msg getRemotePlayerResponse(int socket_desc, uint8_t turn);
void handleRemotePlayerResponse(struct msg remotePlayerResponse, int socket_desc, uint8_t turn);
void getCellIndex(int* row, int* col, uint8_t choice);
void handleGameCompletion(int gameResult, uint8_t turn, uint8_t choice, char board[ROWS][COLUMNS]);
void handleRemotePlayerInvalidMove(uint8_t turn, uint8_t choice, int socket_desc);
int handleResponseCode(uint8_t responseCode);
void sendRequestToServer(uint8_t msg_code, uint8_t turn, uint8_t move, int socket_desc);
int isNum(char* number);
int isMyTurn(uint8_t turn);
uint8_t recvByteResponseAsInteger(int socket_desc);
void sendByteRequestAsInteger(uint8_t number, int socket_desc);
void closeConnection(int socket);

// struct resembling to protocol msg structure
struct msg
{
    uint8_t version;
    uint8_t responseCode;
    uint8_t move;
    uint8_t turnNumber;
};

////////////////////////////////
/// Entry point of application
////////////////////////////////
int main(int argc, char *argv[])
{
    validateInput(argc, argv);                                      // validate input, if fails then terminate

    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc < 0)                                            // validate if socket was created successfully
    {
        printf("Error occurred while creating socket. Terminating game...\n");
        exit(0);
    }

    char remotePlayerIP[16];
    long portNumber = strtol(argv[PORT], NULL, 10);
    strcpy(remotePlayerIP, argv[IP_ADDR]);
    establishConnection(socket_desc, remotePlayerIP, portNumber);   // establish connection

    char board[ROWS][COLUMNS];
    startGame(board, socket_desc);                                  // start the game
    terminateGame(socket_desc);                                     // end game
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
/// Call this method to start game.
///     -> Make sure connection is established before calling this method
/////////////////////////////////////////////////////////////////////////////
void startGame(char board[ROWS][COLUMNS], int socket_desc)
{
    initGameBoard(board);                                // Initialize the game board
    printf("Game started.\n");
    int gameResult;                                      // if -1->ERROR, 0->in progress, 1->someone won, 2->draw;
    int move;                                            // used for keeping track of choice user makes
    char mark;                                           // either an 'X' or an 'O'
    uint8_t turn = 0;                                    // used to keep track of turn number
    int row, column;

    /* loop, first print the board, then ask player 'n' to make a move */
    do
    {
        int validMove = 0;                              // 1 is player move is valid, else 0
        mark = isMyTurn(turn) ? 'O' : 'X';              //depending on who the player is, either use X or O
        print_board(board);                             // call function to print the board on the screen

        int remotePlayerRequestedGameOver = getNextMove(turn, &move, socket_desc);
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
                handleRemotePlayerInvalidMove(turn, move, socket_desc);
            }
        }

        if (validMove == 1)
            gameResult = handleValidMove(turn, move, socket_desc, remotePlayerRequestedGameOver, board);
        
        turn++;
    } while (gameResult == 0); // 0 means game is still running
}

///////////////////////////////////////////////////////////////////////////////////////////////
/// This method handles a valid move by the user.
///     -> Returns 1 if someone won, 2 if game ended in a draw and 0 if game is not over
///     -> If remote player sends invalid game over code, then this method will terminate game
///////////////////////////////////////////////////////////////////////////////////////////////
int handleValidMove(uint8_t turn, int move, int socket_desc, int remotePlayerRequestedGameOver, char board[ROWS][COLUMNS])
{
    /* after a move, check to see if someone won! (or if there is a draw */
    int gameResult = checkwin(board);
    if (gameResult == 0 && remotePlayerRequestedGameOver)                           // game not over but remote player thinks it is
        sendRequestToServer(MSG_CODE_INVALID_REQUEST, turn, move, socket_desc);
    else if (gameResult == 0 && isMyTurn(turn))                                     // game not over and local player turn
        sendRequestToServer(MSG_CODE_OK, turn, move, socket_desc);
    else if (gameResult == 1 || gameResult == 2)                                    // game is over
    {
        handleGameCompletion(gameResult, turn, move, board);                        // display proper msg
        if (isMyTurn(turn))                                                         // if local player won then send GameOver and receive ACK
        {
            sendRequestToServer(MSG_CODE_GAME_OVER, turn, move, socket_desc);       
            getRemotePlayerResponse(socket_desc, turn);
        }
        else
        {
            if (remotePlayerRequestedGameOver)                                      // if remote player won, send ACK
                sendRequestToServer(MSG_CODE_ACK, turn, move, socket_desc);
            else
                printf("Error occurred on server end. Was expecting to receive game over request.\n");                    
        }    
    }

    return gameResult;
}

//////////////////////////////////////////////////////////////////////////////////
/// Call this method to check game results.
/// Returns 0 is game is not complete, 1 if someone won and 2 if game is draw
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
    printf("\n\n\tCurrent TicTacToe Game\n");
    printf("Remote Player (X)  -  You (O)\n\n");

    printf("     |     |     \n");
    printf("  %c  |  %c  |  %c \n", board[0][0], board[0][1], board[0][2]);
    printf("_____|_____|_____\n");
    printf("     |     |     \n");
    printf("  %c  |  %c  |  %c \n", board[1][0], board[1][1], board[1][2]);
    printf("_____|_____|_____\n");
    printf("     |     |     \n");
    printf("  %c  |  %c  |  %c \n", board[2][0], board[2][1], board[2][2]);
    printf("     |     |     \n\n");
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

/****************************************** STREAM SOCKET related routines ***************************************/

///////////////////////////////////////////////////////////////
/// call this method to establish connection to remote player.
///     if connection failed, then game will terminate.
///////////////////////////////////////////////////////////////
void establishConnection(int socket, char *ipAddr, long portNumber)
{
    struct sockaddr_in remotePlayerAddr;
    remotePlayerAddr.sin_family = AF_INET;
    remotePlayerAddr.sin_port = htons(portNumber);
    remotePlayerAddr.sin_addr.s_addr = inet_addr(ipAddr);
    printf("Connecting to remote player at '%s' using port '%ld'. Please wait...\n", ipAddr, portNumber);
    if(connect(socket, (struct sockaddr*)&remotePlayerAddr, sizeof(struct sockaddr_in)) != 0) 
    {
        close(socket);
        perror("Error connecting stream socket");
        exit(1);
    }
    printf("Connection Successful.\nStarting game. Please wait...\n");
}

///////////////////////////////////////////////////////////////////////
/// call this method to read a 1 byte int response from remote player.
///     if receive failed, then game will terminate.
///////////////////////////////////////////////////////////////////////
uint8_t recvByteResponseAsInteger(int socket_desc)  
{
    uint8_t response;
    while (recv(socket_desc, &response, 1, MSG_PEEK) < 1);  // wait unit 1 bytes is present in stream
    int rc = recv(socket_desc, &response, 1, 0);
    if (rc < 0)
    {
        perror("Error occurred while sending msg to remove player. Reason");
        terminateGame(socket_desc);
    }
    return response;
}


/////////////////////////////////////////////////////////////
/// Call this method to tranfer a 1 byte number over socket.
///     if send failed, then game will terminate.
/////////////////////////////////////////////////////////////
void sendByteRequestAsInteger(uint8_t number, int socket_desc)  
{
    // Using MSG_NOSIGNAL to not generate a SIGPIPE signal if the remote player has closed the connection
    // SIGPIPE signal terminate execution abruptly.
    int rc = send(socket_desc, &number, sizeof(number), MSG_NOSIGNAL);
    if (rc < 0)
    {
        perror("Error occurred while sending msg to remove player. Reason");
        terminateGame(socket_desc);
    }
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

/*****************************************************************************************************************/


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
    printf("Error: Invalid move");
    if (choice < 1 || choice > 9)
        printf("(choice should be between 1 and 9).\n");
    else
        printf("(cannot move to cell as it is already occupied).\n");

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

    if (gameResult == 1 && isMyTurn(turn)) // means a player won!! congratulate them
        printf("==>\aYou won.\n ");
    else if (gameResult == 1 && !isMyTurn(turn))
        printf("==>\aYou Lost.\n ");
    else
        printf("==>\aGame draw\n"); // ran out of squares, it is a draw
}


///////////////////////////////////////////////////////////////////////
/// Call this method to get remote player response using STREAM Socket.
///////////////////////////////////////////////////////////////////////
struct msg getRemotePlayerResponse(int socket_desc, uint8_t turn)
{
    struct msg remotePlayerResponse;    
    remotePlayerResponse.version = recvByteResponseAsInteger(socket_desc);
    remotePlayerResponse.responseCode = recvByteResponseAsInteger(socket_desc);
    remotePlayerResponse.move = recvByteResponseAsInteger(socket_desc);
    remotePlayerResponse.turnNumber = recvByteResponseAsInteger(socket_desc);
    handleRemotePlayerResponse(remotePlayerResponse, socket_desc, turn);
    return remotePlayerResponse;
}


/////////////////////////////////////////////////////////////////
/// Call this method to handle/validate remote player response.
///     -> if validation fails then this will terminate game.
/////////////////////////////////////////////////////////////////
void handleRemotePlayerResponse(struct msg response, int socket_desc, uint8_t turn)
{
    int shouldTerminate = handleResponseCode(response.responseCode);
    if (shouldTerminate == 1)
        terminateGame(socket_desc);

    if (response.version != VERSION)        // if version does not match, then terminate game. Use 10 as turn number
    {
        printf("Error: Remote player game version does not match your game version.");
        sendRequestToServer(MSG_CODE_VERSION_MISMATCH, turn, 10, socket_desc);
        terminateGame(socket_desc);
    }

    if (response.turnNumber != turn)        // if turn does not match, then terminate game. Use 10 as turn number
    {
        printf("Error: Game is out of sync\n");
        sendRequestToServer(MSG_CODE_NO_SYNC, turn, 10, socket_desc);
        terminateGame(socket_desc);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
/// Call this method to handle response code received from the remote player.
/// Returns 0 if game should continue and 1 if server encountered error(terminate game)
/////////////////////////////////////////////////////////////////////////////////////////
int handleResponseCode(uint8_t responseCode)
{
    switch (responseCode)
    {
        case MSG_CODE_OK: return 0;
        case MSG_CODE_INVALID_MOVE: printf("Error: last move was invalid.\n"); return 1;
        case MSG_CODE_NO_SYNC: printf("Error: Game is out of sync\n"); return 1;
        case MSG_CODE_INVALID_REQUEST: printf("Error: Invalid request\n"); return 1;
        case MSG_CODE_GAME_OVER: printf("Game complete.\n"); return 0;
        case MSG_CODE_ACK: printf("Remote player acknowledges game completion\n"); return 1;
        case MSG_CODE_VERSION_MISMATCH: printf("Error: Remote player game version does not match your game version.\n"); return 1;
        default: printf("Error: Unknown response received.\n"); return 1;
    }
}

/////////////////////////////////////////////////////////////////////
/// Call this method to send local players move to remote player.
/////////////////////////////////////////////////////////////////////
void sendRequestToServer(uint8_t msg_code, uint8_t turn, uint8_t move, int socket_desc)
{
    if (msg_code == MSG_CODE_ACK)
        printf("Sending game over acknowledgement to remote player. Please wait...\n");

    sendByteRequestAsInteger((uint8_t)VERSION, socket_desc);
    sendByteRequestAsInteger((uint8_t)msg_code, socket_desc);
    sendByteRequestAsInteger((uint8_t)move, socket_desc);
    sendByteRequestAsInteger((uint8_t)turn, socket_desc);
}

void handleRemotePlayerInvalidMove(uint8_t turn, uint8_t choice, int socket_desc)
{
    printf("Error: Remote player used invalid move.\n");
    printf("Game will terminate now. Please wait...\n");
    sendRequestToServer(MSG_CODE_INVALID_MOVE, turn, choice, socket_desc);
    terminateGame(socket_desc);
}

/////////////////////////////////////////////////////////////////////////
/// Call this method to get next players move.
///     if next player is remote player, then get his move using socket.
///     if next player is local player, then use scanf to get his move.
/////////////////////////////////////////////////////////////////////////
int getNextMove(uint8_t turn, int* move, int socket_desc)
{
    int remotePlayerRequestedGameOver = 0;
    if (isMyTurn(turn))
    {
        printf("Your turn, enter a number:  ");     // print out player so you can pass game
        scanf("%d", move);                          //using scanf to get the choice
    }
    else
    {
        printf("Waiting for remote player's turn. Please wait...\n");
        struct msg remotePlayerResponse = getRemotePlayerResponse(socket_desc, turn);
        *move = remotePlayerResponse.move;
        remotePlayerRequestedGameOver = (remotePlayerResponse.responseCode == MSG_CODE_GAME_OVER) ? 1 : 0;
    }

    return remotePlayerRequestedGameOver;
}

void terminateGame(int socket_desc)
{
    closeConnection(socket_desc);
    exit(0);
}

/////////////////////////////////////////////////////////////
/// Method to validate command line arguments
///     If validation fails, then program will be teminated.
/////////////////////////////////////////////////////////////
void validateInput(int argc, char** argv)
{
    // cmdLine to execute: ftpc <remote-IP> <remote-port> <local-file-to-transfer>
    if (argc != 3)
    {
        fprintf(stderr, "%s\nusage: tictactoeOriginal <remote-IP> <remote-port>\n   %s\n   %s\n", "ERROR: Invalid input specified",
            "<remote-IP>    : ip address of the remote player", "<remote-port>  : port address of the remote player.");
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
}