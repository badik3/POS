//
// Created by tobia on 6. 1. 2022.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define LENGTH 2048

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[32];
char heslo[32];
char PorR[2];
char del[1]={"D"};

void str_overwrite_stdout() {
    printf("%s", "> ");
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

void catch_ctrl_c_and_exit(int sig) {
    flag = 1;
}

void send_msg_handler() {
    char message[LENGTH] = {};
    char buffer[LENGTH + 32] = {};

    while(1) {
        str_overwrite_stdout();
        fgets(message, LENGTH, stdin);
        str_trim_lf(message, LENGTH);

        if (strcmp(message, "delete")== 0 ){
            sprintf(buffer, "delete");
            send(sockfd, buffer, strlen(buffer), 0);
            break;
        } else if (strcmp(message, "add") == 0) {
            sprintf(buffer, "add");
            send(sockfd, buffer, strlen(buffer), 0);
            bzero(message, LENGTH);
            bzero(buffer, LENGTH + 32);

            fgets(message, LENGTH, stdin);
            str_trim_lf(message, LENGTH);
            sprintf(buffer, "%s", message);
            send(sockfd, buffer,strlen(buffer),0);

        } else if (strcmp(message, "remove") == 0) {
            sprintf(buffer, "remove");
            send(sockfd, buffer, strlen(buffer), 0);
            bzero(message, LENGTH);
            bzero(buffer, LENGTH + 32);
            fgets(message, LENGTH, stdin);
            str_trim_lf(message, LENGTH);
            sprintf(buffer, "%s", message);
            send(sockfd, buffer,strlen(buffer),0);

        }else if (strcmp(message, "message") == 0) {
            sprintf(buffer, "message");
            send(sockfd, buffer, strlen(buffer), 0);
            bzero(message, LENGTH);
            bzero(buffer, LENGTH + 32);

            fgets(message, LENGTH, stdin);
            str_trim_lf(message, LENGTH);
            sprintf(buffer, "%s", message);
            send(sockfd, buffer,strlen(buffer),0);

            bzero(message, LENGTH);
            bzero(buffer, LENGTH + 32);

            fgets(message, LENGTH, stdin);
            str_trim_lf(message, LENGTH);
            sprintf(buffer, "%s: %s\n",name, message);
            send(sockfd, buffer,strlen(buffer),0);

        }else if (strcmp(message, "exit") == 0) {
            sprintf(buffer, "exit");
            send(sockfd, buffer, strlen(buffer), 0);
            break;
        } else {
            sprintf(buffer, "%s: %s\n", name, message);
            if(send(sockfd, buffer, strlen(buffer), MSG_NOSIGNAL) == -1){
                catch_ctrl_c_and_exit(1);

            }

        }

        bzero(message, LENGTH);
        bzero(buffer, LENGTH + 32);
    }
    catch_ctrl_c_and_exit(2);
}
void prihlasenie(){
    printf("Register or login?\n 1. Register\n 2. Login\n ");
    int c = getchar();
    switch (c){
        case 49:
            strcpy(PorR,"R");
            printf("Zadaj meno:");
            scanf("%s", name);
            printf("Zadaj heslo:");
            scanf("%s", heslo);

            str_trim_lf(name, strlen(name));
            str_trim_lf(heslo, strlen(heslo));

            break;
        case 50:
            strcpy(PorR,"P");
            printf("Zadaj meno:");
            scanf("%s", name);
            printf("Zadaj heslo:");
            scanf("%s", heslo);
            str_trim_lf(name, strlen(name));
            str_trim_lf(heslo, strlen(heslo));

            break;
    }
}
void volbaOperacie(){
    printf("Zvol Moznost?\n 1. w\n 2. Login\n ");
    int c = getchar();
    switch (c){
        case 49:
            strcpy(PorR,"R");
            printf("Zadaj meno:");
            scanf("%s", name);
            printf("Zadaj heslo:");
            scanf("%s", heslo);

            str_trim_lf(name, strlen(name));
            str_trim_lf(heslo, strlen(heslo));

            break;
        case 50:
            strcpy(PorR,"P");
            printf("Zadaj meno:");
            scanf("%s", name);
            printf("Zadaj heslo:");
            scanf("%s", heslo);
            str_trim_lf(name, strlen(name));
            str_trim_lf(heslo, strlen(heslo));

            break;
    }
}

void recv_msg_handler() {
    char message[LENGTH] = {};
    while (1) {
        int receive = recv(sockfd, message, LENGTH, 0);
        if (receive > 0) {
            printf("%s", message);
            str_overwrite_stdout();
        } else if (receive == 0) {
            break;
        } else {
            // -1
        }
        memset(message, 0, sizeof(message));
    }
}

int main(int argc, char **argv){
    FILE* subor;
    char str[50];
    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = "127.0.0.1";
    int port = atoi(argv[1]);

    signal(SIGINT, catch_ctrl_c_and_exit);

    prihlasenie();





    if (strlen(name) > 32 || strlen(name) < 2){
        printf("Memo musi byt dlhsie nez 2 snaky a kratsie ako 30 znakov.\n");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;


    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);



    int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err == -1) {
        printf("ERROR: connect\n");
        return EXIT_FAILURE;
    }

    send(sockfd,PorR,1,0);

    send(sockfd,heslo,32,0);

    send(sockfd, name, 32, 0);


    printf("=== VITAJTE V CHATOVACIEJ MIESTNOSTI===\n");

    pthread_t send_msg_thread;
    if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0){
        printf("ERROR: pthread\n");
        return EXIT_FAILURE;
    }

    pthread_t recv_msg_thread;
    if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0){
        printf("ERROR: pthread\n");
        return EXIT_FAILURE;
    }

    while (1){
        if(flag){
            printf("\nBye\n");
            break;
        }
    }
    pthread_join(send_msg_thread,NULL);
    pthread_join(recv_msg_thread,NULL);
    close(sockfd);

    return EXIT_SUCCESS;
}
