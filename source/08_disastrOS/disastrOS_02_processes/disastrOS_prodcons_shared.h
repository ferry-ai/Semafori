#define SHM_NAME "/disastros_prodcons_shared"


typedef struct{
    int buffer[BUFFER_SIZE];
    int read_index;
    int write_index;

    sem_t mut;
    sem_t e;
    sem_t n;
} ProdConsShared;