1. Run "gcc server.c -o server" to compile server.c.
2. Run "gcc client.c -o client" to compile client.c.
3. Run "./server <port_number>" to start a server.
4. Run "./client localhost <port_number>" to send packets from client.

The client send five correct packets and receive five ACK from server. After that, the client send one correct packet and four packets with errors. After each incorrect packet, a correct version of the same packet will be sent. The server send ACK for correct packets, and corresponding reject sub codes for packets with errors.

To test ack_timer, run "./client localhost <port_number>" before starting the server.
