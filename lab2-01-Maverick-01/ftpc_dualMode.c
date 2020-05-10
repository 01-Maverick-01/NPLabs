#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define IP_ADDR 1
#define PORT 2
#define FILE_TO_TRANSFER 3
#define MODE 4
#define READ_BUFFER_LEN 100

int CLIENT_MODE = 0;

int isNum(char* number)
{
    for (int i = 0; i < strlen(number); i++)
    {
        if (!isdigit(number[i]))
            return 0;
    }
    return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////
/// Method returns file handle for input/output file 
/// This file open operation fails, program is terminated.
/////////////////////////////////////////////////////////////////////////////////////////
FILE* getHandle(char* filePath)
{
    FILE *fp = fopen(filePath, "rb");
    if (fp == NULL)
    {
        printf("Input file '%s' does not exist\n", filePath);
        exit(0);
    }
    return fp;
}

void closeFileHandle(FILE* fileHandle)
{
    printf("Releasing file handle. Please wait...\n");
    if (fileHandle != NULL)
    {
        fclose(fileHandle);
        fileHandle = NULL;
    }
    printf("Released file handle.\n");
}


////////////////////////////////////////////////////////
/// Given a filepath, this method returns the filename.
////////////////////////////////////////////////////////
char* getFileName(char* filePath)
{
    char *fileName = NULL;
    if (filePath)
        fileName = strrchr(filePath, '/') + 1;

    return fileName;
}

/////////////////////////////////////////////////////////////////////////////////////
/// Given a filepath, this method returns the filename which is 20 bytes long.
/// If name is less than 20 bytes, then it is padded on the right using spaces(" ").
/////////////////////////////////////////////////////////////////////////////////////
void getPaddedFileName(char *filePath, char *paddedFileName)
{
    if (filePath)
        filePath = strrchr(filePath, '/') + 1;
    
    for (int i = 0; i < 21; i++)
    {
        if (i < strlen(filePath))
            paddedFileName[i] = filePath[i];
        else if (i == 20)
            paddedFileName[i] = '\0';
        else
            paddedFileName[i] = ' ';
    }
}

///////////////////////////////////////////////////////////////
/// Given a file pointer, this method returns 4 byte file size
///////////////////////////////////////////////////////////////
uint32_t getFileSize(FILE* fp)
{
    fseek(fp, 0L, SEEK_END);
    uint32_t size = ftell(fp);
    rewind(fp);
    return size;
}

/********************************************* Socket Tranfer methods *********************************************/

void sendLongOverSocket(uint32_t number, int socket_desc)  // Call this method to tranfer a number over socket
{
    int rc = send(socket_desc, &number, sizeof(number), 0);
    if (rc < 0)
        perror("sendto");
}

void sendStringOverSocket(char* msg, int socket_desc)  // Call this method to tranfer a strings over socket
{
    char s[100];
    strcpy(s, msg);
    int rc = send(socket_desc, &s, strlen(s), 0);
    if (rc < 0)
        perror("sendto");
}

void sendFileOverSocket(FILE* fileHandle, int socket_desc)  // Call this method to tranfer a strings over socket
{
    printf("Starting file transfer. Please wait...\n");
    unsigned char tempReadBuffer[READ_BUFFER_LEN];
    uint32_t bytesSent = 0;
    while(!feof(fileHandle))
    {
        int readBufferLen = fread(tempReadBuffer, 1, READ_BUFFER_LEN, fileHandle);
        int rc = send(socket_desc, &tempReadBuffer, readBufferLen, 0);
        if (rc < 0)
            perror("sendto");
        bytesSent += rc;
    }
    printf("File sent to server. Total bytes sent is %d\n", bytesSent);
}

uint32_t readLongResponse(int socket_desc)  // call this method to read a 32bit int response from server
{
    uint32_t response;
    while (recv(socket_desc, &response, 4, MSG_PEEK) < 4);  // wait unit 4 bytes is present in stream
    int rc = recv(socket_desc, &response, 4, 0);
    if (rc != 4)
        perror("invalid read");
    return ntohl(response);
}

char readCharOverSocket(int socket_desc)  // Call this method to read a char sent by server
{
    char delimiter;
    while (recv(socket_desc, &delimiter, 1, MSG_PEEK) < 1); // wait unit 1 byte is present in stream
    int rc = recv(socket_desc, &delimiter, 1, 0);
    if (rc != 4)
        perror("invalid read");
    return delimiter;
}

/******************************************************************************************************************/

///////////////////////////////////////////////
// Call this method to handle server response.
///////////////////////////////////////////////
int handleResponseAndPromptRetry(uint32_t errorCode, uint32_t fileSizeReceivedByServer, int errorParsingResponse)
{
    int retry = 0;
    if (errorParsingResponse == 0)
    {
        switch (errorCode)
        {
        case 0: printf("File tranferred complete. Total bytes received on server is %d\n", fileSizeReceivedByServer); break;
        case 100: printf("File tranferred failed: Extra bytes received on server.\n\tTotal bytes received on server is %d\n", fileSizeReceivedByServer); break;
        case 200: printf("File tranferred failed: Less bytes received on server.\n\tTotal bytes received on server is %d\n", fileSizeReceivedByServer); break;
        case 300: printf("File tranferred failed: Error occurred while saving file on server.\n"); break;
        case 400: printf("File tranferred failed: Unknown server error.\n"); break;
        case 500: printf("File tranferred failed: Incorrect request format used to tranfer file\n"); break;
        default: printf("Unknown response received from server\n"); break;
        }
    }
    else
    {
        printf("Error occurred while parsing server response.\n");
    }
    
    if (errorCode > 0 || errorParsingResponse == 1)
    {
        printf ("Do you want to retry? Press Y to retry N to terminate\n");
        int selection = getchar();
        getchar();  // eat up the extra newline
        retry = (selection == 'Y' || selection == 'y') ? 1 : 0;
    }

    return retry;
}

//////////////////////////////////////////////////////// 
/// Method to validate command line arguments
/// If validation fails, then program will be teminated.
////////////////////////////////////////////////////////
void validateInput(int argc, char** argv)
{
    // cmdLine to execute: ftpc <remote-IP> <remote-port> <local-file-to-transfer>
    if (argc != 4 || argc != 5)
    {
        fprintf(stderr, "%s\nusage: ftpc <remote-IP> <remote-port> <local-file-to-transfer>\n   %s\n   %s\n   %s\n", "ERROR: Invalid input specified",
        	"<remote-IP> : ip addressof the server", "<remote-port>  : port address of the server.",
                "<local-file-to-transfer>: path (absolute/relative) of the file that has to be transferred.");
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
    if (isNum(argv[PORT]) == 0)
    {
        fprintf(stderr, "ERROR: Invalid input specified: port number '%s' is invalid. Port number can only be a number.\n", argv[PORT]);
        exit(0);
    }

    // validate filePath
    if (strlen(argv[FILE_TO_TRANSFER]) == 0 || strlen(getFileName(argv[FILE_TO_TRANSFER]))> 20) 
    {
        if (strlen(argv[FILE_TO_TRANSFER]) == 0)
            fprintf(stderr, "ERROR: Invalid input specified: Filepath cannot be empty.\n");
        else
            fprintf(stderr, "ERROR: Transfer file name '%s' cannot exceed 20 characters.\n", getFileName(argv[FILE_TO_TRANSFER]));
        
        exit(0);
    }

    // validate mode flag
    if (atoi(argv[MODE]) != 1 || atoi(argv[MODE]) != 0)
    {
        fprintf(stderr, "ERROR: Invalid input specified: Mode '%s' can only have a value of 0 or 1.\n", argv[PORT]);
        exit(0);
    }
}

void establishConnection(int socket, char *ipAddr, long portNumber, int displayMsgs)
{
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNumber);
    serverAddr.sin_addr.s_addr = inet_addr(ipAddr);
    if (displayMsgs == 1)
        printf("Connecting to server at '%s' using port '%ld'. Please wait...\n", ipAddr, portNumber);
    if(connect(socket, (struct sockaddr*)&serverAddr, sizeof(struct sockaddr_in)) != 0) 
    {
        close(socket);
        perror("Error connecting stream socket");
        exit(1);
    }
    if (displayMsgs == 1)
        printf("Connection Successful. Preparing to send file...\n");
}

void closeConnection(int socket, int displayMsgs)
{
    if (displayMsgs == 1)
        printf("Closing connection to server. Please wait...\n");
    close(socket);
    if (displayMsgs == 1)
        printf("Connection closed.\n");
}

int resetConnection(int socket_desc, char* serverIP, long portNumber)
{
    closeConnection(socket_desc, 0);
    int new_socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    establishConnection(new_socket_desc, serverIP, portNumber, 0);
    return new_socket_desc;
}

int initiateFileTransfer(char** argv, FILE* fileToTransfer, int socket_desc)
{

    char paddedFileName[21];
    getPaddedFileName(argv[FILE_TO_TRANSFER], paddedFileName);

    sendLongOverSocket(htonl(getFileSize(fileToTransfer)), socket_desc);    // send header (4 byte file size)
    sendStringOverSocket(" ", socket_desc);                                 // send delimiter (single space)
    sendStringOverSocket(paddedFileName, socket_desc);                      // send file name (20 bytes)
    sendStringOverSocket(" ", socket_desc);                                 // send delimiter (single space)
    sendFileOverSocket(fileToTransfer, socket_desc);                        // send file data

    uint32_t errCode = readLongResponse(socket_desc);                       // receive error code (4 bytes) from server
    if (' ' != readCharOverSocket(socket_desc))                             // receive delimiter (single space)
        return handleResponseAndPromptRetry(-1, -1, 1);                     // handle error occurred during parsing server response

    uint32_t fileSize = readLongResponse(socket_desc);                      // receive file size (4 bytes) from server
    return handleResponseAndPromptRetry(errCode, fileSize, 0);              // handle any other error, if occurred
}

void shutdownClient(FILE* fp, int socket_desc)
{
    printf("\n\n------------------------------------------------------------\n");
    printf("Initiating Client shutdown. Please wait...\n");
    closeConnection(socket_desc, 1); 
    closeFileHandle(fp);
    printf("Client shutdown complete.\n");
    printf("------------------------------------------------------------\n");
}

void runInReleaseMode(char** argv, FILE* fileToTransfer, int socket_desc, char* serverIP, long portNumber)
{
    int retryCount = 0;
    int retry = 0;      // in case error occurred, we want to retry
    do
    {
        retry = initiateFileTransfer(argv, fileToTransfer, socket_desc);
        if (retry == 1 && retryCount <= 5)
        {
            retryCount++;
            rewind(fileToTransfer);
            socket_desc = resetConnection(socket_desc, serverIP, portNumber);
        }
        else
        {
            if (retryCount > 0)
                printf("File tranfer failed after '%d' retires.\n", retryCount);
            retry = 0;
        }
            
    } while (retry == 1);    
}

int initiateFileTransferDebugMode(char** argv, FILE* fileToTransfer, int socket_desc, int errorType)
{

    char paddedFileName[21];
    if (errorType == 3)
        strcpy(paddedFileName, "./abc/p/invalid    ");
    else if (errorType == 4)
        strcpy(paddedFileName, "./abc/p/invalid");
    else
        getPaddedFileName(argv[FILE_TO_TRANSFER], paddedFileName);
    
    if (errorType == 1)
        sendLongOverSocket(htonl(getFileSize(fileToTransfer))/2, socket_desc);    // send incorrect size(lesser) (4 byte file size)
    else if (errorType == 2)
        sendLongOverSocket(htonl(getFileSize(fileToTransfer))*2, socket_desc);    // send incorrect size(larger) (4 byte file size)
    else
        sendLongOverSocket(htonl(getFileSize(fileToTransfer))*2, socket_desc);    // send correct size
    
    sendStringOverSocket(" ", socket_desc);                                 // send delimiter (single space)
    sendStringOverSocket(paddedFileName, socket_desc);                      // send file name (20 bytes)
    sendStringOverSocket(" ", socket_desc);                                 // send delimiter (single space)
    sendFileOverSocket(fileToTransfer, socket_desc);                        // send file data

    uint32_t errCode = readLongResponse(socket_desc);                       // receive error code (4 bytes) from server
    if (' ' != readCharOverSocket(socket_desc))                             // receive delimiter (single space)
        return handleResponseAndPromptRetry(-1, -1, 1);                     // handle error occurred during parsing server response

    uint32_t fileSize = readLongResponse(socket_desc);                      // receive file size (4 bytes) from server
    return handleResponseAndPromptRetry(errCode, fileSize, 0);              // handle any other error, if occurred
}

void runInDebugMode(char** argv, FILE* fileToTransfer, int socket_desc, char* serverIP, long portNumber)
{
    printf("Running Client in debug mode");
    while(1)
    {
        printf("1. Send additional bytes\n");
        printf("2. Send less bytes\n");
        printf("3. Send invalid fileName\n");
        printf("4. Send msg in invalid protocol\n");
        printf("Anyother input to exit\n");
        int response = getchar();
        getchar();  //absorb extra newline input
        switch (response)
        {
        case '1': initiateFileTransferDebugMode(argv, fileToTransfer, socket_desc, 1); break;
        case '2': initiateFileTransferDebugMode(argv, fileToTransfer, socket_desc, 2); break;
        case '3': initiateFileTransferDebugMode(argv, fileToTransfer, socket_desc, 3); break;
        case '4': initiateFileTransferDebugMode(argv, fileToTransfer, socket_desc, 4); break;       
        default: exit(1); break;
        }
    }   
}

int main(int argc, char **argv)
{
    validateInput(argc, argv);
    CLIENT_MODE = atoi(argv[MODE]);
    
    FILE* fileToTransfer = getHandle(argv[FILE_TO_TRANSFER]);
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    char serverIP[16];
    long portNumber = strtol(argv[PORT], NULL, 10);
    strcpy(serverIP, argv[IP_ADDR]);
    establishConnection(socket_desc, serverIP, portNumber, 1);                 // establish connection

    if (CLIENT_MODE == 1)
        runInDebugMode(argv, fileToTransfer, socket_desc, serverIP, portNumber);
    else
        runInReleaseMode(argv, fileToTransfer, socket_desc, serverIP, portNumber);

    shutdownClient(fileToTransfer, socket_desc);
    return 0;
}
