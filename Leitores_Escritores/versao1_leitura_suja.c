// Versao 1: Leitores e escritores sem preferencia de acesso (demonstracao de leitura suja)

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include "conta.h"

// Cores ANSI para o terminal
#define ANSI_RESET   "\x1b[0m"
#define ANSI_BOLD    "\x1b[1m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"

// Instancia compartilhada da conta bancaria
ContaBancaria* g_conta = NULL;

// Semaforo para exclusao mutua entre escritores
sem_t sem_escritores;

// Estatisticas
int g_total_leituras_sujas = 0;
pthread_mutex_t mutex_estatisticas;

// Parametros padrao
int g_num_leitores = 4;
int g_num_escritores = 3;
int g_delay_leitor_ms = 80;
int g_delay_escritor_ms = 250;

double valores_escritores[100];
double saldo_esperado_matematico = 0.0;

typedef struct {
    long id;
    double valor_operacao;
} ThreadArg;

void* thread_escritora(void* arg) {
    ThreadArg* dados = (ThreadArg*)arg;
    long id = dados->id;
    double valor = dados->valor_operacao;

    printf(ANSI_YELLOW "[ESCRITOR %ld] Criado: %s R$ %.2f\n" ANSI_RESET,
           id, (valor >= 0 ? "deposito de" : "saque de"), (valor >= 0 ? valor : -valor));

    usleep((rand() % 30) * 100000);

    printf(ANSI_YELLOW "[ESCRITOR %ld] Aguardando acesso exclusivo...\n" ANSI_RESET, id);

    sem_wait(&sem_escritores);

    printf(ANSI_BOLD ANSI_MAGENTA "[ESCRITOR %ld] Entrou na secao critica\n" ANSI_RESET, id);

    double novo_saldo = conta_atualizar(g_conta, id, valor, g_delay_escritor_ms);

    printf(ANSI_BOLD ANSI_MAGENTA "[ESCRITOR %ld] Operacao concluida. Novo saldo = R$ %.2f (total ops: %d)\n" ANSI_RESET,
           id, novo_saldo, conta_obter_operacoes(g_conta));

    printf(ANSI_MAGENTA "[ESCRITOR %ld] Saiu da secao critica\n" ANSI_RESET, id);
    sem_post(&sem_escritores);

    pthread_exit(NULL);
}

