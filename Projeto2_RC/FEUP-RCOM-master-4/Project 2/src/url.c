#include "url.h"


void initializeUrlInfo(urlInfo* url){
    memset(url->user,0,256);
    memset(url->password,0,256);
    memset(url->host,0,256);
    memset(url->urlPath,0,256);
    memset(url->ipAddress,0,256);
    memset(url->fileName,0,256);
    url->port = 21;
}

char* getStringBeforeCharacther(char* str, char chr){

    char* aux = (char*)malloc(strlen(str));
    strcpy(aux, strchr(str, chr));
    int index = strlen(str) - strlen(aux);

    aux[index] = '\0';

    strncpy(aux,str,index);

    return aux;

}

int getIpAddressFromHost(urlInfo* url){
    struct hostent* h;

    if ((h=gethostbyname(url->host)) == NULL) {  
        herror("gethostbyname");
        return -1;
    }

    strcpy(url->ipAddress,inet_ntoa(*((struct in_addr *)h->h_addr)));

    return 0;
}


int parseUrlInfo(urlInfo* url, char * urlGiven){
    char* protocol = "ftp://";
    char firstPart[7];
    memcpy(firstPart,urlGiven,6);

    if(strcmp(protocol,firstPart) != 0){
        return -1;
    }

    char * auxUrl = (char*) malloc(strlen(urlGiven));
    char * urlPath = (char*) malloc(strlen(urlGiven));

    strcpy(auxUrl,urlGiven+6);

    char* userNameGiven  = strchr(auxUrl,'@');

    //username given
    if(userNameGiven != NULL){
        strcpy(urlPath,userNameGiven+1);
        
        if(strchr(auxUrl,':') == NULL){
            char* aux;
            strcpy(url->user,getStringBeforeCharacther(auxUrl,'@'));

            printf("Please put the user password: \n");
            fgets(url->password,256,stdin);
            
            aux = strchr(url->password,'\n');
            *aux = '\0';
            
        }
        else{
            strcpy(url->user,getStringBeforeCharacther(auxUrl,':'));

            strcpy(auxUrl,auxUrl + strlen(url->user)+1);

            strcpy(url->password,getStringBeforeCharacther(auxUrl,'@'));
        }
    }
    else{
        strcpy(urlPath,auxUrl);

        strcpy(url->user,"anonymous");
        strcpy(url->password,"anypassword");
    }
    
    
    
    strcpy(url->host,getStringBeforeCharacther(urlPath,'/'));

    strcpy(urlPath,urlPath + strlen(url->host)+1);
    int index = -1;


    for (int i = strlen(urlPath)-1; i >= 0; i--)
    {

        if(urlPath[i] == '/'){
            index = i;
            break;
        }    
    }

    strcpy(url->urlPath,urlPath);
    if(index == -1){
        strcpy(url->fileName,urlPath);
    }
    else
    {
        strcpy(url->fileName,urlPath+index+1);
    }

    return 0;

}
