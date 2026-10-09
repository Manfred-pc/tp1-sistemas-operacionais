#include<pthread.h>
#include<semaphore.h>
#include<stdio.h>
#include<unistd.h>

#define TAM_BUFFER 5
#define NUM_PRODUTORES 3
#define NUM_CONSUMIDORES 1

int buffer[TAM_BUFFER];
int contador_itens = 0;
int in = 0;
int out = 0;

sem_t espacos_vazios;
sem_t itens_disponiveis;
pthread_mutex_t mutex;

void* produtor(void*  arg){
    long id = (long)arg;
    int dado = 1;

    while(1){
        sleep(1);
        
        // se true (retornar diferente de 0), o buffer ta cheio
        if (sem_trywait(&espacos_vazios) != 0) {
            printf("Processo Produtor %ld dormindo...\n\n", id);
            // Agora sim, como sabemos que está cheio, mandamos a thread bloquear e esperar
            sem_wait(&espacos_vazios);
        }

        pthread_mutex_lock(&mutex);

        buffer[in] = dado;
        contador_itens++;
        printf("Produtor %ld produzindo o valor %d (Posicao %d). Total no buffer: %d. ", id, dado, in, contador_itens);
        in = (in+1) % TAM_BUFFER;
        dado++;

        printf("Estado do Buffer: [ ");
        for(int i = 0; i < TAM_BUFFER; i++) {
            printf("%d ", buffer[i]); 
        }
        printf("]\n\n");

        pthread_mutex_unlock(&mutex);

        sem_post(&itens_disponiveis);
    }

    pthread_exit(NULL);
}

void* consumidor(void* arg){
    long id = (long) arg;

    while(1){
        sleep(2);

        
        if (sem_trywait(&itens_disponiveis) != 0) {
            printf("Processo Consumidor %ld dormindo...\n\n", id);
            // bloqueia e espera alguém produzir
            sem_wait(&itens_disponiveis);
        }

        pthread_mutex_lock(&mutex);
        
        int pedido = buffer[out];
        contador_itens--;
        printf("Consumidor %ld consumindo o valor %d (Posicao %d). Total no buffer: %d. ",id,pedido,out,contador_itens);
        out = (out + 1) % TAM_BUFFER;

        printf("Estado do Buffer: [ ");
        for(int i = 0; i < TAM_BUFFER; i++) {
            printf("%d ", buffer[i]); 
        }
        printf("]\n\n");

        pthread_mutex_unlock(&mutex);

        sem_post(&espacos_vazios);
    }
    pthread_exit(NULL);
}

int main(){
    pthread_t produtores[NUM_PRODUTORES];
    pthread_t consumidores[NUM_CONSUMIDORES];

    sem_init(&espacos_vazios, 0, TAM_BUFFER);
    sem_init(&itens_disponiveis, 0, 0);
    pthread_mutex_init(&mutex, NULL);

    for(long i = 0; i < NUM_PRODUTORES; i++){
        pthread_create(&produtores[i], NULL, produtor, (void*)i);
    }

    for(long i = 0; i < NUM_CONSUMIDORES; i++){
        pthread_create(&consumidores[i], NULL, consumidor, (void*)i);
    }

    for(int i = 0; i < NUM_PRODUTORES; i++){
        pthread_join(produtores[i], NULL);
    }

    for(int i = 0; i < NUM_CONSUMIDORES; i++){
        pthread_join(consumidores[i], NULL);
    }

    sem_destroy(&espacos_vazios);
    sem_destroy(&itens_disponiveis);
    pthread_mutex_destroy(&mutex);

    return 0;
}