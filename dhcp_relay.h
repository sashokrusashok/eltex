#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/un.h>

#define BROADCAST "255.255.255.255"
#define BOOTREQUEST 1 /* DHCP запрос, приходящий от клиента */
#define BOOTREPLY 2 /* DHCP ответ, приходящий от сервера */
#define PORT_CLIENT 68 /* порт DHCP клиента */
#define PORT_SERVER 67 /* порт DHCP сервера */
#define SIZE_READ_FILE 25 /* конфигурационный файл считывается по 25 байт */
#define DHCPDISCOVER 1 /* тип DHCP пакета */

struct file_conf
{
    char configuration[SIZE_READ_FILE];
};

/* формат DHCP пакета */

struct bootp_pkt { 
    uint8_t op;             
    uint8_t htype;          
    uint8_t hlen;          
    uint8_t hops;          
    uint32_t xid;           
    uint16_t secs;          
    uint16_t flags;         
    uint32_t client_ip;     
    uint32_t your_ip;       
    uint32_t server_ip;     
    uint32_t relay_ip;      
    uint8_t hw_addr[16];    
    uint8_t serv_name[64];  
    uint8_t boot_file[128]; 
    uint8_t exten[64]; 
};

char ip[100],*dhcp_server,*ifname,*ofname;

/*
    Считывание файла конфигурации
*/

int conf(char *);
