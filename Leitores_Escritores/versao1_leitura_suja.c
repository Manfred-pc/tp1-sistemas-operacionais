/*
 Versao 1: Leitores e Escritores sem a preferencia de acesso (leitura suja eca).
 
  Tema: conta bancaria compartilhada (em conta/conta.c).
  Escritores (deposito/saque): exclusao mutua entre eles msm com umm semaforo binario.
    Leitores: nao vao sincronizar com ninguem. Por isso podem ler o saldo provisorio de uma transacao ainda nao confirmada (leitura suja).
 
  Uso: ./versao1_leitura_suja <leitores> <escritores> <delay_leitor_ms> <delay_escritor_ms>
 */

#include <stdio.h> 
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "../conta/conta.h"

// Cores ANSI para o terminal
#define ANSI_RESET   "\x1b[0m"
#define ANSI_BOLD    "\x1b[1m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"

#define MAX_THREADS 100   // limite do vetor valores_escritores

// Instancia compartilhada da conta bancaria
ContaBancaria* g_conta = NULL;

// Semaforo binario (valor inicial 1): so um escritor altera a conta por vez.
// Os leitores nao usam este semaforo de proposito: e isso que permite a leitura suja
sem_t sem_escritores;


// Varios leitores podem detectar leitura suja ao mesmo tempo, por isso o contador tem mutex proprio (independente do acesso a conta).

// Estatisticas
int g_total_leituras_sujas = 0;
pthread_mutex_t mutex_estatisticas;

// Parametros padrao, mas podem ser modificaddos por argumento ou entrada no teclado
int g_num_leitores = 4;
int g_num_escritores = 3;
int g_delay_leitor_ms = 80;
int g_delay_escritor_ms = 250;

// valores_escritores[i]: operacao do escritor i ( >  0 deposito, <  0 saque).
double valores_escritores[MAX_THREADS];

//  saldo_esperado_matematico: saldo inicial + soma das operacoes. conferido no final para provar que nenhuma atualizacao foi de vala (perdida)
double saldo_esperado_matematico = 0.0;

typedef struct {
    long id;
    double valor_operacao;
} ThreadArg;


// ESPERA UM semaforo, se elle estiver fechado, imprime que a thread foi bloqueada e dps dorme no sem_wait ate outra thread dar sem_post
static void esperar_avisando(sem_t* sem, const char* tipo, long id, const char* motivo) {
    if (sem_trywait(sem) == 0) {
        return;   // estava livre, nao precisou esperar
    }
    printf(ANSI_BOLD ANSI_RED "[%s %ld] BLOQUEADO: %s\n" ANSI_RESET, tipo, id, motivo);
    sem_wait(sem);
    printf(ANSI_GREEN "[%s %ld] DESBLOQUEADO, pode continuar\n" ANSI_RESET, tipo, id);
}


void* thread_escritora(void* arg) {
    ThreadArg* dados = (ThreadArg*)arg;
    long id = dados->id;
    double valor = dados->valor_operacao;

    printf(ANSI_YELLOW "[ESCRITOR %ld] Criado: %s R$ %.2f\n" ANSI_RESET, id, (valor >= 0 ? "deposito de" : "saque de"), (valor >= 0 ? valor : -valor));
// Atraso aleatorio de chegada. varia a ordem em que as threads disputam a conta
    usleep((rand() % 300) * 1000);

// Se outro escritor estiver na secao critica, esta thread barra e bloqueia aqui ate o outro executar sem_post
    esperar_avisando(&sem_escritores, "ESCRITOR", id, "outro escritor esta usando a conta");

    printf(ANSI_BOLD ANSI_MAGENTA "[ESCRITOR %ld] Entrou na secao critica\n" ANSI_RESET, id);


    // conta_atualizar marca a transacao como "em andamento" e so confirma o saldo e apos o delay. Durante essa janela, um leitor pode enxergar o valor provisorio
    double novo_saldo = conta_atualizar(g_conta, id, valor, g_delay_escritor_ms);

    printf(ANSI_BOLD ANSI_MAGENTA "[ESCRITOR %ld] Operacao concluida. Novo saldo = R$ %.2f (total ops: %d)\n" ANSI_RESET, id, novo_saldo, conta_obter_operacoes(g_conta));

    printf(ANSI_MAGENTA "[ESCRITOR %ld] Saiu da secao critica\n" ANSI_RESET, id);
    sem_post(&sem_escritores);

    printf(ANSI_YELLOW "[ESCRITOR %ld] Thread finalizada.\n" ANSI_RESET, id);
    pthread_exit(NULL);
}

