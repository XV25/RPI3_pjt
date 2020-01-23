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

int envoyerResolution(int socketID, int x, int y){
    char * charx = malloc(sizeof(4));
    char * chary = malloc(sizeof(4));
    sprintf(charx, "%d", x);
    sprintf(chary, "%d", y);

    char * res = concat(charx, "/");
    res = concat(res, chary);

    return envoyerMessage(socketID, CMD_DEFINIR_RESOLUTION, res);
} 

int initComm(){
    //structure donnant les informations sur le serveur et sur le client
    struct sockaddr_in information_server;
    struct sockaddr_in information_client;
 
    int socketID = socket(AF_INET, SOCK_STREAM, 0);
    int connexion = 0;
    int pid;
    int id;
    char msg[255];
 
    id=0;
    socklen_t len = sizeof(struct sockaddr_in); //déclaration d' une variable du type socklen_t qui contiendra la taille de la structure
 
    if (socketID == -1)
    {
        perror("socket");
        exit(-1);
    }
 
    /*initialisation du protocole, TCP  l'adresse de connection 127.0.0.1 (en local) et du port du serveur (1400)*/
    memset(&information_server, 0, sizeof(struct sockaddr_in));
    information_server.sin_port = htons(2500);
    information_server.sin_family = AF_INET;
 
    /* création de la connexion*/
    if ((bind(socketID, (struct sockaddr *) &information_server, sizeof(struct sockaddr))) == -1)
    {
        perror("bind");
        exit(-1);
    }
 
    /* le serveur écoute si un client cherche à se connecter*/
    if ((listen(socketID, 5)) == -1)
    {
        perror("listen");
        exit (-1);
    }
}

/*
*
*
*/
int envoyerMessage(int socketID, char commande, char * msg){
    printf("Evoi du message : %s", msg);
    if ((send(socketID, msg, strlen(msg), 0)) == -1){
           perror("send");
           return 0;
    }
    return 1;
}

/*
*
*
*/
void sendData(char * data){

}

char * concat(char * dest, char * source){
	char *message = (char *)malloc(256);
    strcpy(message, dest);
    strcat(message, source);
    return message;
}