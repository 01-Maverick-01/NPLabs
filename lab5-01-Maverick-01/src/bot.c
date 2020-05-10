/****************************************************************************************/
/* CSE 5462 - Lab 5                                                                     */
/*      This program provides defintion for various methods used by the tictactoe bot.  */
/*                                                                                      */
/* Name: Ishan Deep                                                                     */
/* Lname.#: deep.24                                                                     */
/*                                                                                      */
/* Created on Thu Feb 25 2020                                                           */
/*                                                                                      */
/****************************************************************************************/

#include <stdint.h>
#include "../include/utility.h"


/*********************************** Method Definitions ***********************************/
// public
int getBotMove(char board[ROWS][COLUMNS], uint8_t turn, char mark, int botLevel);
// internal methods
static char check(int cell, char board[ROWS][COLUMNS]);
static int getHardBotMove(char board[ROWS][COLUMNS], int turn, char mark, char opponentMark);
static int getFreeCell(char board[ROWS][COLUMNS]);
static int isFree(int cell, char board[ROWS][COLUMNS]);
static int isGoingToWin(char playerMark, char board[ROWS][COLUMNS], int checkIfTwoCellsFree);
static int checkIfCells(char cells[], int cellIndex[], int size, char playerMark, int checkIfTwoCellsFree);
/******************************************************************************************/

static char check(int cell, char board[ROWS][COLUMNS])
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

static int getHardBotMove(char board[ROWS][COLUMNS], int turn, char mark, char opponentMark)
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

static int getFreeCell(char board[ROWS][COLUMNS])
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

    return -1;
}

static int isFree(int cell, char board[ROWS][COLUMNS])
{
    int row, col;
    getCellIndex(&row, &col, cell);
    if (board[row][col] != 'X' && board[row][col] != 'O')
        return 1;

    return 0;
}

static int isGoingToWin(char playerMark, char board[ROWS][COLUMNS], int checkIfTwoCellsFree)
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

static int checkIfCells(char cells[], int cellIndex[], int size, char playerMark, int checkIfTwoCellsFree)
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