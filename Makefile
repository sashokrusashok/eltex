#makefiles
#tyutyutyu
objects = all_functions.o libfs.a dhcp_relay.o dhcp

edit : $(objects) start clean

all_functions.o: all_functions.c
		gcc -c all_functions.c

libfs.a: all_functions.o
		ar cr libfs.a all_functions.o

dhcp_relay.o: dhcp_relay.c
		gcc -c dhcp_relay.c

dhcp: dhcp_relay.o libfs.a
		gcc -Wall -pedantic -o dhcp dhcp_relay.o -L. -lfs

start: dhcp
		./dhcp conf_dh_relay

clean: 
		rm $(objects)
