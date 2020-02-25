#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h> // la estructura sockaadr_in pertence a esta libreria
#include <arpa/inet.h>
#include <unistd.h> //en esta libreria esta la funcion sleep(), la funcion read()
#include <sys/sysinfo.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024
#define PORTUDP 5521
#define FIRMWARE_FILE "./updated_firmaware_received"
#define FILE_BUFFER_SIZE 1500
#define ARCHIVO_IMAGEN "../data/20200481950_GOES16-ABI-FD-GEOCOLOR-10848x10848.jpg"
#define ID 0

char firmware_version[20] = "1.0";
uint16_t server_port= 5520;
char ip_server_buff[32]="192.168.2.7";
char *ip_server = NULL;
unsigned int retry_time=3;
//char buffer[BUFFER_SIZE], auxBuffer[BUFFER_SIZE];


long get_uptime(); //esta funcion devuelve el uptime del SO del satelite
//funcion 1
int update_firmware(int sockfd_arg);
//funcion 2
int start_scanning(int sockfd);
int send_telemetria();
void get_dir();


/**
 * @brief Programa cliente. Tiene como objetivo simular el firmaware del satelite que se conecta al programa servidor (Base Terrestre) y queda a la espera de ordenes del mismo.
 * @return
 */
int main() {

    printf("DEBUG: ejecutando main \n");
    printf("Version del firmware: %s\n", firmware_version);
    int sockfd;
    //int ip_srv_load;
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
        printf("%s\n", buffer_recepcion);
        if (strcmp(buffer_recepcion, "1") == 0 ){ // Update Satellite Firmware
            memset(buffer_recepcion, 0, sizeof(buffer_recepcion));

            //recv(sockfd, buffer_recepcion, sizeof(buffer_recepcion), 0 );

            update_firmware(sockfd); // le paso a la func el sockfd que debe usar ya que ahi llegaran los datos

        }
        else if( strcmp(buffer_recepcion, "2") == 0 ){ // Start Scanning
            start_scanning(sockfd);
        }
        else if( strcmp(buffer_recepcion, "3") == 0 ){ // Get Telemetry
            send_telemetria();
        }
        else{
            printf("DEBUG: se recibio algo distinto de 1 2 o 3\n");
            sleep(2);
        }


    }//end while

    if ( shutdown( sockfd, SHUT_RDWR ) < 0 ){
        perror("shutdown fail");
    }

    return 0;
}//end main

/**
 * @brief Funcion que devuelve el uptime del sistema cuando es invacada
 * @return Devuelve un long con el uptime del sistema en segundos
 */
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
/**
 * @brief Recibe mediante el socjet TCP de la comunicacion establecida un nuevo firmware. Luego de recibirlo satisfactoriamente se cierra el proceso actual y se ejecuta en nuevo firmaware perdiendo la conexion.
 * @param sockfd_arg El socket de la comunicacion TCP establecida.
 * @return Devuelve 0 si no se ha podido crear en el file system el archivo para la recepcion del firmware. Devuelve 1 en caso de que no se haya podido reiniciar el proceso cliente.
 */
int update_firmware(int sockfd_arg){
    printf("DEBUG: ha invocado la funcion 'Update Satellite Firmware'\n");
    printf("la version actual del firmware en este dispositivo es: %s\n", firmware_version);
    int firmware_fd;
    int bytes_escritos=0;

    //try to open fd
    /*
    //https://pubs.opengroup.org/onlinepubs/009695399/functions/open.html
    //int open(const char *path, int oflag, file_permissions );
    */
    if ( (firmware_fd=open(FIRMWARE_FILE, O_WRONLY|O_CREAT|O_TRUNC, 0777) ) < 0 ){
        perror("error al crear el archivo");
        return 0;
    }

    char buffer_recepcion[FILE_BUFFER_SIZE]; //FILE_BUFFER_SIZE=16000
    long byte_leido;

    uint32_t bytes_recibidos;
    //atencion que estoy leyendo de a 4 bytes a la vez, no leo el stream completo
    if ( (byte_leido=recv(sockfd_arg, &bytes_recibidos, 4, 0) ) != 0 ){
        if ( byte_leido <= 0 ){
            perror("error en la recepcion a traves del socket_arg");
        }

    }

    //se transforma el numero de bytes recibidos de network a host long (uint32_t)
    bytes_recibidos=ntohl(bytes_recibidos);
    printf("Cantidad de bytes en el archivo a recibir (stream TCP): %i\n", bytes_recibidos);


    //mientras queden bytes sin leer...
    while(bytes_recibidos){
        memset(buffer_recepcion, 0, sizeof(buffer_recepcion));

        //controlo un prosible error de recepcion
        if( (byte_leido = recv(sockfd_arg, buffer_recepcion, sizeof(buffer_recepcion), 0)) != 0){
            if (byte_leido < 0){
                perror("error en la recepcion en el socket 'sockfd_arg'");
            }
        }

        //voy a ir escribiendo el archivo que cree oon los bytes que vaya "sacando/leyendo" del socket
        if(0 > (bytes_escritos = write(firmware_fd, buffer_recepcion, (size_t) byte_leido))  ){
            perror("error al escribir el archivo creado");
            _exit(EXIT_FAILURE);
        }

        printf("%d\n", bytes_escritos);
        bytes_recibidos -= byte_leido;
    }

    //cierro el file descriptor
    close(firmware_fd);

    printf("DEBUG: Finalizada la recepcion del archivo desde la estacion terrestre\n");
    printf("reiniciando el sistema con el nuevo firmware\n");
    //cierro tambien el socket_arg
    close(sockfd_arg); //esto lo cierro solocuando implemente el RE-EJECUTAR

    //aca esta faltando la parte de reiniciar automaticamente el programa con la nueva version de firmware
    char ejecutable[100] = "";
    strcat(ejecutable, FIRMWARE_FILE );
    char *argv[] = {FIRMWARE_FILE, NULL};
    //ejemplode uso de excev
    /*
     https://pubs.opengroup.org/onlinepubs/009695399/functions/exec.html
     Using execv()

        The following example passes arguments to the ls command in the cmd array.

        #include <unistd.h>


        int ret;
        char *cmd[] = { "ls", "-l", (char *)0 };
        ...
        ret = execv ("/bin/ls", cmd);
    */
    if (execv(ejecutable, argv) < 0){
        perror("error al reiniciar");
    }
    return 1;
}

