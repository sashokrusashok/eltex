#include "dhcp_relay.h"
int main(int argc, char *argv[])
{
    int broadcast_sock, unicast_sock, len1,maxfdp1,index_interface,position_opt;
    int bytes_read,opt_on_off=1,bytes,struct_len,nready,result,snooping_on;
    struct sockaddr_in addr_broadcast_interface,addr1,addr_unicast_interface, server_info,client_info;
    struct in_addr addr;
    struct bootp_pkt *dhcp;
    char  buf[1024];
    struct in_addr verification_ip_client;
    struct ifreq ifr;
    uint8_t verification_mac_client[6];
    fd_set rset;
    FD_ZERO(&rset); /* сбрасываем все биты в rset */
    snoop_flag = 0;
    first_client = 0;
    last_add_client_inbase = NULL;

    /*
        Считывается конфигурация DHCP relay: ip адрес сервера, имена входного(ifname) и выходного(ofname) 
        интерфейса, опцию включения/отключения снуппинга,Remote ID и Circuit ID.
    */
    if(argc!=2) 
    {
        printf("Вы забыли указать файл конфигурации\n");
        return 0;
    }
    else
    {
        conf(argv[1]);
    }
    puts(dhcp_server);
    puts(ifname);
    puts(ofname);
    puts(OPTIONS);
    puts(CID);
    /*
       Если в файле конфигурации указана опция "-S" значит снуппинг включен, если в файле не указана опция, то есть "",
       то снуппинг выключен  
    */
    if(strcmp(OPTIONS, "-S") == 0)
    {
        snooping_on = 1;
        puts("Снуппинг включен");
    }
    else
    {
        snooping_on = 0;
        puts("Снуппинг выключен");
    }
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
        return 0;
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
        return 0;   
    }
    /*
            Создание второго сокете unicast_sock, через который DHCP relay будет общаться с 
        сервером по юникасту, так как ретранслятору известен ip адрес сервера

    */    
    unicast_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(unicast_sock < 0)
    {
        perror("socket");
        return 0;
    }
    /*
            Бинд сокета unicast_sock и установка параметра в сокет 
        для повторного использования порта
    */
    addr_unicast_interface.sin_family = AF_INET;
    addr_unicast_interface.sin_port = htons(PORT_SERVER);
    /*
        Структура ifr описывает интерфейс сокета. В данном случае, узнаем ip адрес выходного
        интерфейса, имя которого указано в файле конфигурации
    */
    strncpy(ifr.ifr_name, ofname, IFNAMSIZ-1);
    if(ioctl(unicast_sock, SIOCGIFADDR, &ifr) == -1)
    {
        printf("Ofname interface invalid!\n");
    }
    if (inet_aton(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), (struct in_addr *)&addr_unicast_interface.sin_addr) == 0) 
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
        return 0;    
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
                perror("Error select()");
                return 0;
            }
        }
        /*
            Проверка, на какой дескриптор сокета пришли данные. Полученные данные помещаются в buf.
            В DHCP snooping используются доверенные и недоверенные интерфейсы. Доверенными являются те, 
            данные на который приходят от DHCP сервера, то есть на unicast_sock, а недоверенным  
            является интерфейс, данные на который приходят от клиента, то есть на broadcast_sock.

        */
        if (FD_ISSET(broadcast_sock, &rset)) 
        {
            bytes_read = recvfrom(broadcast_sock, buf, 1024, 0, (struct sockaddr *)&addr1, (socklen_t *)&len1);
            if (bytes_read < 0) 
            {
                printf("Error in function recvfrom()");
            }
            /*
                Приводим полученный заголовок к формату DHCP
            */    
            dhcp = (struct bootp_pkt *)buf;
            /*
                Получение индекса входного интерфейса
            */
            index_interface = if_nametoindex(ifname);
            if (index_interface == 0) 
            {
                perror("if_nametoindex");
                return 0;
            }
            /*
                Если снуппинг включен и на DHCP relay пришел запрос на недоверенный интерфейс (broadcast_sock),
                то есть, который связан с клиентом, то для того, чтобы предотвратить подмену DHCP-сервера,
                проверяется тип пакета, если это DHCPOFFER, DHCPACK или DHCPNAK, которые отправить может 
                только DHCP сервер, значит необходимо этот пакет отбросить. Именно для этой проверки устанавливается flag = 0
            */
            if(snooping_on == 1)
            {
                dhcp_snooping(dhcp->exten[6],dhcp,0,NULL,index_interface);
            }
        }
        else
            if(FD_ISSET(unicast_sock, &rset)) 
            {
                bytes_read = recvfrom(unicast_sock, buf, 1024, 0, (struct sockaddr *)&addr1, (socklen_t *)&len1);
                if (bytes_read < 0) 
                {
                    printf("Error in function recvfrom()");
                }
                dhcp = (struct bootp_pkt *)buf;
            }
        /*
            Если snoop_flag == 1, то это значит, что в запросе пришло одно из сообщений, которые отправляет DHCP-сервер, поэтому он отбрасывается
        */
        if(snoop_flag == 1)
        {
            snoop_flag = 0;
            continue;
        }    
        /*
            Если тип пакета DHCP равен DHCPDISCOVER, значит клиент пытается получить ip адрес от сервера,
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
            /*
                Если пришел запрос и включена функция снуппинга
            */
            if(snooping_on == 1)
            {
                /*
                    Если в клиентском сообщении содержалась 82 опция, необходимо прервать передачу пакета.
                */
                if(position_opt=get_position_opt(DHCP_OPTION_AGENT_INFORMATION,&dhcp->exten[4]) != -1)
                {
                    continue;
                }
                /*
                    По умолчанию устройство, на котором включен DHCP snooping, вставляет опцию 82 в DHCP-запросы,
                    чтобы проинформировать DHCP-сервер о том, от какого DHCP-ретранслятора и через какой его порт был получен запрос.
                    Для этого необходимо найти конец опций (DHCP_OPTION_END) со значением 255, вместо нее вставить
                    опцию 82 и на необходимое количество байт переместить конец опций(DHCP_OPTION_END).
                */
                position_opt=get_position_opt(DHCP_OPTION_END, &dhcp->exten[4]);
                add_opt_82(dhcp,position_opt);
            }
            /*
                Если клиент хочет прекратить аренду IP-адреса или отказаться от него, то это означет, что клиент 
                уже должен быть зарегестрирован в базе данных привязки DHCP (DHCP snooping binding database).
            */
            if(dhcp->op == DHCPRELEASE || dhcp->op == DHCPDECLINE)
            {
                /*
                    Для этого необходимо скопировать его mac,ip и номер интерфейса, на который он пришел и отправить на соответствие
                    в базе данных.         
                */
                memcpy(&verification_ip_client,&dhcp->client_ip,4);
                memcpy(verification_mac_client,dhcp->hw_addr,6);
                /*
                    Если информация пришедшего пакета не соответствует зарегистрированным клиентам,то  необходимо прервать передачу пакета.
                */
                if(result=verification_authorized_client(head,verification_ip_client.s_addr,verification_mac_client, index_interface)==0)
                {
                    continue;
                }
               
            }
            server_info.sin_family = AF_INET;
            server_info.sin_port = htons(PORT_SERVER);
            if (inet_aton(dhcp_server, (struct in_addr *)&server_info.sin_addr) == 0) 
            {
                printf("Address of server is invalid!\n");
                return 0;   
            }
            struct_len = sizeof(server_info);
            /*
                Отправка сообщения серверу с доверенного интерфейса
            */
            bytes = sendto(unicast_sock, buf, 1024, 0, (struct sockaddr *)&server_info, struct_len);
            if (bytes < 0) 
            {
                printf("error in function sendto(), sended %d bytes!\n", bytes);
            } 
        }
        /*
            Если на DHCP relay пришел ответ, то переотправляем его клиенту на 68 порт широковещательно.
            Клиент получит DHCP пакет, так как в поле hw_addr указан его MAC-адрес
        */
        if(dhcp->op == BOOTREPLY)
        {
            if(snooping_on == 1)
            {
                /*
                    Если снуппинг включен, то после того, как DHCP сервер отправит подтверждение получения ip для клиента,
                    необходимо добавить клиента в базу данных привязки DHCP (DHCP snooping binding database), Организованного
                    с помощью односвязного списка.
                */
                if(dhcp->exten[6] == DHCPACK)
                {
                    last_add_client_inbase=dhcp_snooping(dhcp->exten[6],dhcp,1,last_add_client_inbase,index_interface);
                    if(first_client==0)
                    {
                        head = head->next;
                        first_client=1;
                    }
                }
            }
            client_info.sin_family = AF_INET;
            client_info.sin_port = htons(PORT_CLIENT);
            if (inet_aton(BROADCAST, (struct in_addr *)&client_info.sin_addr) == 0) 
            {
                printf("Address of server is invalid!\n");
                return 0;    
            }
            struct_len = sizeof(client_info);
            /*
                Отправка сообщения клиенту с недоверенного интерфейса
            */
            bytes = sendto(broadcast_sock, buf, 1024, 0, (struct sockaddr *)&client_info, struct_len);
            if (bytes < 0) 
            {
                printf("error in function sendto(), sended %d bytes!\n", bytes);
            } 
        }
    }
    /*
        Завершение работы DHCP relay: очистка списка и закрытие слушающих сокетов
    */
    delete_list(head1);
    close(broadcast_sock);
    close(unicast_sock);
    return 0;
}
