#include "dhcp_relay.h"
/*
    Считывание конфигурации DHCP relay.
*/
int conf(char *p)
{
    int descript_file,read_byte,i,i1;
    struct file_conf files;
    char *pch;
    descript_file = open (p, O_RDONLY);
    if (descript_file < 0)
    {
        printf("Error opening configuration file");
        return 0;
    }
    /*
        Пока файл не закончится, считываем по 25 байт
    */
    while(1)
    {
        read_byte = read (descript_file, files.configuration, SIZE_READ_FILE);
        if (read_byte < 0)
        {
            printf("Error reading configuration file");
            return 0;
        }
        if(read_byte > 0)
        {
            files.configuration[SIZE_READ_FILE-1] = '\0';
            /*
                Если считали конфигурация DHCP сервера, то записываем его ip в массив ip[] и ставим пробел,
                который является разделителем между параметрами DHCP relay и увеличиваем значение i, который 
                будет находиться после пробела
            */
            if( strncmp(files.configuration, "SERVERS",6) == 0) 
            {
                for(i=0; files.configuration[9+i] != '"'; i++)
                {
                    ip[i]=files.configuration[9+i];
                }
                ip[i]=' ';
                i+=1;            
            }
            else
            { 
                /*
                    Если считали имя входного интерфейса DHCP relay(ifname), то записываем его в конец массив ip[]
                */   
                if( strncmp(files.configuration, "ifname",6) == 0) 
                {     
                    for(i1=0; files.configuration[8+i1] != '"'; i1++,i++)
                    {
                      ip[i] = files.configuration[8+i1]; 
                    }
                    
                    ip[i]=' ';
                    i+=1;               
                }
                else
                {
                    /*
                        Если считали имя выходного интерфейса DHCP relay(ofname), то записываем его в конец массив ip[]
                    */ 
                    if( strncmp(files.configuration, "ofname",6) == 0) 
                    {
                        for(i1=0; files.configuration[8+i1] != '"'; i1++,i++)
                        {
                            ip[i] = files.configuration[8+i1]; 
                        }
                        ip[i]=' ';
                        i+=1; 
                    }
                    else
                    {
                        /*
                            Если считали значение опции вкл/выкл снуппинга DHCP relay, то записываем его в конец массив ip[]
                        */
                        if( strncmp(files.configuration, "OPTIONS",6) == 0)
                        {
                            for(i1=0; files.configuration[9+i1] != '"'; i1++,i++)
                            {
                                ip[i] = files.configuration[9+i1]; 
                            }
                            if(i1==0)
                            {
                                ip[i]='n';
                                ++i;
                            }
                            ip[i]=' ';
                            i+=1;
                        }
                        else 
                         /*
                            Если считали номер порта, на который поступают запросы от клиента (Circuit ID) DHCP relay, то записываем его в конец массива ip[]
                        */                          
                            if( strncmp(files.configuration, "CID",3) == 0)
                            {
                                for(i1=0; files.configuration[5+i1] != '"'; i1++,i++)
                                {
                                    ip[i] = files.configuration[5+i1];
                                    
                                }
                                ip[i]=' ';
                                i+=1;
                            }
                            else
                                /*
                                    Если считали идентификатор самого DHCP-ретранслятора (Remote ID), а именного mac-адресс, то записываем его в конец массива ip[]
                                */
                                if( strncmp(files.configuration, "RID",3) == 0)
                                {
                                    for(i1=0; files.configuration[5+i1] != '"'; i1++,i++)
                                    {
                                        ip[i] = files.configuration[5+i1];
                                        
                                    }
                                    i+=1;
                                }
                                else
                                    ip[i]='\0';   
                    }
                }
            }
        }
        else
        {
            break;
        }
    }
    /*
        В итоге получили массив параметров ip[], каждый из которых 
        разделяется пробелом, поэтому разобьем его на части с помощью функции strtok()
    */
    puts(ip);
    pch= strtok (ip," ");     
    dhcp_server=pch; 
    pch = strtok (NULL, " :\0");
    ifname=pch;  
    pch = strtok (NULL, " :\0");
    ofname=pch;
    pch = strtok (NULL, " :\0");
    OPTIONS=pch;
    pch = strtok (NULL, " :\0");
    CID=pch;
    pch = strtok (NULL, " :\0");
    /*
        RID - массив указателей, в каждом из которых содержится строка с частью mac-адреса
    */
    RID[0]=pch;
    pch = strtok (NULL, " :\0");
    RID[1]=pch;
    pch = strtok (NULL, " :\0");
    RID[2]=pch;
    pch = strtok (NULL, " :\0");
    RID[3]=pch;
    pch = strtok (NULL, " :\0");
    RID[4]=pch;
    pch = strtok (NULL, " :\0");
    RID[5]=pch;  
    return 0;
}
/*
    Получение значения опции. После того, как найдена позиция опции, следующим байтом будет описана длинна опции([i+1]).
    В область памяти, на которую указывает value1 помещается величина опции, которая следует сразу же после значения длинны[i+2].
*/
char *get_value_opt(uint8_t opt, uint8_t *dhcp_opt)
{
    int i,len;
    uint8_t *ptr;
    char *value1;
    for(i=0;i<60;i++)
    {
        if(dhcp_opt[i] == opt)
        {
          len = dhcp_opt[i+1];
            ptr = &dhcp_opt[i+2];
            value1 = (char *) malloc(len * sizeof(char));
            for(i=0;i<len;i++)
            {
                sprintf(&value1[i],"%d",ptr[i]);
            }
            break;
        }
    }
    return value1;
}
/*
    Получение позиции опции. Из SIZE_OPTION вычитается 4, потому что в массиве exten первые четыре байта это значение 
    magick cookie DHCP
*/
int get_position_opt(uint8_t opt, uint8_t *dhcp_opt)
{
    int i;
    for(i=0;i<SIZE_OPTION-4;i++)
    {
        if(dhcp_opt[i] == opt)
        {
            return i;
        }
    }
   return -1; 
}
/*
    Добавление 82 опции, начиная с той позиции, где раньше была 255 опция. Функция принимает пакет dhcp, куда допишится 82 опция и позиция с которой необходимо начать запись
*/
void add_opt_82(struct bootp_pkt *dhcp, int position_option)
{
    int i,len_CID;
    len_CID=sizeof(atoi(CID))/4;
    dhcp->exten[position_option] = 0x52;/*значение 82 опции в 16-м виде*/
    dhcp->exten[position_option+1] = len_CID+SIZE_MAC+4;/*длинна опции, которая учитывает длинну значений подопций RID,CID их длинны и номера подопций*/
    dhcp->exten[position_option+2] = 0x01;/*номер подопции (Circuit ID)*/
    dhcp->exten[position_option+3] = len_CID;/*длинна Circuit ID*/
    dhcp->exten[position_option+4] = atoi(CID);/*значение Circuit ID(номер порта, на который поступают запросы от клиента), считанное с файла конфигурации*/
    dhcp->exten[position_option+5] = 0x02;/*номер подопции (Remote ID)*/
    dhcp->exten[position_option+6] = SIZE_MAC;/*длинна Remote ID*/
    for(i=0; i < SIZE_MAC; i++)/*значение Circuit ID(MAC-адрес ретранслятора), считанное с файла конфигурации*/
        dhcp->exten[position_option+7+i] = atoi(RID[i]);
    dhcp->exten[position_option+7+i] = 0xFF;/*конец опций*/
}
/*
    Создание база данных привязки DHCP. Организуется с помощью списка, когда выделяется память для нового элемента списка, то на него ставится указатель, 
    как следующий элемент для предыдущего и заполнятся всего его поля, которые описывают клиента: ip,mac,время аренды ip, vlan, номер интерфейса, на который пришел запрос от клиента   
*/
struct info_client *create(struct info_client *table,struct bootp_pkt *dhcp, int index_interface)
{
    struct info_client *list;
    char *value;                                    
    list = (struct info_client *)malloc(sizeof(struct info_client)); 
    table->next=list;
    table=table->next;
    memcpy(table->macaddr, dhcp->hw_addr, 6); 
    memcpy(&table->rent_ipaddr, &dhcp->client_ip, 4);
    /*
        Получение значения времени аренды ip адреса через 51-ю опцию.
    */
    value=get_value_opt(DHCP_OPTION_LEASE_TIME, &dhcp->exten[4]);
    table->leasetime = value;
    table->vlan=1;
    table->interface=1;
    /*
        Функция возвращает последний созданный элемент списка
    */
    return table; 
}
/*
    Проверка соответствия информации о клиенте, который отправил пакет DHCPRELEASE/DHCPDECLINE, с информацией которая находится в базе данных об этом клиенте.
    Функция принимает, элемент списка, ip,mac и индекс интефейса, на кот пришел запрос
    Если возвращается 0, то информация о клиенте не соответствует.  
*/
int verification_authorized_client(struct info_client *table, int verification_ip_client, uint8_t *verification_mac_client,int index_interface)
{  
    char *ip_client_inbase; 
    while (table != NULL) // пока не конец стека 
    { 
        ip_client_inbase = inet_ntoa(table->rent_ipaddr); 
        if(verification_ip_client==table->rent_ipaddr.s_addr)
        {
            if(strcmp((char*)verification_mac_client,(char*)table->macaddr)==0)
            {
                if(table->interface == index_interface)
                return 1;
            }
        }
        /*
            Переходим к следующей структуре
        */
        table = table->next;
    } 
    return 0;
}
/*
    Функция снуппинга. В функцию поступает тип пакета, сам пакет, флаг, информация о последнем добавленном клиенте в список и индекс интефейса, на который пришел запрос 
*/
struct info_client * dhcp_snooping(int type, struct bootp_pkt *dhcp, int flag,struct info_client *last_add_client_inbase, int index_interface)
{
    struct info_client *table;
    struct in_addr cli_addr;
    /*
        Если установлен флаг ноль, то проверяется не пришло ли одно из сообщений, которые отправляет DHCP-сервер
    */
    if(flag == 0)
    {    
        if((type == DHCPOFFER) || (type == DHCPACK) || (type == DHCPNAK))
        {
            snoop_flag = 1;
        }
    }
    /*
        Если установлен флаг 1, то клиента можно считать зарегестрированным(то есть ему выдали ip), поэтому необходимо заполнить информацию о нем в БД
    */
    if(flag == 1)
    {
    /*
        Если это первый клиент в бд, то создаем заголовок head. head1 будет использован для удаления очереди
    */ 
        if(first_client==0)
        {    
            last_add_client_inbase = (struct info_client *)malloc(sizeof(struct info_client));
            head=last_add_client_inbase;
            head1=head;
        }    
        /*
            После того, как добавился новый элемент списка, функция create() возвращает последний созданный указатель на адрес нового клиента в БД
        */ 
        last_add_client_inbase = create(last_add_client_inbase,dhcp,index_interface); 
        last_add_client_inbase->next=NULL;
        return last_add_client_inbase;
    }
}
/*
    Удаление списка.
*/
int delete_list(struct info_client *table)
{  
    struct info_client *prev;
    while (table != NULL) 
    { 
        prev = table;
        table = table->next;
        /*Для prev->leasetime была выделина динамическая память, но так как для заголовка она не выделялась, то в заголовке ее удалять не надо*/
        if(head_flag == 0)
            head_flag = 1;
        else
            free(prev->leasetime);
        free(prev);
    } 
    return 0;
}
