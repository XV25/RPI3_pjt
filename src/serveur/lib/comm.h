#include <string.h>
// Reponses du serveur
const int CAPTURE_SUCCESS = 0;
const int CAPTURE_ERROR = 1;

// Commandes du client
const int CMD_PRENDRE_PHOTO = 2;
/*
*
*
*/
void sendMessage(char * msg);

/*
*
*
*/
void sendData(char * data);