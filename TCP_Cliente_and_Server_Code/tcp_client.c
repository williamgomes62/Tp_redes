/*
 * MIT License
 *
 * Copyright (c) 2018 Lewis Van Winkle
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "chap03.h"

#if defined(_WIN32)
#include <conio.h>
#endif

#include <stdlib.h>
#include <time.h>

#define TAM_MESSAGE 512
#define NUM_MESSAGE 512

#define LOCAL_MACHINE // TEMPO DE EXECUÇÃO EM CASO DE CLIENTE/SERVIDOR RODAREM NA MESMA MAQUINA 

int main(int argc, char *argv[]) {

#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif

    if (argc < 3) {
        fprintf(stderr, "usage: tcp_client hostname port\n");
        return 1;
    }

    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *peer_address;
    if (getaddrinfo(argv[1], argv[2], &hints, &peer_address)) {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }


    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
            address_buffer, sizeof(address_buffer),
            service_buffer, sizeof(service_buffer),
            NI_NUMERICHOST);
    printf("%s %s\n", address_buffer, service_buffer);


    printf("Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family,
            peer_address->ai_socktype, peer_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_peer)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }


    printf("Connecting...\n");
    if (connect(socket_peer,
                peer_address->ai_addr, peer_address->ai_addrlen)) {
        fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(peer_address);

    printf("Connected.\n\n");
    printf("To send data, enter text followed by enter.\n");


    /*
    Para o envio da rajada de 512 mensagens conseecutivas
    */

    int i = 0;//iterador do loop
    char* send_messages = (char*)malloc(sizeof(char)*TAM_MESSAGE);//vetor para o envio de mensagens 
    char* received_messages = (char*)malloc(sizeof(char)*TAM_MESSAGE);//vetor para recebimento das mensagens
    long int total_receveid = 0;

    
    //Medição do tempo de round/trip
    clock_t t; //variável para armazenar tempo
    t = clock();

    while(i<NUM_MESSAGE) {

        //-------------------------
        
        for (int i = 0; i < TAM_MESSAGE; ++i)
        {
            send_messages[i] = 'a';
        }
        //if (!fgets(read, 4096, stdin)) break;
        //printf("\nSending: %s\n", read);
        int bytes_sent = send(socket_peer, send_messages, strlen(send_messages), 0);
        printf("Sent %d bytes.\n", bytes_sent);

        //-------------------------

        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(socket_peer, &reads);
#if !defined(_WIN32)
        FD_SET(0, &reads);
#endif

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;

        if (select(socket_peer+1, &reads, 0, 0, &timeout) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }  

        if (FD_ISSET(socket_peer, &reads)) {
            int bytes_received = recv(socket_peer, received_messages, TAM_MESSAGE, 0);
            if (bytes_received < 1) {
                printf("Connection closed by peer.\n");
                break;
            }
            /*printf("\nReceived (%d bytes): %.*s\n",
                    bytes_received, bytes_received, received_messages);*/
            printf("Received (%d bytes)\n",bytes_received );
            total_receveid += bytes_received;
        }


#if defined(_WIN32)
        if(_kbhit()) {
#else
        if(FD_ISSET(0, &reads)) {
#endif
    } 
    i++;  
    } //end while(1)

#if defined(LOCAL_MACHINE)
    //calculo do tempo para o caso de cliente/servidor ocuparem a mesma maquina
    t = clock() - t;
    double time = ((double)t)/((CLOCKS_PER_SEC/1000));//tempo em milisegundos
    printf("\nTime in round/trip : %lf ms \n",time );
#endif

    //calculo de perda de dados
    double loss = 0.0;
    if(total_receveid == 0)
        loss = 100.0;
    else
    loss = ((NUM_MESSAGE - (total_receveid/TAM_MESSAGE))/NUM_MESSAGE)/100.0;

    printf("\nNum of mesages sent : %d\n",i );
    printf("Size in bytes : %d\n",TAM_MESSAGE );
    printf("Loss : %.3lf %% \n",loss );


    //Desaloca memoria dinamica
    free(send_messages);//libera o vetor send_messages
    free(received_messages);//libera o vetor received_messages

    printf("\nClosing socket...\n");
    CLOSESOCKET(socket_peer);

#if defined(_WIN32)
    WSACleanup();
#endif

    printf("Finished.\n");
    return 0;
}

