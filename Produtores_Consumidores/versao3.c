#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define TAM_BUFFER 10
#define NUM_PRODUTORES 2
#define NUM_CONSUMIDORES 2

int buffer[TAM_BUFFER];
int contador_itens = 0;
int in = 0;  // Índice para o produtor
int out = 0; // Índice para o consumidor

void* produtor(void* arg){
    long id = (long) arg;
    int dado = 7; // valor genérico

    while(1){
        sleep(1);
        
        buffer[in] = dado;
        contador_itens++;
        printf("Estado do Buffer: [ ");
        for(int i = 0; i < TAM_BUFFER; i++) {
            printf("%d ", buffer[i]); 
        }
        printf("]\n\n");
        printf("Processo Produtor %ld produzindo (Posicao %d). Itens no buffer: %d. ", id, in, contador_itens);
        printf("Estado do Buffer: [ ");
        for(int i = 0; i < TAM_BUFFER; i++) {
            printf("%d ", buffer[i]); 
        }
        printf("]\n\n");

        in = (in + 1) % TAM_BUFFER; 
    }
    pthread_exit(NULL);
}

void* consumidor(void* arg){
    long id = (long) arg;

    while(1){
        sleep(2);

        contador_itens--;

        printf("Processo Consumidor %ld consumindo (Posicao %d). Itens restantes: %d. ", id, out, contador_itens);
        printf("Estado do Buffer: [ ");
        for(int i = 0; i < TAM_BUFFER; i++) {
            printf("%d ", buffer[i]); 
        }
        printf("]\n\n");

        out = (out + 1) % TAM_BUFFER;
    }
    pthread_exit(NULL);
}

int main(){
    pthread_t produtores[NUM_PRODUTORES];
    pthread_t consumidores[NUM_CONSUMIDORES];

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

    return 0;
}