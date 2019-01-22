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
#include <sys/ioctl.h>
#include <net/if.h>

#define BROADCAST "255.255.255.255"
#define BOOTREQUEST 1 /* DHCP запрос, приходящий от клиента */
#define BOOTREPLY 2 /* DHCP ответ, приходящий от сервера */
#define PORT_CLIENT 68 /* порт DHCP клиента */
#define PORT_SERVER 67 /* порт DHCP сервера */
#define SIZE_READ_FILE 25 /* конфигурационный файл считывается по 25 байт */
#define DHCPDISCOVER 1 /* тип DHCP пакета */
#define DHCPOFFER 2
#define DHCPDECLINE 4
#define DHCPACK 5
#define DHCPNAK 6
#define DHCPRELEASE 7
#define DHCP_OPTION_LEASE_TIME 51
#define DHCP_OPTION_END 255
#define SIZE_MAC 6
#define SIZE_OPTION 100
#define DHCP_OPTION_AGENT_INFORMATION 82

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
    uint8_t exten[SIZE_OPTION]; 
};
/* Информация о клиенте */
struct info_client {
    uint8_t macaddr[6];         /* MAC-адрес клиента */
    struct in_addr rent_ipaddr; /* Арендованный IP-адрес клиента */
    char *leasetime;  /* Время аренды */
    int vlan;                   /* Идентификатор VLAN */
    unsigned char   interface;  /* Интерфейс, на который приходят данные от клиента */
    struct info_client *next;/* Следующая структура*/
};

char ip[100],*dhcp_server,*ifname,*ofname,*OPTIONS,*CID,*RID[6];
int snoop_flag,first_client;
struct info_client *head,*head1;
struct info_client *last_add_client_inbase; /*последний добавленный клиент в базу данных*/

int conf(char *);
char *get_value_opt(uint8_t opt, uint8_t *dhcp_opt);
struct info_client *create(struct info_client *table,struct bootp_pkt *dhcp, int index_interface);
int verification_authorized_client(struct info_client *table, int verification_ip_client, uint8_t *verification_mac_client,int index_interface);
struct info_client * dhcp_snooping(int type, struct bootp_pkt *dhcp, int flag,struct info_client *last_add_client_inbase, int index_interface);
int get_position_opt(uint8_t opt, uint8_t *dhcp_opt);
void add_opt_82(struct bootp_pkt *dhcp, int position_option);
int delete_list(struct info_client *table);
