#include "dhcp_relay.h"

int main()
{
    int broadcast_sock, unicast_sock, len1,maxfdp1;
    int bytes_read,opt_on_off=1,bytes,struct_len,nready;
    struct sockaddr_in addr_broadcast_interface,addr1,addr_unicast_interface, server_info,client_info;
    struct bootp_pkt *dhcp;
    char  buf[1024];
    fd_set rset;
    FD_ZERO(&rset); /* сбрасываем все биты в rset */

    /*
            Считываем конфигурацию DHCP relay, а именно ip адрес сервера, а так же 
        интерфейсы ifname и ofname DHCP relay
    */

    conf("conf_dh_relay");
    puts(dhcp_server);
    puts(ifname);
    puts(ofname);

    /*
        Инициализация структур нулями
    */
    
    memset(&addr_broadcast_interface, 0, sizeof(addr_broadcast_interface));
    memset(&addr_unicast_interface, 0, sizeof(addr_unicast_interface));
    memset(&addr1, 0, sizeof(addr1));
    memset(&server_info, 0, sizeof(server_info));
    memset(&client_info, 0, sizeof(client_info));

    /*
            Создание первого сокета (broadcast_sock), через который DHCP relay будет обмениваться
        запросами и ответами с клиентом широковещательно, так как клиенту еще не был назначен ip адрес
    */

    broadcast_sock = socket(AF_INET, SOCK_DGRAM, 0);

    if(broadcast_sock < 0)
    {
        perror("socket");
        exit(1);
    }

    /*
            Так как 67 порт является зарезервированным портом DHCP сервера и клиент отправляет 
        широковещательный запрос именно на этот порт, то чтобы DHCP relay мог использовать 
        его повторно, необходимо установить в сокет broadcast_sock параметр SO_REUSEADDR
    */

    if(setsockopt(broadcast_sock,SOL_SOCKET,SO_REUSEADDR,(char *)&opt_on_off,sizeof(opt_on_off))<0)
    {
        printf("Error: Could not set reuse address option on DHCP socket!\n");
        return 0;
    }

    /*
            Так же чтобы DHCP relay мог отправлять широковещательные ответы обратно клиенту, 
        неоходимо установить в сокет broadcast_sock параметр SO_BROADCAST
    */    

    if(setsockopt(broadcast_sock, SOL_SOCKET, SO_BROADCAST, &opt_on_off, sizeof(opt_on_off))<0)
    {
        printf("Error: Could not set reuse address option on DHCP socket!\n");
        return 0;
    }

    /*
        Бинд широковещательный сокета broadcast_sock
    */

    addr_broadcast_interface.sin_family = AF_INET;
    addr_broadcast_interface.sin_port = htons(PORT_SERVER);
    addr_broadcast_interface.sin_addr.s_addr=INADDR_ANY;

    if(bind(broadcast_sock, (struct sockaddr *)&addr_broadcast_interface, sizeof(addr_broadcast_interface)) < 0)
    {
        perror("bind1");
        exit(2);    
    }

    /*
            Создание второго сокете unicast_sock, через который DHCP relay будет общаться с 
        сервером по юникасту, так как ретранслятору известен ip адрес сервера

    */    

    unicast_sock = socket(AF_INET, SOCK_DGRAM, 0);

    if(unicast_sock < 0)
    {
        perror("socket");
        exit(1);
    }

    /*
            Бинд сокета unicast_sock и установка параметра в сокет 
        для повторного использования порта
    */

    addr_unicast_interface.sin_family = AF_INET;
    addr_unicast_interface.sin_port = htons(PORT_SERVER);

    if (inet_aton(ofname, (struct in_addr *)&addr_unicast_interface.sin_addr) == 0) 
    {
        printf("Address of server is invalid!\n");
        return 0;
    }

    if(setsockopt(unicast_sock,SOL_SOCKET,SO_REUSEADDR,(char *)&opt_on_off,sizeof(opt_on_off))<0)
    {
        printf("Error: Could not set reuse address option on DHCP socket!\n");
        return 0;
    }

    if(bind(unicast_sock, (struct sockaddr *)&addr_unicast_interface, sizeof(addr_unicast_interface)) < 0)
    {
        perror("bind1");
        exit(2);    
    }

    /*
            Выбор максимального значения дескриптора из двух созданных сокетов
        и получаем величину maxfdp1, которое на 1 больше максимального. maxfdp1 
        применяется в функции select() 
    */

    if(broadcast_sock > unicast_sock)
        maxfdp1=broadcast_sock+1;
    else
        maxfdp1=unicast_sock+1;
    len1=sizeof(addr1);
  
    while(1)
    {

        /*
                Добавление дескрипторов в rset
        */

        FD_SET(broadcast_sock, &rset);
        FD_SET(unicast_sock, &rset);

        /* 
                Функция select() позволяет одновременно ждать появление данных на двух 
            дескрипторах сокета (unicast_sock и addr_broadcast_interface), добавленные rset
        */

        if ((nready = select(maxfdp1, &rset, NULL, NULL, NULL)) < 1) 
        {
            if (nready < 1) 
            {
                perror("server5");
                exit(1);
            }
        }

        /*
            Проверка, на какой дескриптор сокета пришли данные. Полученные данные помещаются в buf
        */

        if (FD_ISSET(broadcast_sock, &rset)) 
        {
            bytes_read = recvfrom(broadcast_sock, buf, 1024, 0, (struct sockaddr *)&addr1, (socklen_t *)&len1);
        }
        else
            if(FD_ISSET(unicast_sock, &rset)) 
            {
                bytes_read = recvfrom(unicast_sock, buf, 1024, 0, (struct sockaddr *)&addr1, (socklen_t *)&len1);
            }

        /*
            Приводим полученный заголовок к формату DHCP
        */    

        dhcp = (struct bootp_pkt *)buf;

        /*
                Если тип пакета DHCP равен DHCPDISCOVER, значит клиент пытается получить ip адрес от клиента,
            но так как между клиентом и сервером находится DHCP relay,то увеличивается 
            количество хопов между ними в поле hops, а так же записываем ip адрес DHCP relay в поле relay_ip. 
            Используется именно 6-й байт массива exten[6] при проверки типа пакета,
            потому что первые четыре описываю magick cookie DHCP, следующие три байта описывают 
            53-ю опцию, в которой в 3-ем байте содердится тип пакета
        */

        if(dhcp->exten[6] == DHCPDISCOVER)
        {
            dhcp->hops++;
            dhcp->relay_ip=inet_addr(ofname);
        }

        /*
                Если на DHCP relay пришел запрос, то переотправляем его серверу на 67 порт и на ip адрес, 
            указанном в конфигурации
        */
        
        if(dhcp->op == BOOTREQUEST)
        {
            server_info.sin_family = AF_INET;
            server_info.sin_port = htons(PORT_SERVER);
            if (inet_aton(dhcp_server, (struct in_addr *)&server_info.sin_addr) == 0) 
            {
                printf("Address of server is invalid!\n");
                return 0;   
            }
            struct_len = sizeof(server_info);
            bytes = sendto(unicast_sock, buf, bytes_read, 0, (struct sockaddr *)&server_info, struct_len);
            if (bytes < 0) 
            {
                printf("error in function sendto(), sended %d bytes!\n", bytes);
            } 
        }

        /*
                Если на DHCP relay пришел ответ, то переотправляем его клиенту на 68 порт широковещательно, 
            клиент получит DHCP пакет, так как в поле hw_addr указан его MAC-адрес
        */

        if(dhcp->op == BOOTREPLY)
        {
            client_info.sin_family = AF_INET;
            client_info.sin_port = htons(PORT_CLIENT);
            if (inet_aton(BROADCAST, (struct in_addr *)&client_info.sin_addr) == 0) 
            {
                printf("Address of server is invalid!\n");
                return 0;    
            }
            struct_len = sizeof(client_info);
            bytes = sendto(broadcast_sock, buf, bytes_read, 0, (struct sockaddr *)&client_info, struct_len);
            if (bytes < 0) 
            {
                printf("error in function sendto(), sended %d bytes!\n", bytes);
            } 
        }
    }

    close(broadcast_sock);
    close(unicast_sock);

    return 0;
}