//funcion 2
/**
 * @brief Abre el archivo de imagen y lo envia al proceso servidor a traces del socket TCP abierto en la comunicacion establecida.
 * @param sockfd_arg2 Socketfd TCP abierto en la comunicacion cliente servidor
 * @return Devuelve -1 cuando no se podido abrir el archivo de imagen a enviar para su lectura. Devuelve 1 al completar con exito el envio de la imagen.
 */
int start_scanning(int sockfd_arg2){
    printf("DEBUG: ha invocado la funcion 'start_scanning' \n");
    int imagen_fd;
    struct stat wtf;
    char *archivo_imagen=ARCHIVO_IMAGEN;
    if ((imagen_fd=open(archivo_imagen, O_RDONLY)) < 0){
        perror("error al abrir el archivo de imagen\n");
        return -1; //esto es cero porque da error. Se le podria cambiar a otro valor como -1
    }

    int count;
    char buffer_envio2[FILE_BUFFER_SIZE];
    fstat(imagen_fd, &wtf);
    off_t file_size= wtf.st_size;
    printf("DEBUG: tamaño del archivo a enviar %li bytes\n", file_size);
    u_int32_t bytes=htonl(file_size);
    char *send_bytes = (char*)&bytes;
    printf("DEBUG: n° de bytes a enviar: %i\n", ntohl(bytes) );

    if(send(sockfd_arg2, send_bytes, sizeof(bytes), 0 ) <0 ){
        perror("error al enviar el archivo de imagen");
    }

    while ( (count = read(imagen_fd, buffer_envio2, FILE_BUFFER_SIZE) ) > 0){
        if(send(sockfd_arg2, buffer_envio2, count, 0) < 0 ){
            perror("error al enviar el archivo de imagen en la suscesion de bytes");
        }
        memset(buffer_envio2, 0, sizeof(buffer_envio2));
    }
    close(imagen_fd);
    printf("DEBUG: envio de la imagen finalizado\n");
    return 1; //se retorna 1 al completar con exito el envio de la imagen

}

//funcion 3
/**
 * @brief Abre un socket UDP con el numero de puerto especificado en la macro PORTUDP. Obtiene la informacion de telemetria y la envia por el socket UDP recientemente abierto.
 * @return Devuelve 0 si no se ha podido abrir el socket UDP. Devuelve 1 si la telemetria se ha enviado satisfactoriamente. No hay garantia de la integridad ni de la recepcion de los datos.
 */
int send_telemetria(){
    printf("DEBUG: se hainvocado la funcion send_telemetria \n");

    //implementacion
    char telemetria[200];
    struct sysinfo s_info;
    long upt= get_uptime();
    u_long fram = s_info.freeram;

    sprintf(telemetria, "%d | %ld | %s | %lu ", ID, upt, firmware_version, fram );
    printf("DEBUG: telemetria = %s\n", telemetria);

    //envio por UDP
    int sockudp_client;
    struct sockaddr_in st_server; //aca hay que poner la IP que vaya a tener el satellite, quien oficia de server UDP
    sockudp_client = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockudp_client < 0){
        perror("error al abrir el socket UDP en el cliente");
        return 0;
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
    return 1;

}//fin send_telemetria

/**
 *@brief Funcion simple que imprime en consola el path absoluto en el cual se encuentra el ejecutable del programa que la invoca.
 */
void get_dir() {
    char cwd[BUFFER_SIZE];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working dir: %s\n", cwd);
    } else {
        perror("getcwd() error");
    }
}
