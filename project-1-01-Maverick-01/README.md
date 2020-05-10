# Network Programming - Project

This program implements server end of tictactoe game and uses Datagram socket to communicate with the client. The client and server play against each other and server is always player 1 and client is player 2. This version of the server can handle multiple clients at the same time and supports game resume requests from clients. This version of server only supports clients with VERSION 4 and does not support older versions of the client.

### Environment: [ stdlinux.cse.ohio-state.edu ]

### Build Instruction
* Clone repo or download zip file and extract it.
* Launch terminal in the repository folder.
* Execute cmd: make

### Execution
**cmdLine: tictactoeServer &lt;port-number&gt; &lt;bot-lvl&gt;** <br />
*port-number*&nbsp;: port number the server is using. <br />
*bot-lvl* &nbsp; &nbsp;&nbsp; &nbsp; &nbsp; &nbsp;: Specify 1 to use easy bot, 2 for medium and 3 to use hard bot.

### Important #defines in tictactoeServer.c
* **TIMEOUT&nbsp; &nbsp; &nbsp; &nbsp;:** Controls the number of seconds after select() time outs.
* **MULTICAST_IP :** IP address for multicast socket.
* **MULTICAST_PORT :** Port number for multicast socket.
* **MAX_GAME &nbsp;:** Controls the maximum number of clients that can connect to the game.
