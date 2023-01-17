1. Run "gcc server.c -o server" to compile server.c.
2. Run "gcc client.c -o client" to compile client.c.
3. Run "./server <port_number>" to start a server.
4. Run "./client localhost <port_number>" to send packets from client.

The client will send four requests. The first subscriber number will not be found in database. The second subscriber number will be found, but the technology will not match. The third subscriber hasn't paid. And the last one will be permitted to access the network.

To test ack_timer, run "./client localhost <port_number>" before starting the server.
