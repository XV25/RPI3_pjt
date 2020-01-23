
// Reponses du serveur
const struct CMDS_SERVEUR {
    const char CAPTURE_SUCCESS;
    const char CAPTURE_ERROR;
} CMDS_SERVEUR = {0,1};

// Commandes du client
const struct CMDS_CLIENT {
    const char CMD_PRENDRE_PHOTO;
    const char CMD_DEFINIR_RESOLUTION;
    const char INFO;
} CMDS_CLIENT = {2,3,4};