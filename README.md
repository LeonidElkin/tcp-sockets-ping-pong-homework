# TCP Sockets ping-pong homework
This program demonstrates inter-process synchronization using TCP sockets
on a UNIX-like system. The application creates two processes via fork():

* Process A acts as a TCP server. It starts in the READY state, performs
    some simulated work, sends a "PING" message to Process B, and then
    switches to the SLEEP state while waiting for a reply.

* Process B acts as a TCP client. It starts in the SLEEP state, connects
    to Process A, waits for the "PING" message, switches to READY, performs
    its own simulated work, and replies with "PONG".

The two processes exchange these messages in a ping-pong pattern for a
fixed number of iterations. This demonstrates how a reliable full-duplex
TCP connection can be used as a synchronization primitive between separate
processes, ensuring ordered message delivery and deterministic turn-taking.
