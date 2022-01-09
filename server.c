//
// Created by tobia on 6. 1. 2022.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048

static _Atomic unsigned int cli_count = 0;
static int uid = 10;


/* Client structure */


typedef struct{
    struct sockaddr_in address;
    int sockfd;
    int uid;
    char name[32];
    char heslo[32];
    char friendlist[50][32];
    int pocetPriatelov;
} client_t;


client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


int numberUsers = 0;

client_t* users[10];

void str_overwrite_stdout() {
    printf("\r%s", "> ");
    fflush(stdout);
}



void str_trim_lf (char* arr, int length) {
    int i;
    for (i = 0; i < length; i++) { // trim \n
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}

void print_client_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d",
           addr.sin_addr.s_addr & 0xff,
           (addr.sin_addr.s_addr & 0xff00) >> 8,
           (addr.sin_addr.s_addr & 0xff0000) >> 16,
           (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

/* Add clients to queue */
void queue_add(client_t *cl){
    pthread_mutex_lock(&clients_mutex);

    for(int i=0; i < MAX_CLIENTS; ++i){
        if(!clients[i]){
            clients[i] = cl;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}


/* Remove clients to queue */
void queue_remove(int uid){
    pthread_mutex_lock(&clients_mutex);

    for(int i=0; i < MAX_CLIENTS; ++i){
        if(clients[i]){
            if(clients[i]->uid == uid){
                clients[i] = NULL;
                break;
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void updateAccountsLoad() {
    FILE *filePointer;
    char line[50];
    if (access("users.txt", F_OK) == 0) {
        filePointer = fopen("users.txt", "r");
        while (fgets(line, 50, filePointer) != NULL) {
            char name[10]="";
            char psswd[10]="";
            sscanf(line, "%s %s", name, psswd);
            if(name){str_trim_lf(name, sizeof(name));}
            if(psswd){str_trim_lf(psswd, sizeof(psswd));}
            if (numberUsers < 10) {
                client_t *new = (client_t *) malloc(sizeof(client_t));
                strcpy(new->name, name);
                strcpy(new->heslo, psswd);
                users[numberUsers] = new;
                numberUsers++;
            }
        }
        fclose(filePointer);
    } else {
        printf("Subor pouzivatelov sa nenasiel!\n");
    }

}

void updateAccountsSave() {
    FILE *filePointer;
    filePointer = fopen("users.txt", "w");
    for (int i = 0; i < numberUsers; ++i) {
        fputs(users[i]->name, filePointer);
        fputs(" ", filePointer);
        fputs(users[i]->heslo, filePointer);
        fputs("\n", filePointer);
    }
    fclose(filePointer);
}

void delete_account(char *name) {
    for (int i = 0; i < numberUsers; ++i) {
        if (strcmp(users[i]->name, name) == 0) {
            printf("Pouzivatel %s deletovany\n", users[i]->name);
            users[i] = NULL;
            for (int j = i; j < numberUsers - 1; ++j) {
                users[j] = users[j + 1];
            }


            numberUsers--;
            updateAccountsSave();
        }
    }
}


void send_message(char *s, int uid){
    pthread_mutex_lock(&clients_mutex);

    for(int i=0; i<MAX_CLIENTS; ++i){
        if(clients[i]){
            if(clients[i]->uid != uid){
                if(write(clients[i]->sockfd, s, strlen(s)) < 0){
                    perror("ERROR: write to descriptor failed");
                    break;
                }
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}
void send_message_to(char *s, int uid){
    pthread_mutex_lock(&clients_mutex);

    for(int i=0; i<MAX_CLIENTS; ++i){
        if(clients[i]){
            if(clients[i]->uid == uid){
                if(write(clients[i]->sockfd, s, strlen(s)) < 0){
                    perror("ERROR: write to descriptor failed");
                    break;
                }
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg){
    char buff_out[BUFFER_SZ];
    char PorR[2]="";
    char name[32];
    char heslo[32];
    int leave_flag = 0;
    client_t *cli = (client_t *)arg;
    for(int i =0; i < 50;i++) {
        for(int j =0; j < 32;j++) {
            cli->friendlist[i][j] = 0;
        }
    }

    recv(cli->sockfd,PorR,1,0);
    recv(cli->sockfd,heslo,32,0);
    recv(cli->sockfd, name, 32, 0);
    cli->pocetPriatelov=0;

    strcpy(clients[cli_count]->name,name);
    strcpy(clients[cli_count]->heslo,heslo);
    cli_count++;
    if(strcmp(PorR,"R") == 0) {
        for (int i = 0; i < numberUsers; ++i) {
            if (strcmp(users[i]->name, name) == 0) {
                sprintf(buff_out,"Zadane meno uz existuje.\n");
                send_message_to(buff_out, cli->uid);
                leave_flag = 1;
                break;
            }
        }
        if(leave_flag != 1){
            strcpy(cli->name, name);
            strcpy(cli->heslo, heslo);
            users[numberUsers] = cli;
            numberUsers++;
            sprintf(buff_out, "%s sa pripojil\n", cli->name);
            printf("%s", buff_out);
            bzero(buff_out, BUFFER_SZ);
            sprintf(buff_out, "Zaregistrovali ste sa ako %s.\n",cli->name);
            send_message_to(buff_out, cli->uid);
            bzero(buff_out, BUFFER_SZ);
            updateAccountsSave();
        }

    }

    if(strcmp(PorR,"P") == 0) {
        int nasiel = 0;
        for (int i = 0; i < numberUsers; ++i) {
            if (strcmp(users[i]->name, name) == 0) {
                if(strcmp(users[i]->heslo, heslo) == 0) {
                    sprintf(buff_out, "Ste prihlaseny ako %s.\n",name);
                    send_message_to(buff_out, cli->uid);
                    bzero(buff_out, BUFFER_SZ);
                    strcpy(cli->name, name);
                    strcpy(cli->heslo, heslo);
                    sprintf(buff_out, "%s sa pripojil\n", cli->name);
                    send_message(buff_out,cli->uid);
                    printf("%s", buff_out);
                    nasiel=1;
                    break;
                } else{
                    sprintf(buff_out,"Zle zadane heslo .\n");
                    send_message_to(buff_out, cli->uid);
                    leave_flag = 1;

                    break;
                }
            }
        }
        if(nasiel!=1) {
            sprintf(buff_out, "zadane meno neexistuje .\n");
            send_message_to(buff_out, cli->uid);
            leave_flag = 1;

        }

    }


    bzero(buff_out, BUFFER_SZ);
    while(leave_flag ==0){
        bzero(buff_out, BUFFER_SZ);
        int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
        if (receive > 0){
            if(strlen(buff_out) > 0) {
                if (strcmp(buff_out, "delete") == 0) {
                    sprintf(buff_out, "%s deletoval svoj ucet\n", cli->name);
                    printf("%s", buff_out);
                    send_message(buff_out, cli->uid);
                    delete_account(cli->name);
                    leave_flag = 1;
                    updateAccountsSave();
                } else if((strcmp(buff_out, "remove") == 0)) {
                    sprintf(buff_out, "Vyberte koho chcete odstranit \n");
                    send_message_to(buff_out, cli->uid);
                    for (int i = 0; i < cli->pocetPriatelov;i++) {
                        sprintf(buff_out, "%s\n", cli->friendlist[i]);
                        send_message_to(buff_out, cli->uid);
                    }
                    bzero(buff_out, BUFFER_SZ);  // 0 1 2 3 4  i !=4
                    recv(cli->sockfd,buff_out,BUFFER_SZ,0);
                    for (int i = 0; i < cli->pocetPriatelov;i++) {
                        if(!strcmp(cli->friendlist[i],buff_out)) {
                            sprintf(buff_out, "%s si vás odstránil z priatelov\n", cli->name);
                            send_message_to(buff_out, clients[i]->uid);
                            sprintf(buff_out, "%s ste si odstranili z priatelov\n", cli->friendlist[i]);
                            send_message_to(buff_out, cli->uid);
                            for(int j = 0; j < 32;j++) {
                                cli->friendlist[i][j] = 0;
                            }
                            for(int l = i; l < cli->pocetPriatelov -1 ;l++) {
                                for (int j = 0; j < 32; ++j) {
                                    cli->friendlist[l][j] = cli->friendlist[l + 1][j];
                                }
                            }
                            cli->pocetPriatelov--;
                            clients[i]->pocetPriatelov--;
                        }
                    }

                } else if (strcmp(buff_out, "exit") == 0) {
                    sprintf(buff_out, "%s odisiel z chatu\n", cli->name);
                    printf("%s", buff_out);
                    send_message(buff_out, cli->uid);
                    leave_flag = 1;
                    updateAccountsSave();
                } else if (strcmp(buff_out, "message") == 0) {
                    sprintf(buff_out, "Vyberte ktoremu priatelovi napisete\n");
                    send_message_to(buff_out, cli->uid);
                    for (int i = 0; i < cli->pocetPriatelov;i++) {
                            sprintf(buff_out, "%s\n", cli->friendlist[i]);
                            send_message_to(buff_out, cli->uid);
                    }
                    bzero(buff_out, BUFFER_SZ);
                    recv(cli->sockfd,buff_out,BUFFER_SZ,0);
                    for (int h = 0; h < cli_count;h++) {
                        if(!strcmp(clients[h]->name,buff_out)) {
                            sprintf(buff_out, "Napiste spravu\n");
                            send_message_to(buff_out, cli->uid);
                            bzero(buff_out, BUFFER_SZ);
                            recv(cli->sockfd,buff_out,BUFFER_SZ,0);
                            send_message_to(buff_out,clients[h]->uid);
                        }
                    }
                } else if (strcmp(buff_out, "add") == 0) {
                    sprintf(buff_out, "Vyberte koho si chcete pridat za priatela\n");
                    send_message_to(buff_out, cli->uid);
                    for (int i = 0; i < numberUsers;i++) {
                        if(strcmp(users[i]->name,cli->name)) {
                            sprintf(buff_out, "%s\n", users[i]->name);
                            send_message_to(buff_out, cli->uid);
                        }
                    }
                    bzero(buff_out, BUFFER_SZ);
                    recv(cli->sockfd,buff_out,BUFFER_SZ,0);
                    for (int i = 0; i < cli_count;i++) {
                        if(!strcmp(clients[i]->name,buff_out)) {
                            cli->pocetPriatelov++;
                            clients[i]->pocetPriatelov++;
                            sprintf(buff_out, "%s si vás pridal za priatela\n", cli->name);
                            send_message_to(buff_out, clients[i]->uid);
                            sprintf(buff_out, "%s ste si pridali do priatelov\n", clients[i]->name);
                            send_message_to(buff_out, cli->uid);


                            strcpy(cli->friendlist[cli->pocetPriatelov-1] , clients[i]->name);
                            strcpy(clients[i]->friendlist[clients[i]->pocetPriatelov-1] , cli->name);
                        }
                    }
                }else {
                    send_message(buff_out, cli->uid);
                    str_trim_lf(buff_out, strlen(buff_out));
                    printf("%s\n", buff_out);
                }
            }
        }else {
            printf("ERROR: -1\n");
            leave_flag = 1;
        }

        bzero(buff_out, BUFFER_SZ);
    }

    /* Delete client from queue and yield thread */
    close(cli->sockfd);
    queue_remove(cli->uid);
    cli_count--;

    pthread_join(pthread_self(),NULL);
    free(cli);

    return NULL;
}

typedef struct vlakna{
    pthread_t* vlakna;
    int pocetVlakien;
}VLAKNA;

typedef struct vlaknoparam{
    int koniec;
    int socket;
}VLAKNOPAR;

void pridajVlakno(VLAKNA* vlakna, pthread_t pridajToto){
    pthread_t *novePole = (pthread_t*)malloc((vlakna->pocetVlakien+1) * sizeof(pthread_t));
    for (int i = 0; i < vlakna->pocetVlakien; ++i) {
        novePole[i] = vlakna->vlakna[i];
    }
    novePole[vlakna->pocetVlakien]=pridajToto;
    if(vlakna->pocetVlakien!=0){
        free(vlakna->vlakna);
    }
    vlakna->pocetVlakien+=1;
    vlakna->vlakna = novePole;
}
void vymazVlakna(VLAKNA* vlakna){
    free(vlakna->vlakna);
}
void* ukonci(void* data){
    char prem[5];
    scanf("%s", prem);
    VLAKNOPAR *vlaknopar = (VLAKNOPAR*) data;
    vlaknopar->koniec=1;
    shutdown(vlaknopar->socket,SHUT_RD);
    close(vlaknopar->socket);
}


int main(int argc, char **argv){
    updateAccountsLoad();
    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = "127.0.0.1";
    int port = atoi(argv[1]);
    int option = 1;
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;




    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);


    signal(SIGPIPE, SIG_IGN);

    if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
        perror("ERROR: setsockopt failed");
        return EXIT_FAILURE;
    }


    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR: Socket binding failed");
        return EXIT_FAILURE;
    }


    if (listen(listenfd, 10) < 0) {
        perror("ERROR: Socket listening failed");
        return EXIT_FAILURE;
    }

    printf("=== VITAJTE V CHATOVACIEJ MIESTNOSTI ===\n");
    VLAKNA vlakna = {
            NULL,
            0
    };
    pthread_t ukoncovac;
    VLAKNOPAR vlaknopar = {
            0,
            listenfd
    };
    pthread_create(&ukoncovac,NULL,&ukonci,&vlaknopar);


    pridajVlakno(&vlakna,ukoncovac);

    while(1){

        unsigned int clilen = sizeof(cli_addr);
        connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);
        if(vlaknopar.koniec){
            break;
        }
        pthread_t tid;

        if((cli_count + 1) == MAX_CLIENTS){
            printf("Max clients reached. Rejected: ");
            print_client_addr(cli_addr);
            printf(":%d\n", cli_addr.sin_port);
            close(connfd);
            continue;
        }


        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->address = cli_addr;
        cli->sockfd = connfd;
        cli->uid = uid++;


        queue_add(cli);
        pthread_create(&tid, NULL, &handle_client, (void*)cli);
        pridajVlakno(&vlakna,tid);

        sleep(1);

    }
    for (int i = 0; i < vlakna.pocetVlakien; ++i) {
        pthread_join(vlakna.vlakna[i],NULL);

    }
    vymazVlakna(&vlakna);
    return EXIT_SUCCESS;

}






















