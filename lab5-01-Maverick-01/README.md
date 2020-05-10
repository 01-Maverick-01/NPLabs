# Network Programming - Lab5

This repo has my implementation for Lab4 where we have to write the server end of tictactoe game. The server will communicate with multiplt clients using datagram. This version of server will also work with client written as part of lab4.

### Environment: [ OS: Ubuntu 18.04.3 LTS; Compiler: gcc (7.4.0); GNU Make (4.1) ]

### Build Instruction
* Launch terminal in the repository folder.
* Execute cmd: make

### Execution
**cmdLine: tictactoeServer &lt;port-number&gt; &lt;bot-lvl&gt;** <br />
*port-number*&nbsp;: port number the server is using. <br />
*bot-lvl* &nbsp; &nbsp;&nbsp; &nbsp; &nbsp; &nbsp;: Specify 1 to use easy bot, 2 for medium and 3 to use hard bot.

### Implementation Logic:
* Validate cmdLine args. If failed, terminate game.
* Create and bind to datagram socket. If failed, terminate game.
* Listen to incoming requests on datagram socket. If received request is invalid then decline request/end game, else:
  * if new game request recevied, then
    * if new game slot is available [and client is not already connected(only if protocol VERSION 1)], then create new game and send server move to remote player.
    * else, send server busy response code
  * if continue request recvd, get game number from remote player request [or ipAddr/port in case of VERSION 1].
    * if game number not found, return send invalid request return code to client.
    * else
      * if remote player sent an game complete acknowledgement, then end current game
      * else, validate move and turn number. 
        * If found invalid, send appropriate return code to remote player.
        * else register move and check if remote player won. 
          * if won, then send game over acknowledgement to remote player.
          * else, get next move from bot, register move and check if game is over.
            * if won, then send game over msg with last move to remote player
            * else, just send the last move to remote player.
