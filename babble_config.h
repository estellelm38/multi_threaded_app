#ifndef __BABBLE_CONFIG_H__
#define __BABBLE_CONFIG_H__

/*********************************************/
/* Config options that should not be changed */
#define BABBLE_BUFFER_SIZE 256  /* Maximum size for a char buffer -- including
                                   input buffer when reading from the console */
#define BABBLE_PUBLICATION_SIZE 64   /* Maximum size for a publication */
#define BABBLE_ID_SIZE 32  /* Maximum size for a client identifier */
/*********************************************/

#define BABBLE_BACKLOG 100

#define BABBLE_PORT 5656
#define MAX_CLIENT 1000
#define MAX_FOLLOW MAX_CLIENT


#define BABBLE_TIMELINE_MAX 4

#define BABBLE_EXECUTOR_THREADS 1

/* defines the size of the prod-cons buffer */
#define BABBLE_PRODCONS_SIZE 4

/* defines the number of prodcons buffers in stage 3 */
#define BABBLE_PRODCONS_NB 1

/* expressed in micro-seconds */
#define MAX_DELAY 10000

#endif
