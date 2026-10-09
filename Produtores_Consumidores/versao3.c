#include <pthread.h>
#include <stdio.h>
#include<unistd.h>

#define TAM_BUFFER 10
#define NUM_PRODUTORES 2
#define NUM_CONSUMIDORES 2

int buffer[TAM_BUFFER];
int contador_itens = 0;

void* produtor(void* arg){
    long id = (long) arg;

    while(1){
        sleep(1);

        if(contador_itens < TAM_BUFFER){
            buffer[contador_itens] = 7; //valor genérico
            contador_itens++;
            printf("Processo %ld produzindo. Itens no buffer: %d\n\n",id,contador_itens);

        }
    }
    pthread_exit(NULL);
}

void* consumidor(void* arg){
    long id = (long) arg;

    while(1){
        sleep(2);

        if(contador_itens > 0){
            contador_itens--;
            printf("Processo Consumidor %ld consumindo. Itens restantes: %d\n\n",id,contador_itens);
        }
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