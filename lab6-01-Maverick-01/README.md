# Network Programming - Lab6

This program implements server end of tictactoe game and uses Datagram socket to communicate with the client. The client and server play against each other and server is always player 1 and client is player 2. This version of the server can handle multiple clients at the same time and can account for lost/duplicate packets due to network issues. The server can also handle clients with older version of protocol (least supported version is 1). 
* **Clients with Version 1 and 2:** server will not support handling of lost/duplicate packets and incase of inactivity (TIMEOUT x MAX_RETRY) game session will be terminated with those clients.
* **Clients with Version 3:** server will support handling of lost/duplicate packets and in case of such events it will resend the last sent packet. However, if client is inactive for more than TIMEOUT x MAX_RETRY, game session will be terminated.

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
* **TIMEOUT&nbsp; &nbsp; &nbsp; &nbsp;:** Controls the number of seconds after recvFrom time outs.
* **MAX_RETRY :** Controls the maximum number of retries for each client.
* **MAX_GAME &nbsp;:** Controls the maximum number of clients that can connect to the game.

### Tested with client(s)
* **Nithin Senthil Kumar** (senthilkumar.16)
* **Frank Cerny** (cerny.25)
* **Xueyin Yin** (yin.622)

