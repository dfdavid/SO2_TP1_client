#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h> // la estructura sockaadr_in pertence a esta libreria
#include <arpa/inet.h>
#include <unistd.h> //en esta libreria esta la funcion sleep(), la funcion read()

#define BUFFER_SIZE 1000
#define SERVER_PORT 10000



char firmware_version[20] = "1.0";
uint16_t server_port= 5520;
char ip_server_buff[32]="192.168.2.7";
char *ip_server = NULL;



int main() {

    int sockfd, ip_srv_load;
    struct sockaddr_in dest_addr;
    char buffer[BUFFER_SIZE], auxBuffer[BUFFER_SIZE];


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
    }
    printf("Se ha realizado la conexion de manera exitosa con el servidor \n");
    //printf("cerrando programa en 5 segundos...");
    //sleep(5);

    strcpy(buffer, "probando el envio" );
    //Upon successful completion, send() returns the number of bytes sent. Otherwise, -1 is returned and errno is set to indicate the error.

    if (send(sockfd, &buffer, sizeof(buffer), 0) < 0 ){
        perror("error al enviar");
    }









    //realizar conexion



    return 0;
}