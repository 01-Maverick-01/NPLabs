/****************************************************************************************/
/* CSE 5462 - Lab 5                                                                     */
/*      This file is provides definiation for various utility methods that are used     */
/*      for tictactoe server implementation.                                            */
/*                                                                                      */
/* Name: Ishan Deep                                                                     */
/* Lname.#: deep.24                                                                     */
/*                                                                                      */
/* Created on Thu Feb 25 2020                                                           */
/*                                                                                      */
/****************************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "../include/utility.h"

void    printMoveOnBoard(char cellValue, char srvrMark, char clientMark);

int isNum(char* number)
{
    for (int i = 0; i < strlen(number); i++)
    {
        if (!isdigit(number[i]))
            return 0;
    }
    return 1;
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

///////////////////////////////////////////////////////
/// Call this method to print error msgs in red color.
///////////////////////////////////////////////////////
void printError(char *errMsg, ...)
{
    printf("\033[1;31m");
    va_list args;
    va_start(args, errMsg);
    char updatedFormat[500];
    sprintf(updatedFormat, "%s\n", errMsg);
    vprintf(updatedFormat, args);
    va_end(args);
    printf("\033[0m");
}

//////////////////////////////////////////////////////////////
/// Call this method to print send/recved msgs in cyan color.
//////////////////////////////////////////////////////////////
void printSendRecvMsg(char *msg, ...)
{
    printf("\033[0;36m");
    va_list args;
    va_start(args, msg);
    char updatedFormat[100];
    sprintf(updatedFormat, "\t--| %s |--\n", msg);
    vprintf(updatedFormat, args);
    va_end(args);
    printf("\033[0m");
}

//////////////////////////////////////////////////////////////
/// Call this method to print send/recved msgs in green color.
//////////////////////////////////////////////////////////////
void printGameMsg(char *msg, ...)
{
    printf("\033[1;32m");
    va_list args;
    va_start(args, msg);
    char updatedFormat[500];
    sprintf(updatedFormat, "  --> %s\n", msg);
    vprintf(updatedFormat, args);
    va_end(args);
    printf("\033[0m");
}

//////////////////////////////////////////////////////////////
/// Call this method to print send/recved msgs in red color.
//////////////////////////////////////////////////////////////
void printGameErr(char *errMsg, ...)
{
    printf("\033[0;31m");
    va_list args;
    va_start(args, errMsg);
    char updatedFormat[500];
    sprintf(updatedFormat, "  *** Game Error: %s. Terminating game...\n", errMsg);
    vprintf(updatedFormat, args);
    va_end(args);
    printf("\033[0m");
}

/*****************************************************************/
/* brute force print out the board and all the squares/values    */
/*****************************************************************/
void print_board(char board[ROWS][COLUMNS], char *remoteAddr, char srvrMark)
{
    char lineStart[10] = "\t*    ";
    printf("\t*-----------------------------------------------------------------*\n");
    char clientMark = srvrMark == SERVER_MARK ? CLIENT_MARK : SERVER_MARK;    
    for (int i = 0; i < 3; i++)
    {
        printf("%s     |     |     %44s*\n", lineStart, " ");
        for (int j = 0; j < 3; j++)
        {
            if (j == 0)
                printf("%s", lineStart);

            printMoveOnBoard(board[i][j], srvrMark, clientMark);
            if (j < 2)
                printf("|");
            else if (i == 0)
                printf("\tServer (%c)%32s*\n", srvrMark, " ");
            else if (i == 1)
                printf("\tRemote Player (%c) : '%s'%s*\n", clientMark, remoteAddr, " ");
            else
                printf("%44s*\n", " ");       
        }
        if (i < 2)
            printf("%s_____|_____|_____%44s*\n", lineStart, " ");
        else
            printf("%s     |     |     %44s*\n", lineStart, " ");
    }
    printf("\t*-----------------------------------------------------------------*\n");
}

/////////////////////////////////////////////////////////////////////
/// This method prints player moves board in their respective color.
/////////////////////////////////////////////////////////////////////
void printMoveOnBoard(char cellValue, char srvrMark, char clientMark)
{
    if (cellValue == srvrMark)                   // remote player is bold green
        printf("\033[1;32m");
    else if (cellValue == clientMark)            // local player is bold blue
        printf("\033[1;34m");
    
    printf("  %c  ", cellValue);
    printf("\033[0m");                           // reset color
}