void* thread_leitora(void* arg) {
    ThreadArg* dados = (ThreadArg*)arg;
    long id = dados->id;

    printf(ANSI_CYAN "[LEITOR %ld] Iniciando consulta...  \n" ANSI_RESET, id);

    usleep((rand() % 300) * 1000);

    // o leitor nao usa semaforo: le a conta mesmo com escritor ativo
    printf(ANSI_CYAN "[LEITOR %ld] Lendo dados da conta... \n" ANSI_RESET, id);

    double saldo_lido;
    int ops_lidas;
    char status_lido[64];
    long escritor_ativo_id = -1;

    // vai retornar 1 se existiu escritor com transacao em andamento na hora da leitura
    int leitura_suja = conta_consultar(g_conta, &saldo_lido, &ops_lidas, status_lido, &escritor_ativo_id, g_delay_leitor_ms);

    if (leitura_suja) {
        pthread_mutex_lock(&mutex_estatisticas);
        g_total_leituras_sujas++;
        pthread_mutex_unlock(&mutex_estatisticas);

        printf(ANSI_BOLD ANSI_RED "[LEITOR %ld] Leitura suja detectada! Leu R$ %.2f (Escritor %ld ativo em '%s')\n" ANSI_RESET, id, saldo_lido, escritor_ativo_id, status_lido);
    } else {
        printf(ANSI_GREEN "[LEITOR %ld] Leitura limpa: Saldo = R$ %.2f | Ops = %d\n" ANSI_RESET, id, saldo_lido, ops_lidas);
    }

    printf(ANSI_CYAN "[LEITOR %ld] Consulta finalizada\n" ANSI_RESET, id);
    printf(ANSI_CYAN "[LEITOR %ld] Thread finalizada.\n" ANSI_RESET, id);
    pthread_exit(NULL);
}

// Le os parametros de argumentos ou entrada de teclado
static void ler_parametros(int argc, char* argv[]) {
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
}

// confere se os valores digitados sao validos. se for > 100 o programa morre
static int parametros_validos(void) {
    return !(g_num_leitores < 1 || g_num_leitores > MAX_THREADS || g_num_escritores < 1 || g_num_escritores > MAX_THREADS || g_delay_leitor_ms < 0 || g_delay_escritor_ms < 0);
}


// Define a operacao de cada escritor e calcula o saldo que DEVERIAS sobrar no final
static void preparar_escritores(void) {
    printf("Operacoes programadas para os Escritores:\n");
    for (int i = 0; i < g_num_escritores; i++) {
        // escritores pares depositam, impares sacam
  if (i % 2 == 0) {
            valores_escritores[i] = 150.00 * (i + 1);
        } else {
           valores_escritores[i] = -80.00 * (i + 1);
        }
        saldo_esperado_matematico += valores_escritores[i];
     printf("  Escritor %d: %s R$ %.2f\n", i,(valores_escritores[i] >= 0 ? "Deposito de" : "Saque de   "),(valores_escritores[i] >= 0 ? valores_escritores[i] : -valores_escritores[i]));
    }
    printf("Saldo esperado: R$ %.2f\n\n", saldo_esperado_matematico);
}

