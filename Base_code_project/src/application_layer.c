// Application layer protocol implementation

#include "application_layer.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    // TODO

    int resTx= strcmp( role, "tx");
    int resRx= strcmp (role, "rx");

    LinkLayerRole Tr;
    if(resTx==0){
        Tr=LlTx;
    }
    else if(resRx==0){
        Tr=LlRx;
    }
    else{ 
        printf("\nError! Role is Invalid!");
        return;
    }
}
