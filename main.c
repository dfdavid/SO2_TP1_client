#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h> // la estructura sockaadr_in pertence a esta libreria
#include <arpa/inet.h>
#include <unistd.h> //en esta libreria esta la funcion sleep(), la funcion read()
#include <sys/sysinfo.h>
#include <stdbool.h>

#define BUFFER_SIZE 1000
#define PORTUDP 5521



char firmware_version[20] = "1.0";
uint16_t server_port= 5520;
char ip_server_buff[32]="192.168.2.7";
char *ip_server = NULL;
unsigned int retry_time=3;
char buffer[BUFFER_SIZE], auxBuffer[BUFFER_SIZE];

long get_uptime(); //esta funcion devuelve el uptime del SO del satelite
//funcion 1
int update_firmware();
//funcion 2
int start_scanning();
int send_telemetria();




int main() {

    printf("ejecutando main \n");

    int sockfd, ip_srv_load;
    struct sockaddr_in dest_addr;

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

    //Conexion
    //Upon successful completion, connect() returns 0. Otherwise, -1 is returned and errno is set to indicate the error.
    while( connect( sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr) ) < 0 ){
        perror("Intentando conectar al servidor");
        //printf("Reintentando coenctar en %d segundos \n", retry_time);
        sleep(retry_time);
    }
    printf("Se ha realizado la conexion de manera exitosa con el servidor \n");


    while( strcmp(buffer_recepcion, "fin\n") != 0  ){
        memset(buffer_recepcion, 0 , sizeof(buffer_recepcion));

        //En este punto se recibe el comando desde la estacion terresre en forma de codigo numerico
        //Upon successful completion, recv() returns the length of the message in bytes. If no messages are available to be received and the peer has performed an orderly shutdown, recv() returns 0. Otherwise, -1 is returned and errno is set to indicate the error.
        if (recv(sockfd, buffer_recepcion, sizeof(buffer_recepcion), 0  ) < 0){
            perror("error al recibir");
        }

        //opciones del lado del cliente:
        /*
        1 - Update Satellite Firmware
        2 - Start Scanning
        3 - Get Telemetry
        */
        printf("%s \n", buffer_recepcion);
        if (strcmp(buffer_recepcion, "1") == 0 ){ // Update Satellite Firmware
            update_firmware();
            send(sockfd, buffer, sizeof(buffer), 0 );
        }
        else if( strcmp(buffer_recepcion, "2") == 0 ){ // Start Scanning
            start_scanning();
        }
        else if( strcmp(buffer_recepcion, "3") == 0 ){ // Get Telemetry
            send_telemetria();
        }
        else{
            printf("DEBUG: se recibio algo distinto de 1 2 o 3 \n");
            sleep(2);
        }


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

//funcion 1
int update_firmware(){
    printf("ha invocado la funcion 'Update Satellite Firmware'\n");
    printf("la version actual del firmware en este dispositivo es: %s\n", firmware_version);
    printf("falta implementar el cuerpo de esta funcion\n");
    char *msg="";
    sprintf(msg, "la version actual del firmware en este dispositivo es: %s", firmware_version);
    strcpy(buffer, msg);

}

//funcion 2
int start_scanning(){
    printf("ha invocado la funcion 'start_scanning' \n");
    printf("falta implementar el cuerpo de esta funcion \n");
}

//funcion 3
int send_telemetria(){
    printf("DEBUG: se hainvocado la funcion send_telemetria \n");

    //implementacion
    char telemetria[200];
    int num =0;
    char *str="a";
    sprintf(telemetria, "%d|%s| \n", num, str);
    printf("DEBUG: telemetria = %s\n", telemetria);

    //envio por UDP
    int sockudp_client;
    struct sockaddr_in st_server; //aca hay que poner la IP que vaya a tener el satellite, quien oficia de server UDP
    sockudp_client = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockudp_client < 0){
        perror("error al abrir el socket UDP en el cliente");
    }

    //limpio la estrucutra que contiene los datos del server
    memset(&st_server, 0, sizeof(st_server));
    //carga de la estructura
    st_server.sin_family=AF_INET;
    // le estoy cargando la ip del servidor al socket, ojo que no se si esta bien porque juan le mete aca la ip de cualerui internfaz del cliente "ANYADDR"
    //Carga de dirección IPv4 del socket
    /*
     aca se le pone directamente la macro INADDR_ANY a la estructura "st_server" porque este sera el socket server UDP
    */
    st_server.sin_addr.s_addr=INADDR_ANY;
    st_server.sin_port=htons(PORTUDP);

    //seteo de las opciones del socket
    int valor=1;
    setsockopt(sockudp_client, SOL_SOCKET, SO_REUSEADDR, &valor, sizeof(valor)); // le indico al SO que puede reutilizar la dir del socket

    //se hace el bind porque en esta fucion "send telemetria" quien oficia de server y recibe una peticion a traves de UDP en este caso es el satelite y no la base terrestre
    if ( bind(sockudp_client, (struct sockaddr *)&st_server, sizeof(st_server) )  < 0 ){
        perror("satellite: error al hacer bind en el socket_udp");
    }

    socklen_t dest_size= sizeof(struct sockaddr);
    char buffer[BUFFER_SIZE];


    printf("esperando que la base solicite la telemetria \n");
    while (strcmp(buffer, "get_tel") != 0){
        // recbir de: estacion terrestre
        //

        if ( recvfrom(sockudp_client, buffer, sizeof(buffer), 0, (struct sockaddr *)&st_server, &dest_size  ) <0 ){
            perror("error al recibir UDP desde etacion terrestre");
        }

    }

    //envio de la telemetria
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer,telemetria);
    if ( sendto(sockudp_client, buffer, strlen(telemetria), 0, (struct sockaddr *)&st_server, dest_size) <0 ){
        perror("error al enviar telemetria por UDP");
        _exit(1);
    }
    printf("DEBUG: telemetria enviada\n");
    sleep(1); //le doy tiempo a la estacion para que parsee la telemetria

    //envio mensaje de finalizacion
    char *finish = "udp_complete";
    if ( sendto(sockudp_client, finish, strlen(finish), 0, (struct sockaddr *)&st_server, dest_size  ) <0 ){
        perror("error al enviar el finish");
        _exit(1);
    }
    printf("DEBUG: mensaje de finalizacion enviado\n");

    //limpio el buffer
    memset(buffer, 0, sizeof(buffer));
    printf("DEBUG: en este punto la telemetria deberia estar enviada\n");

    //shutdown(sock, opt)
    //https://pubs.opengroup.org/onlinepubs/7908799/xns/shutdown.html
    shutdown(sockudp_client, 2); //opcion 2 = SHUT_WR
    close(sockudp_client);

}//fin send_telemetria