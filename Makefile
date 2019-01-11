#makefiles
objects = read_conf_file.o libfs.a dhcp_relay.o dhcp

edit : $(objects) start 

read_conf_file.o: read_conf_file.c
		gcc -c read_conf_file.c

libfs.a: read_conf_file.o
		ar cr libfs.a read_conf_file.o

dhcp_relay.o: dhcp_relay.c
		gcc -c dhcp_relay.c

dhcp: dhcp_relay.o libfs.a
		gcc -Wall -pedantic -o dhcp dhcp_relay.o -L. -lfs

start: dhcp
	./dhcp

