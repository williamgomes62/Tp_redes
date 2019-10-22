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

#include "chap04.h"
#include <ctype.h>

/*
inicia o main() e inicializa o Winsock.
*/
int main() {

#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif

    /*
    Em seguida, encontramos nosso endereço local em que devemos ouvir,
    criar o soquete e vincular a ele.
    A única diferença entre esse código e os servidores TCP é que
    usamos SOCK_DGRAM em vez de SOCK_STREAM. SOCK_DGRAM especifica que queremos um soquete UDP.
    Aqui está o código para definir o endereço e criar um novo soquete:
    */

    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(0, "8080", &hints, &bind_address);


    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family,
            bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    /*
    A ligação do novo soquete ao endereço local é feita da seguinte maneira:
    */

    printf("Binding socket to local address...\n");
    if (bind(socket_listen,
                bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(bind_address);

    /*
    Como nosso servidor usa select(), precisamos criar um novo fd_set para armazenar nossa escuta.
    Nós zeramos o conjunto usando FD_ZERO () e adicionamos nosso soquete a este conjunto usando
    FD_SET (). Também mantemos o soquete máximo no conjunto usando max_socket:
    */

    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_listen, &master);
    SOCKET max_socket = socket_listen;

    printf("Waiting for connections...\n");

    /*
    Loop principal : Copia o conjunto de soquetes para uma nova variável, lê,
    e então usa select() para esperar até que nosso soquete esteja pronto para ler.
    Lembre-se de que poderíamos transmita um valor de tempo limite como o último
    parâmetro a select() se quisermos definir um valor máximo de tempo de espera para a próxima leitura. 

    Depois que select() retorna, usamos FD_ISSET() para saber se nosso soquete específico,
    socket_listen, está pronto para ser lido. Se tivéssemos soquetes adicionais, precisaríamos
    usar FD_ISSET() para cada soquete.


    Se FD_ISSET() retornar true, lemos do soquete usando recvfrom().
    recvfrom() nos fornece o endereço do remetente; portanto, devemos primeiro
    alocar uma variável para reter o endereço, para que é, client_address.
    Depois de ler uma string do soquete usando recvfrom(), nós convertemos
    a string em maiúscula usando a função C toupper(). Em seguida, enviamos o
    texto modificado de volta ao remetente usando sendto(). Observe que os dois
    últimos parâmetros para sendto() são os endereços do cliente que obtemos de recvfrom().
    */

    while(1) {
        fd_set reads;
        reads = master;
        if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        if (FD_ISSET(socket_listen, &reads)) {
            struct sockaddr_storage client_address;
            socklen_t client_len = sizeof(client_address);

            char read[1024];
            int bytes_received = recvfrom(socket_listen, read, 1024, 0,
                    (struct sockaddr *)&client_address, &client_len);
            if (bytes_received < 1) {
                fprintf(stderr, "connection closed. (%d)\n",
                        GETSOCKETERRNO());
                return 1;
            }

            int j;
            for (j = 0; j < bytes_received; ++j)
                read[j] = toupper(read[j]);
            sendto(socket_listen, read, bytes_received, 0,
                    (struct sockaddr*)&client_address, client_len);

        } //if FD_ISSET
    } //while(1)

    /*
    Podemos então fechar o soquete, limpar o Winsock e finalizar o programa.
    Note que este o código nunca é executado, porque o loop principal
    nunca termina. Nós incluímos esse código de qualquer maneira como
    boa prática; caso o programa seja adaptado no futuro para ter uma função de saída.
    */

    printf("Closing listening socket...\n");
    CLOSESOCKET(socket_listen);

#if defined(_WIN32)
    WSACleanup();
#endif

    printf("Finished.\n");

    return 0;
}