void* thread_leitora(void* arg) {
    ThreadArg* dados = (ThreadArg*)arg;
    long id = dados->id;

    printf(ANSI_CYAN "[LEITOR %ld] Iniciando consulta...  \n" ANSI_RESET, id);

    usleep((rand() % 100) * 1000);

    printf(ANSI_CYAN "[LEITOR %ld] Lendo dados da conta... \n" ANSI_RESET, id);

    double saldo_lido;
    int ops_lidas;
    char status_lido[64];
    long escritor_ativo_id = -1;

    int leitura_suja = conta_consultar(g_conta, &saldo_lido, &ops_lidas, 
                                       status_lido, &escritor_ativo_id, g_delay_leitor_ms);

    if (leitura_suja) {
        pthread_mutex_lock(&mutex_estatisticas);
        g_total_leituras_sujas++;
        pthread_mutex_unlock(&mutex_estatisticas);

        printf(ANSI_BOLD ANSI_RED "[LEITOR %ld] Leitura suja detectada! Leu R$ %.2f (Escritor %ld ativo em '%s')\n" ANSI_RESET,
               id, saldo_lido, escritor_ativo_id, status_lido);
    } else {
        printf(ANSI_GREEN "[LEITOR %ld] Leitura limpa: Saldo = R$ %.2f | Ops = %d\n" ANSI_RESET,
               id, saldo_lido, ops_lidas);
    }

    printf(ANSI_CYAN "[LEITOR %ld] Consulta finalizada\n" ANSI_RESET, id);

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    srand(time(NULL));

    printf("=============================================================\n");
    printf(" Versao 1: Leitores e Escritores sem preferencia (Leitura Suja)\n");
    printf("===========================================================\n\n");

    if (argc >= 5) {
        g_num_leitores = atoi(argv[1]);
        g_num_escritores = atoi(argv[2]);
        g_delay_leitor_ms = atoi(argv[3]);
        g_delay_escritor_ms = atoi(argv[4]);
    } else {
        printf("Deseja usar a configuracao padrao de demonstracao? (S/n): ");
        char opcao = 's';
        if (scanf(" %c", &opcao) == 1 && (opcao == 'n' || opcao == 'N')) {
            printf("Quantidade de threads leitoras: ");
            if (scanf("%d", &g_num_leitores) != 1) g_num_leitores = 4;
            printf("Quantidade de threads escritoras: ");
            if (scanf("%d", &g_num_escritores) != 1) g_num_escritores = 3;
            printf("Tempo de processamento do escritor (ms, ex: 250): ");
            if (scanf("%d", &g_delay_escritor_ms) != 1) g_delay_escritor_ms = 250;
            printf("Tempo de leitura do leitor (ms, ex: 80): ");
            if (scanf("%d", &g_delay_leitor_ms) != 1) g_delay_leitor_ms = 80;
        }
    }

    // Inicializacao da conta
    g_conta = conta_cria(98765, "Conta UFAM - Leitores e Escritores", 1000.00);
    saldo_esperado_matematico = conta_obter_saldo(g_conta);

    sem_init(&sem_escritores, 0, 1);
    pthread_mutex_init(&mutex_estatisticas, NULL);

    printf("\n--- DADOS INICIAIS DA CONTA ---\n");
    printf("Conta: %d | Titular: %s | Saldo Inicial: R$ %.2f\n", conta_obter_numero(g_conta), conta_obter_titular(g_conta), conta_obter_saldo(g_conta));
    printf("Configuracao: %d Leitores | %d Escritores | Delay Leitor: %dms | Delay Escritor: %dms\n\n", g_num_leitores, g_num_escritores, g_delay_leitor_ms, g_delay_escritor_ms);

    printf("Operacoes programadas para os Escritores:\n");
    for (int i = 0; i < g_num_escritores; i++) {
        if (i % 2 == 0) {
            valores_escritores[i] = 150.00 * (i + 1);
        } else {
            valores_escritores[i] = -80.00 * (i + 1);
        }
        saldo_esperado_matematico += valores_escritores[i];
        printf("  Escritor %d: %s R$ %.2f\n", i,(valores_escritores[i] >= 0 ? "Deposito de" : "Saque de   "),(valores_escritores[i] >= 0 ? valores_escritores[i] : -valores_escritores[i]));
    }
    printf("Saldo esperado: R$ %.2f\n\n", saldo_esperado_matematico);
    printf("Iniciando threads...\n");
    printf("-----------------------------------------------------------------\n");

    pthread_t threads_l[g_num_leitores];
    pthread_t threads_e[g_num_escritores];
    ThreadArg args_l[g_num_leitores];
    ThreadArg args_e[g_num_escritores];

    int max_threads = (g_num_leitores > g_num_escritores) ? g_num_leitores : g_num_escritores;
    for (int i = 0; i < max_threads; i++) {
        if (i < g_num_escritores) {
            args_e[i].id = i;
            args_e[i].valor_operacao = valores_escritores[i];
            pthread_create(&threads_e[i], NULL, thread_escritora, &args_e[i]);
        }
        if (i < g_num_leitores) {
            args_l[i].id = i;
            args_l[i].valor_operacao = 0;
            pthread_create(&threads_l[i], NULL, thread_leitora, &args_l[i]);
        }
    }

    for (int i = 0; i < g_num_escritores; i++) {
        pthread_join(threads_e[i], NULL);
    }
    for (int i = 0; i < g_num_leitores; i++) {
        pthread_join(threads_l[i], NULL);
    }

    sem_destroy(&sem_escritores);
    pthread_mutex_destroy(&mutex_estatisticas);

    printf("-----------------------------------------------------------------\n");
    printf("\n=== Resultado final  ===\n");
    printf("Operacoes concluidas:    %d de %d\n",
           conta_obter_operacoes(g_conta), g_num_escritores);
    printf("Saldo esperado:          R$ %.2f\n", saldo_esperado_matematico);
    printf("Saldo final da conta:    R$ %.2f\n", conta_obter_saldo(g_conta));
    printf("Total de leituras sujas: %d\n\n", g_total_leituras_sujas);

    if (conta_obter_saldo(g_conta) == saldo_esperado_matematico) {
        printf(ANSI_GREEN "Status: Saldo correto (exclusao mutua entre escritores OK).\n" ANSI_RESET);
    } else {
        printf(ANSI_RED "Status: Inconsistencia detectada no saldo!\n" ANSI_RESET);
    }

    if (g_total_leituras_sujas > 0) {
        printf(ANSI_YELLOW "Nota: Ocorreram leituras sujas devido a concorrencia livre para leitores.\n" ANSI_RESET);
    }

    conta_libera(g_conta);

    printf("=================================================================\n");
    return 0;
}
