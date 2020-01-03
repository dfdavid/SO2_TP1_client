#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h> // la estructura sockaadr_in pertence a esta libreria
#include <arpa/inet.h>
#include <unistd.h> //en esta libreria esta la funcion sleep(), la funcion read()
#include <sys/sysinfo.h>
#include <stdbool.h>

#define BUFFER_SIZE 1000




char firmware_version[20] = "1.0";
uint16_t server_port= 5520;
char ip_server_buff[32]="127.0.0.1";
char *ip_server = NULL;
int retry_time=3;

long get_uptime();


int main() {

    printf("ejecutando main \n");

    int sockfd, ip_srv_load;
    struct sockaddr_in dest_addr;
    char buffer[BUFFER_SIZE], auxBuffer[BUFFER_SIZE];
    char buffer_recepcion[BUFFER_SIZE];

    //crer socket
    sockfd= socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd <0){
        perror("error al abrir el socket cliente");
    }


    //setear la estructura sockaddr
    memset(&dest_addr, 0, sizeof(dest_addr) ); //limpieza de la estructura
    dest_addr.sin_family= AF_INET;
    dest_addr.sin_port=htons(server_port);
    ip_server = (char *)&ip_server_buff;
    //https://www.systutorials.com/docs/linux/man/3-inet_aton/
    //The inet_aton() function returns 1 if the address is successfully converted, or 0 if the conversion failed.

    if (  inet_aton(ip_server, &dest_addr.sin_addr ) == 0 )  {
        fprintf(stderr, "Invalid address\n");

    }

    //implementar bind()    hace falta esto en el cliente?

    //Conexion
    //Upon successful completion, connect() returns 0. Otherwise, -1 is returned and errno is set to indicate the error.
    while( connect( sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr) ) < 0 ){
        perror("Intentando conectar al servidor");
        //printf("Reintentando coenctar en %d segundos \n", retry_time);
        sleep(retry_time);
    }
    printf("Se ha realizado la conexion de manera exitosa con el servidor \n");



    //char key[10];
    while( strcmp(buffer, "fin\n") != 0  ){
        memset(buffer, 0 , sizeof(buffer));
        printf("enviar al servidor: \n");

        //scanf lee desde la consola pero separa strings al detectar espacios en blanco
        //https://pablohaya.com/2013/10/12/diferencia-entre-scanf-gets-y-fgets/
        //scanf( "%s", buffer);

        //usando fgets para ingresar string por consola
        fgets(buffer, sizeof(buffer)-1, stdin);

        //Upon successful completion, send() returns the number of bytes sent. Otherwise, -1 is returned and errno is set to indicate the error.
        if (send(sockfd, &buffer, sizeof(buffer), 0) < 0 ){
            perror("error al enviar");
        }

        //esto esta al pedo. El hilo se bloquea hasta que recibe algo
        //sleep(1);

        //Upon successful completion, recv() returns the length of the message in bytes. If no messages are available to be received and the peer has performed an orderly shutdown, recv() returns 0. Otherwise, -1 is returned and errno is set to indicate the error.
        if (recv(sockfd, buffer_recepcion, sizeof(buffer_recepcion), 0  ) < 0){
            perror("error al recibir");
        }

        printf("%s", buffer_recepcion);


    }//end while

    if ( shutdown( sockfd, SHUT_RDWR ) < 0 ){
        perror("shutdown fail");
    }

    return 0;
}//end main

long get_uptime(){
    struct sysinfo s_info;
    int error = sysinfo(&s_info);
    if(error != 0)
    {
        printf("code error = %d\n", error);
    }
    return s_info.uptime;
}