//MOStra o resultado final e faz um check-list se o saldo bateu
static void mostrar_resultado(void) {
    double saldo_real = conta_obter_saldo(g_conta);
 
    printf("\nRESULTADO FINAL DA EXECUÇÃO (VERSÃO 1)\n");
    printf("Operacoes concluidas:    %d de %d\n",conta_obter_operacoes(g_conta), g_num_escritores);
    printf("Saldo esperado:          R$ %.2f\n", saldo_esperado_matematico);
    printf("Saldo final da conta:    R$ %.2f\n", saldo_real);
    printf("Total de leituras sujas: %d\n\n", g_total_leituras_sujas);
 
    // compara com tolerancia porque double nao e exato (nao usar ==)
    if (fabs(saldo_real - saldo_esperado_matematico) < 0.005) {
        printf(ANSI_GREEN "Status: Saldo correto (exclusao mutua entre escritores OK).\n" ANSI_RESET);
    } else {
        printf(ANSI_RED "Status: Inconsistencia detectada no saldo!!!!!!1!\n" ANSI_RESET);
    }
 
    if (g_total_leituras_sujas > 0) {
        printf(ANSI_YELLOW "Nota: Ocorreram leituras sujas devido a concorrencia livre para leitores.\n" ANSI_RESET);
    } else {
        // a leitura suja depende do escalonamento, entao as vezes nao aparece
        printf(ANSI_YELLOW "Nota: Nenhuma leitura suja nesta execucao (depende da sortee); rode de novo ou aumente os delays.\n" ANSI_RESET);
    }
}

int main(int argc, char* argv[]) {
    srand(time(NULL));

    printf("=========================================================\n");
    printf(" Versao 1: Leitores e Escritores sem preferencia (Leitura Suja)\n");
    printf("=========================================================\n\n");

    ler_parametros(argc, argv);
    if (!parametros_validos()) {
        fprintf(stderr, "Erro: leitores e escritores devem estar entre 1 e %d e os atrasos devem ser >= 0.\n", MAX_THREADS);
        return EXIT_FAILURE;
    }


    // Inicializacao da conta
    g_conta = conta_cria(98765, "Conta UFAM - Leitores e Escritores", 1000.00);
    saldo_esperado_matematico = conta_obter_saldo(g_conta);

    sem_init(&sem_escritores, 0, 1);
    pthread_mutex_init(&mutex_estatisticas, NULL);

    printf("\n--- DADOS INICIAIS DA CONTA ---\n");
    printf("Conta: %d | Titular: %s | Saldo Inicial: R$ %.2f\n", conta_obter_numero(g_conta), conta_obter_titular(g_conta), conta_obter_saldo(g_conta));
    printf("Configuracao: %d Leitores | %d Escritores | Delay Leitor: %dms | Delay Escritor: %dms\n\n", g_num_leitores, g_num_escritores, g_delay_leitor_ms, g_delay_escritor_ms);

    preparar_escritores();
    printf("Iniciando threads...\n");
    printf("-----------------------------------------------------------------\n");

    pthread_t threads_l[g_num_leitores];
    pthread_t threads_e[g_num_escritores];
    ThreadArg args_l[g_num_leitores];
    ThreadArg args_e[g_num_escritores];

    // cria escritor e leitor intercalados para misturar os dois tipos desde o principio
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

    // Aguarda todas as threads terminarem antes de ler o resultado final
    for (int i = 0; i < g_num_escritores; i++) {
        pthread_join(threads_e[i], NULL);
    }
    for (int i = 0; i < g_num_leitores; i++) {
        pthread_join(threads_l[i], NULL);
    }

    sem_destroy(&sem_escritores);
    pthread_mutex_destroy(&mutex_estatisticas);

    printf("-----------------------------------------------------------------\n");
    mostrar_resultado();
    conta_libera(g_conta);
    printf("=================================================================\n");
    return 0;
}