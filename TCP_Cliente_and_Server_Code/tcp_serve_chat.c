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

/*Inclusão de cabeçalhos*/
#include "chap03.h"
#include <ctype.h>

int main() {

/*
Inicia o main() e inicializa o Winsock(Socket API's).
*/
#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif

    /*
    Obtem o endereço local. Cria o socket e bind()-liga-conecta
    */

    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
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
    Vamos ouvir na porta '8080' e isso pode ser mudado.
    Estamos fazendo um IPv4 aqui. Para ouvir conecções IPv6 
    basta alterar AF_INET para AF_INET6.
    Em seguida ligamos [bind()] nosso socket ao endereço local
    e o fazemos entrar em um estado de escuta.
    */

    printf("Binding socket to local address...\n");
    if (bind(socket_listen,
                bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(bind_address);


    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0) {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    /*
    Definimos uma estrutura fd_set que armazena todos os sockets ativos.
    Também mantemos uma variável max_socket que contém o maior descritor 
    de socket. Por enquanto, adicionamos apenas nossa escuta tomada para 
    o aparelho. Por ser o único socket, ele também deve ser o maoir, então
    definimos max_socket = socket_listen.
    */

    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_listen, &master);
    SOCKET max_socket = socket_listen;

    /*
    Posteriormente adivionaremos novas conecções ao master à medida que 
    forem estabelecidas.
    Em seguida, imprimimos uma mensagem de status, inserimos o loop principal
    e configuramos nossa chamada para select(). 
    */

    printf("Waiting for connections...\n");


    while(1) {
        fd_set reads;
        reads = master;
        if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

    /*
    Isso funciona copiando nosso master fd_set para leituras. Lembre-se que
    select() modifica o conjunto dado a ele. Se não copiassemos o mestre, perderíamos seus dados.
    Passamos um valor de tempo limite igual a 0 (NULL) para select() para
    que ele não retorne até que um soccket master esteja pronto pra ser lido.
    No início do progrma somente master contém
    socket_listen, mas durante a execução adicionamos cada nova conecção ao master.
    Agora, percorremos cada soquete possível e verificamos se foi sinalizado por select() como
    estar pronto. Se um socket, x, foi sinalizado por select(), então FD_ISSET(x, &reads) é verdadeiro.
    Os descritores de soquete são números inteiros positivos, portanto, podemos tentar todos os descritores
    de soquete possíveis até   max_socket. A estrutura básica do nosso loop é a seguinte:
    */

        SOCKET i;
        for(i = 1; i <= max_socket; ++i) {
            if (FD_ISSET(i, &reads)) {
                //Handle socket

                /*

                Lembre-se, FD_ISSET () é verdadeiro apenas para soquetes prontos para serem lidos.
                No caso de socket_listen, isso significa que uma nova conecção está prota para 
                ser estabelecida com accept(). Para todos os outros soquetes, isso significa que 
                os dados estão prontos para serem lidos com recv(). Nós devemos primeiro determinar 
                se o soquete atual é o de escuta ou não. Se for, chamamos accept().
                Esse trecho de código e o que segue substituem o soquete // Handle comentado acima.
                */
                if (i == socket_listen) {
                    struct sockaddr_storage client_address;
                    socklen_t client_len = sizeof(client_address);
                    SOCKET socket_client = accept(socket_listen,
                            (struct sockaddr*) &client_address,
                            &client_len);
                    if (!ISVALIDSOCKET(socket_client)) {
                        fprintf(stderr, "accept() failed. (%d)\n",
                                GETSOCKETERRNO());
                        return 1;
                    }

                    FD_SET(socket_client, &master);
                    if (socket_client > max_socket)
                        max_socket = socket_client;

                    char address_buffer[100];
                    getnameinfo((struct sockaddr*)&client_address,
                            client_len,
                            address_buffer, sizeof(address_buffer), 0, 0,
                            NI_NUMERICHOST);
                    printf("New connection from %s\n", address_buffer);

                }

                /*
                Se o socket for socket_listen, accept()-aceitamos- a conecção.
                Usamos FD_SET () para adicionar as novas conexões soquete ao conjunto
                de soquetes mestre(master). Isso nos permite monitorá-lo com chamadas 
                subseqüentes para select(). Também mantemos max_socket.
                Como etapa final, esse código imprime as informações do endereço do cliente
                usando getnameinfo().

                Se o socket i não for socket_listen, é uma solicitação de um estabelecimento
                conexão. Nesse caso, precisamos lê-lo com recv(), convertê-lo em maiúsculas usando o
                função toupper() incorporada e envie os dados de volta:
                */

                 else {
                    char read[1024];
                    int bytes_received = recv(i, read, 1024, 0);
                    if (bytes_received < 1) {
                        FD_CLR(i, &master);
                        CLOSESOCKET(i);
                        continue;
                    }

                    SOCKET j;
                    for (j = 1; j <= max_socket; ++j) {
                        if (FD_ISSET(j, &master)) {
                            if (j == socket_listen || j == i)
                                continue;
                            else
                                send(j, read, bytes_received, 0);
                        }
                    }
                }
            } //if FD_ISSET
        } //for i to max_socket
    } //while(1)

    /*
    Se o cliente foi desconectado, recv() retorna um número não positivo.
    Nesse caso, nós removemos esse soquete do conjunto de soquetes mestre
    e também chamamos CLOSESOCKET() nele para Limpar.
    Podemos finalizar a instrução if FD_ISSET(), finalizar o
    para loop, encerre o loop while, feche o soquete de escuta e limpe o Winsock:
    */


    printf("Closing listening socket...\n");
    CLOSESOCKET(socket_listen);

#if defined(_WIN32)
    WSACleanup();
#endif


    printf("Finished.\n");

    /*
    Nosso programa está configurado para ouvir continuamente as conexões, portanto,
    o código após o final do loop while nunca será executado. No entanto, 
    ainda é uma boa prática incluí-lo no caso programamos a funcionalidade
    posteriormente para abortar o loop while.

   
    */

    return 0;
}
