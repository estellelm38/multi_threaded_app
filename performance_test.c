#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "babble_types.h"
#include "babble_communication.h"
#include "babble_utils.h"
#include "babble_client.h"

typedef struct client_thread_data{
    int client_id;
    pthread_barrier_t *gbarrier;
} client_thread_data_t;

/* duration of the test in seconds */
int duration = 2;

char hostname[BABBLE_BUFFER_SIZE]="127.0.0.1";
int portno = BABBLE_PORT;

int with_streaming = 0;

/* reset to stop the test */
volatile int keep_on_going = 1;

/* used to aggregate the results */
double* ops;


static void ALRMhandler (int sig)
{
    keep_on_going = 0;
}


static void display_help(char *exec)
{
    printf("Usage: %s -m hostname -p port_number -d duration -n nb_clients -s [activate_streaming]\n", exec);
    printf("\t hostname can be an ip address\n" );
}

static void *working_thread (void *arg)
{
    int64_t op_count=0;
    struct timespec t0, t1;

    client_thread_data_t *data= (client_thread_data_t*) arg;
    
    char client_name[BABBLE_ID_SIZE];
    memset(client_name, 0, BABBLE_ID_SIZE);
    snprintf(client_name, BABBLE_ID_SIZE, "client_%d", data->client_id);


    int sockfd = connect_to_server(hostname, portno);

    if(sockfd == -1){
        fprintf(stderr,"*** Test Failed ***\n");
        fprintf(stderr,"client %s failed to contact server\n", client_name);
        exit(-1);
    }

    unsigned long client_key= client_login(sockfd, client_name);
    
    if(client_key == 0){
        fprintf(stderr,"*** Test Failed ***\n");
        fprintf(stderr,"client %s failed to login\n", client_name);
        close(sockfd);
        exit(-1);
    }

    /* global barrier before starting to follow others */
    int ret = pthread_barrier_wait(data->gbarrier);
    if (ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD)
    {
        fprintf(stderr, "Barrier synchronization failed!\n");
        return (void*)EXIT_FAILURE;
    }


    char my_msg[BABBLE_PUBLICATION_SIZE];
    memset(my_msg, 0, BABBLE_PUBLICATION_SIZE);
    snprintf(my_msg, BABBLE_PUBLICATION_SIZE, "ping_%d", data->client_id);

    if(clock_gettime(CLOCK_REALTIME, &t0) != 0) {
        perror("Error in calling clock_gettime");
        exit(EXIT_FAILURE);
    }
    
    for (op_count = 0; keep_on_going; op_count++){
        if(client_publish(sockfd, my_msg, with_streaming)){
            fprintf(stderr,"*** Test Failed ***\n");
            fprintf(stderr,"%s failed to publish %s\n", client_name, my_msg);
            close(sockfd);
            exit(-1);
        }
    }

    /* rdv to ensure that all messages have been processed */
    if(client_rdv(sockfd)){
        fprintf(stderr,"*** Test Failed ***\n");
        fprintf(stderr,"%s failed to rdv with server\n", client_name);
        close(sockfd);
        exit(-1);
    }

    if(clock_gettime(CLOCK_REALTIME, &t1) != 0) {
        perror("Error in calling clock_gettime");
        exit(EXIT_FAILURE);
    }

    double t = (double)(t1.tv_sec - t0.tv_sec) + ((double)(t1.tv_nsec - t0.tv_nsec)/1000000000L);
    printf("thread %d: %ld reqs in %lf seconds\n", data->client_id, op_count, t);
    
    /* store the result */
    ops[data->client_id] = ((double)op_count/t);
    
    close(sockfd);
    return (void*)EXIT_SUCCESS;
}



int main(int argc, char *argv[])
{
    int opt;
    int nb_args=1;
    
    pthread_barrier_t global_barrier;

    int nb_threads=4;

    pthread_t *tids=NULL;
    client_thread_data_t *clients_data=NULL;

    int i=0;

    signal (SIGALRM, ALRMhandler);

    
    /* parsing command options */
    while ((opt = getopt (argc, argv, "+hm:p:d:sn:")) != -1){
        switch (opt){
        case 'm':
            strncpy(hostname,optarg,BABBLE_BUFFER_SIZE);
            nb_args+=2;
            break;
        case 'p':
            portno = atoi(optarg);
            nb_args+=2;
            break;
        case 'n':
            nb_threads= atoi(optarg);
            nb_args+=2;
            break;
        case 'd':
            duration= atoi(optarg);
            if(duration > 60){
                printf("duration set to 60 seconds\n");
            }
            nb_args+=2;
            break;
        case 's':
            with_streaming=1;
            nb_args+=1;
            break;
        case 'h':
        case '?':
        default:
            display_help(argv[0]);
            return -1;
        }
    }

    if(nb_args != argc){
        display_help(argv[0]);
        return -1;
    }

    ops = (double*) malloc(nb_threads * sizeof(double));
    memset(ops, 0, nb_threads * sizeof(int64_t));
    
    if(pthread_barrier_init(&global_barrier, NULL, nb_threads+1))
    {
        printf("Could not create a barrier\n");
        return -1;
    }

    printf("starting performance test with %d clients (sending requests during %d seconds)\n", nb_threads, duration);

    
    tids = malloc(sizeof(pthread_t)*nb_threads);
    clients_data = malloc(sizeof(client_thread_data_t)*nb_threads);
    
    for(i=0; i < nb_threads; i++){
        clients_data[i].gbarrier= &global_barrier;
        clients_data[i].client_id = i;
        if(pthread_create (&tids[i], NULL, working_thread, (void*) &clients_data[i]) != 0){
            fprintf(stderr,"WARNING: Failed to create clientthread\n");
        }
        /* we create the threads by groups of 50 to reduce the stress
           during the registration */
        if(nb_threads % 50 == 0){
            usleep(5000);
        }
    }


    /* barrier after all clients registered */
    int ret = pthread_barrier_wait(&global_barrier);
    if (ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD)
    {
        fprintf(stderr, "Barrier synchronization failed!\n");
        return -1;
    }

    /* start measuring time */
    alarm (duration);
    
    printf("**** SUCCESS: All clients registered -- starting the stress test\n");

    for(i=0; i < nb_threads; i++){
        pthread_join (tids[i], NULL) ;
    }

    printf("**** SUCCESS: test terminated ****\n");


    double totops = 0;
    for(i = 0; i < nb_threads; i++){ 
        totops += ops[i];
    }
    
    printf("\n throughput: %.2lf msg/s\n", (double)totops);
  
    
    return 0;
}
