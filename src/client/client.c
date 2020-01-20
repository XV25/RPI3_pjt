#include <errno.h>
#include <sys/types.h>

#include "./lib/comm.h"

int main()
{
    int socketID = initComm();
    //pid_t pid;
    int id;
    char msg[255];//variable qui contiendrat les messages
 
    
 
    if (strcmp(msg, "aurevoir") != 0)
    {
        memset(msg, 0, 255);
        recv(socketID, msg, 255, 0);
        printf ("%s\n", msg);
    }
 
    do
    {
        id+=1;
        printf ("moi : ");
        fgets(msg, 255, stdin);// le client ecrit son message
        msg[strlen(msg) - 1] = '\0';
 
        if (envoyerMessage(socketID, INFO, msg) == 0){
            printf("\nMessage envoye");
        } else {
            printf("\nErreur");
        }
        recv(socketID, msg, 255, 0);
        printf ("Phrase reçue : %s\n", msg);
 
    }
    while (strcmp(msg, "aurevoir") != 0);    // tant que le client n'envoie pas "aurevoir" la conversation n'est pas fini
 
    // shutdown(socketID, SHUT_RDWR);// fermeture du socket
 
    return 0;
 
}