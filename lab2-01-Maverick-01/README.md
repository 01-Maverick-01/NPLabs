# Network Programming - Lab2

This repo has my implementation for Lab2 where we have to tranfer a file to server specified by user using socket.

### Environment: [ OS: Ubuntu 18.04.3 LTS; Compiler: gcc (7.4.0); GNU Make (4.1) ]

### Build Instruction
* Launch terminal in the repository folder.
* Execute cmd: make

### Execution
**cmdLine: ftpc &lt;ip-address&gt; &lt;port-number&gt; &lt;file-to-transfer&gt;** <br />
*ip-address*: ip address of the server. <br />
*port-number*: port number the server is using. <br />
*file-to-transfer*: path (relative/absolute) of the file to be transferred. <br />

### Implementation Logic:
* Validate cmdLine args. If failed, terminate execution.
* Open input file handle in binary read mode. If failed, display error and terminate execution.
* Establish connection using ip address and port number. If failed, display error and terminate execution.
* Tranfer the message.
    * Tranfer file size (4 bytes).
    * Tranfer delimiter (single space).
    * Tranfer filename (20 bytes char array padded on the right using spaces).
    * Tranfer the file contents in 100 bytes increment.
* Parse the reponse received from server.
    * Read the 4 byte response code.
    * Read the delimiter. If delimiter is not space, then handle error.
    * Read the 4 bytes file size that was received by the server.
    * Handle errors. If error occurred, then prompt the user for a retry.
* Close connection
* Close input file handle.
* Terminate execution.
