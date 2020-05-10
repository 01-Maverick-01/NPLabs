# Network Programming - Lab3

This repo has my implementation for Lab3 where we have to write the client end of tictactoe game. The client will communicate with server using stream socket.

### Environment: [ OS: Ubuntu 18.04.3 LTS; Compiler: gcc (7.4.0); GNU Make (4.1) ]

### Build Instruction
* Launch terminal in the repository folder.
* Execute cmd: make

### Execution
**cmdLine: tictactoeOriginal &lt;ip-address&gt; &lt;port-number&gt;** <br />
*ip-address*: ip address of the server. <br />
*port-number*: port number the server is using. <br />

### Implementation Logic:
* Validate cmdLine args. If failed, terminate game.
* Eastablish connection with server using ip address and port number. If failed, display error and terminate game.
* Initialize game board and start game.
* Until someone wins or no more moves possible, do following:
  * Get next move from player.
    * If local players move, then prompt user for the move using CLI.
    * if remote players move, then use stream socket to get move (4 byte msg) from server.
  * Validate player move.
    * If validation fails then do following:
      * If it was local players turn, then prompt him again for the move.
      * If it was remote players turn, then send him error msg (4 byte msg) to server and terminate game.
    * If validation passes then commit move (and send msg to server if it was local players turn) and then change turn to next player.
* Once game is over, do the following: 
  * If game ended in your turn, send msg to server and after receiving acknowledgement terminate game.
  * If game ended in remote player turn, send ACK to server if it was fair game else send error and then terminate game.
