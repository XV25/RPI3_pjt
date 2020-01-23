#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include<sys/types.h>

#include "./lib/comm.h"

int main()
{
 
    //pid_t pid;
    int id;
    char msg[255];//variable qui contiendrat les messages
 
    
 
    if (strcmp(msg, "aurevoir") != 0)
    {
        memset(msg, 0, 255);
        // recv(socketID, msg, 255, 0);
        printf ("%s\n", msg);
    }
 
    do
    {
        id+=1;
        printf ("moi : ");
        fgets(msg, 255, stdin);// le client ecrit son message
        msg[strlen(msg) - 1] = '\0';
 
        // if ((send(socketID, msg, strlen(msg), 0)) == -1)
        //     perror("send");
        // recv(socketID, msg, 255, 0);
        printf ("Phrase reçue : %s\n", msg);
 
    }
    while (strcmp(msg, "aurevoir") != 0);    // tant que le client n'envoie pas "aurevoir" la conversation n'est pas fini
 
    // shutdown(socketID, SHUT_RDWR);// fermeture du socket
 
    return 0;
 
}