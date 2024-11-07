#include "babble_communication.h"
#include "babble_types.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/* writing data of file descriptor */
static int write_data(int fd, unsigned long size, void* buf)
{
    unsigned long total_sent=0;
    unsigned long sent=0;
    
    do {
        sent = write(fd, ((char*) buf+total_sent), size - total_sent);
        if(sent > 0){
            total_sent += sent;
        }
    } while(total_sent < size || (sent == -1 && errno != EINTR) );

    if(sent == -1){
        perror("write_data");
    }
    else{
        if(total_sent < size){
            fprintf(stderr,"sent only %lu/%lu bytes\n", total_sent, size);
        }
    }

    return (total_sent == size) ? total_sent : -1;
}

/* reading data on file descriptor */
static int read_data(int fd, unsigned long size, void* buf)
{
    unsigned long total_recv=0;
    unsigned long recv=0;

    do {
        recv = read(fd, ((char*) buf+total_recv), size - total_recv);
        if (recv > 0){
            total_recv += recv;
        }
    } while(total_recv < size  || (recv == -1 && errno == EINTR));

    if(recv == -1){
        perror("read_data");
    }
    else{    
        if(total_recv < size){
            fprintf(stderr,"received only %lu/%lu bytes\n", total_recv, size);
        }
    }
    
    return (total_recv == size) ? total_recv : -1;
}




int network_send(int fd, unsigned long size, void* buf)
{   
    if(write_data(fd, sizeof(unsigned long), &size) != sizeof(unsigned long)){
        perror("writing on socket");
        return -1;
    }

    
    if(write_data(fd, size, buf) != size){
        perror("writing on socket");
        return -1;
    }
    
    return size;
}


int network_recv(int fd, void **buf)
{
    unsigned long payload_size = 0;
    int r=0;
    

    if((r=read_data(fd, sizeof(unsigned long), &payload_size)) != sizeof(unsigned long)){
        /* fprintf(stderr,"error recv: expected %lu received %d\n", sizeof(unsigned long), r); */
        *buf = NULL;
        return -1;
    }

    char* recv_buf = (char*) malloc(payload_size);

    if((r=read_data(fd, payload_size, recv_buf)) != payload_size){
        /* fprintf(stderr,"error recv: expected %lu received %d\n", payload_size, r); */
        free(recv_buf);
        *buf = NULL;
        return -1;
    }

    *buf = (void*)recv_buf;
    
    return payload_size;
}



