/****************************************************************************************/
/* CSE 5462 - Lab 5                                                                     */
/*      This file is the header for various utility methods that are used for           */
/*      tictactoe server implementation.                                                */
/*                                                                                      */
/* Name: Ishan Deep                                                                     */
/* Lname.#: deep.24                                                                     */
/*                                                                                      */
/* Created on Thu Feb 25 2020                                                           */
/*                                                                                      */
/****************************************************************************************/

#ifndef _utility_h
#define _utility_h

#include <stdint.h>

#define ROWS 3
#define COLUMNS 3

void    getCellIndex(int* row, int* col, uint8_t choice);
void    printError(char *errMsg, ...);
void    printSendRecvMsg(char *msg, ...);
void    printGameMsg(char *msg, ...);
void    printGameErr(char *errMsg, ...);
int     isNum(char* number);
void    print_board(char board[ROWS][COLUMNS], char *remoteAddr, char srvrMark);

#endif