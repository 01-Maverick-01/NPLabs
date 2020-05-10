#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define READ_BUFFER_LEN 80

/********************* Helper Method to display text to file and console ********************/
void displayOutput(char* text, FILE* outFileHandle)
{
    printf("%s", text);
    if (outFileHandle != NULL)
    {
        fwrite(text, strlen(text), 1, outFileHandle);
    }
}

void displayUnformattedLongOutput(char* unformattedText, long value, FILE* outFileHandle)
{
    char *msg = (char*)malloc(200 * sizeof(char));
    sprintf(msg, unformattedText, value);
    displayOutput(msg, outFileHandle);
    free(msg);
}

void displayUnformattedCharOutput(char* unformattedText, char* value, FILE* outFileHandle)
{
    char *msg = (char*)malloc(200 * sizeof(char));
    sprintf(msg, unformattedText, value);
    displayOutput(msg, outFileHandle);
    free(msg);
}
/*******************************************************************************************/


/////////////////////////////////////////////////////////////////////////////////////////
/// Method returns file handle for input/output file 
/// This method displays appropriate msgs if an error occurred while getting file handle.
/////////////////////////////////////////////////////////////////////////////////////////
FILE* getHandle(char* mode, char* filePath, int isInputFile, FILE* outFile)
{
    FILE *fp = fopen(filePath, mode);
    if (fp == NULL)
    {
        displayOutput("Failed to get file handle: ", outFile);
        if (isInputFile)
            displayUnformattedCharOutput("Input file '%s' does not exist\n", filePath, outFile);
        else
            displayUnformattedCharOutput("Unable to create/overwrite file '%s'. Make sure write permission is granted\n", filePath, outFile);
    }
    return fp;
}


//////////////////////////////////////////////////////// 
/// Method to validate command line arguments
/// If validation fails, then program will be teminated.
////////////////////////////////////////////////////////
void validateInput(int argc, char **argv)
{
    // cmdLine to execute: count <input-filename> <search-string> <output-filename>
    if (argc != 4)
    {
        fprintf(stderr, "%s\nusage: count <input-filename> <search-string> <output-filename>\n   %s\n   %s\n   %s\n", "ERROR: Invalid input specified",
        	"<input-filename> : path to the input file (absolute/relative).", "<search-string>  : string that needs to be searched.",
                "<output-filename>: path to the output file (absolute/relative).");
        exit(0);
    }
}

////////////////////////////////////////////////////////////////
/// Method to get count of pattern occurrence in current buffer
////////////////////////////////////////////////////////////////
int getCountInCurrentBuffer(unsigned char pattern[], unsigned char readBuffer[], int bufferLen)
{
    int count = 0;
    unsigned char* index;
    do
    {
        index = memchr(readBuffer, pattern[0], bufferLen);
        if (index != NULL)
        {
            if (memcmp(index, pattern, strlen(pattern)) == 0)
                count++;
            readBuffer = index + 1;
        }
    } while (index != NULL);
    
    return count;
}

/////////////////////////////////////////////////////////////////////////
/// Method to process input file and display file size and pattern count.
/// This method read input file in increments of 80 bytes and then calls
/// 'getCountInCurrentBuffer' method to get pattern occurrence count. To
/// handle the scenario where pattern is split across two buffers, we
/// concat the last (patternLen-1) bytes from previous buffer to current.
/////////////////////////////////////////////////////////////////////////
void processFile(FILE *inputHandle, char pattern[], FILE *outputHandle)
{
    long count = 0, size = 0;
    int patternLen = strlen(pattern)-1;
    unsigned char *prevResidualBuff  = (char*)malloc(1 * sizeof(char));
    prevResidualBuff[0] = '\0';
    unsigned char tempReadBuffer[READ_BUFFER_LEN];
    
    while(!feof(inputHandle))
    {
        int readBufferLen = fread(tempReadBuffer, 1, READ_BUFFER_LEN, inputHandle);
        size += readBufferLen;
        
        if (strlen(prevResidualBuff) > 0)
        {
            int finalBufferLen = readBufferLen + patternLen;
            unsigned char *finalBuffer = (char*)malloc((finalBufferLen) * sizeof(char));
            memcpy(finalBuffer, &prevResidualBuff[0], patternLen);                          // copy contents of residualBuffer to finalBuffer
            memcpy(&finalBuffer[patternLen], tempReadBuffer, readBufferLen);                // copy contents of currentReadBuffer to finalBuffer
            count += getCountInCurrentBuffer(pattern, finalBuffer, finalBufferLen);
            free(finalBuffer);
        }
        else
            count += getCountInCurrentBuffer(pattern, tempReadBuffer, readBufferLen);
        
        free(prevResidualBuff);
        
        // move last 'strlen(pattern)-1' bytes of current buffer to residual buffer. 
        prevResidualBuff = (char*)malloc((patternLen + 1) * sizeof(char));
        int chopIndex = readBufferLen - patternLen;
        memcpy(prevResidualBuff, &tempReadBuffer[chopIndex], patternLen);
        prevResidualBuff[patternLen] = '\0';
    }

    displayUnformattedLongOutput("Size of file is %ld\n", size, outputHandle);
    displayUnformattedLongOutput("Number of matches = %ld\n", count, outputHandle);
}

int main(int argc, char **argv)
{
    validateInput(argc, argv);

    FILE *out = getHandle("wb", argv[3], 0, NULL);
    if (out != NULL)
    {
        FILE *input = getHandle("rb", argv[1], 1, out);
        if (input != NULL)
        {
            processFile(input, argv[2], out);
            fclose(input);
        }
        fclose(out);
    }
    return 0;
}
