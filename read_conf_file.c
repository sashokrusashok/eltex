#include "dhcp_relay.h"

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
                Если считали конфигурация DHCP сервера, то записываем его ip в массив ip[]
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
                    Если считали конфигурацию интерфейса DHCP relay ifname, то записываем его ip в массив ip[]
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
                        Если считали конфигурацию интерфейса DHCP relay ofname, то записываем его ip в массив ip[]
                    */ 

                    if( strncmp(files.configuration, "ofname",6) == 0) 
                    {
                      
                        for(i1=0; files.configuration[8+i1] != '"'; i1++,i++)
                        {
                            ip[i] = files.configuration[8+i1]; 
                        }
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
        В итоге получился массив ip[] с тремя ip адресами, каждый из которых 
        разделяется пробелом, поэтому разобьем его на части с помощью функции
        strtok()
    */

    pch= strtok (ip," ");     
    dhcp_server=pch; 
    pch = strtok (NULL, " ");
    ifname=pch;  
    pch = strtok (NULL, " ");
    ofname=pch;
    return 0;
}
