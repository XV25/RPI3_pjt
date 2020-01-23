#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include<sys/types.h>
#include "comm.h"

void initComm(){
    struct sockaddr_in informations;  //structure donnant les informations sur le serveur
 
    /*initialisation du protocole, TCP  l'adresse de connection 127.0.0.1 (en local) et du port du serveur (1400)*/
    informations.sin_family = AF_INET;
    informations.sin_port = htons(2500);
    informations.sin_addr.s_addr = inet_addr("127.0.0.1");
 
    int socketID = socket(AF_INET, SOCK_STREAM, 0); // creation du socket propre au client
 
    if (socketID == -1)    //test de création du socket
    {
        perror("socket");
        exit (-1);
    }
 
    if ((connect(socketID, (struct sockaddr *) &informations, sizeof(struct sockaddr_in))) == -1)   //connexion au serveur
    {
        perror("connect");
        exit (-1);
    }
}

/*
*
*
*/
void sendMessage(char * msg){

}

/*
*
*
*/
void sendData(char * data){

}