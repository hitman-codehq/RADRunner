
#ifndef SERVERCOMMANDS_H
#define SERVERCOMMANDS_H

class RSocket;

void ExecuteServer(RSocket &a_socket, struct SCommand *a_command);

void ReceiveFile(RSocket &a_socket, struct SCommand *a_command);

#endif /* ! SERVERCOMMANDS_H */
