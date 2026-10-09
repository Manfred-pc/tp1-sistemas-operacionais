#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "../conta/conta.h"

// Cores para o terminal
#define ANSI_RESET   "\x1b[0m"
#define ANSI_BOLD    "\x1b[1m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"

//limite do vetor valores_escritores
#define MAX_THREADS 100 

// conta bancaria 
ContaBancaria *g_conta = NULL;

// cont de estados
int g_num_leitores_ativos = 0;

// escritores esperando ouu escrevendo 
int g_num_escritores_fila = 0;

// semaforos para a organização
sem_t sem_mutex_leitor; // protege fofo g_num_leitores_ativos
sem_t sem_mutex_escritor; // protege g_num_escritores_fila
sem_t sem_bloqueio_leitores; //fecha o primeiro escritor e barra novos leitores
sem_t sem_acesso_dados; // exclusao mutua na area de dados
sem_t sem_ordem_leitor; //so um eleitor por vez pode passar pelo bloqueio (parece um pedagio)

// estatisticas
int g_total_leituras_sujas = 0;
int g_max_leitores_simultaneos = 0;
pthread_mutex_t mutex_estatisticas;

// parâmetros  
int g_num_leitores = 5;
int g_num_escritores = 3;
int g_delay_leitor_ms= 80;
int g_delay_escritor_ms = 250;

//operaçao que cada escritor realizara (add money ou sacar)
double valores_escritores[MAX_THREADS];

// saldo inicial + (operacoes realizadas)
double saldo_esperado_matematico = 0.0;

typedef struct {

    long id;
    double valor_operacao;
} ThreadArg;

// espera um semaforo; se ele estiver ja fechado, imprimire que a thread foi bloqueada e dps ele dorme no sem_wait ate outra thread usar sem_post
static void esperar_avisando(sem_t* sem, const char* tipo, long id, const char* motivo){

    if(sem_trywait(sem) == 0){

        return;
    }
    printf(ANSI_BOLD ANSI_RED "[%s %ld] BLOQUEADO: %s\n" ANSI_RESET, tipo, id, motivo);
    sem_wait(sem);
    printf(ANSI_GREEN "[%s %ld] DESBLOQUEADO, pode continuar\n" ANSI_RESET, tipo, id);

}

// funcao fofa para ser executada em threads escritoras
void * thread_escritora( void* arg){

    ThreadArg* dados = (ThreadArg*) arg;
    long id = dados -> id;
    double valor = dados -> valor_operacao;

    printf(ANSI_YELLOW "[ESCRITOR %ld] Thread criada. Operacao: %s R$ %.2f\n" ANSI_RESET, id, (valor >= 0 ? "DEPOSITO" : "SAQUE"), (valor >= 0 ? valor : -valor));

    // atraso aleatorio para os escritores nao chegarem na msm ordem 
    usleep((rand() % 40) *1000);

    // registro do escritor, dessa forma, ativnado a prioridade sobre os leitorees
    sem_wait(&sem_mutex_escritor);
    g_num_escritores_fila++;

    // apenas o primeiro escritor fecha a porta dos leitores; os outros ja vao encontrar fechadas  

    if(g_num_escritores_fila == 1){

        printf(ANSI_BOLD ANSI_YELLOW "[ESCRITOR %ld] [PRIORIDADE ATIVADA] Primeiro escritor na fila! Bloqueando novos leitores.\n" ANSI_RESET, id);
        sem_wait(&sem_bloqueio_leitores);
    }
    sem_post(&sem_mutex_escritor);

    //agora, é voltado para esperar os leitores que ja estavam lendo terminarem, ou algum escritor anterior 

    esperar_avisando(&sem_acesso_dados, "ESCRITOR", id, "leitores lendo ou outro escritor usando os dados ");

    /// sessao critica de escrita
     printf(ANSI_BOLD ANSI_MAGENTA "[ESCRITOR %ld] ENTROU na Seção Crítica de Dados (Acesso Exclusivo Garantido)\n" ANSI_RESET, id);

    // atualiza a conta atraves da func do tad
    double novo_saldo = conta_atualizar(g_conta, id, valor, g_delay_escritor_ms);
    printf(ANSI_BOLD ANSI_MAGENTA "[ESCRITOR %ld] Saldo atualizado com SUCESSO para R$ %.2f (Total Ops: %d)\n" ANSI_RESET, id, novo_saldo, conta_obter_operacoes(g_conta));

    // liberando os dados
    printf(ANSI_MAGENTA "[ESCRITOR %ld] SAIU da Seção Crítica de Dados\n" ANSI_RESET, id);
    sem_post(&sem_acesso_dados);

    // desregistro do escritor
    sem_wait(&sem_mutex_escritor);
    g_num_escritores_fila--;

    //obs: so o ultimo escritor reabre a porta, enquanto o tiver escritor na fila o leitor n vai entrar
    if (g_num_escritores_fila == 0) {
    printf(ANSI_BOLD ANSI_YELLOW "[ESCRITOR %ld] Ultimo escritor concluido. Liberando entrada para leitores aguardando!\n" ANSI_RESET, id);

    sem_post(&sem_bloqueio_leitores);
    }
    sem_post(&sem_mutex_escritor);
    printf(ANSI_YELLOW "[ESCRITOR %ld] Thread finalizada.\n" ANSI_RESET, id);
    pthread_exit(NULL);

}

    // func que vai ser executada pelas threads leitoras 

   
 void* thread_leitora(void*arg){
        ThreadArg* dados= (ThreadArg*) arg;
        long id = dados-> id;
        printf(ANSI_CYAN "[LEITOR   %ld] Thread criada. Deseja consultar saldo bancario.\n" ANSI_RESET, id);

        // atraso aleatorio para simular a chegada
        usleep((rand() %80 ) *1000);

        // regras para a entrada do leitor
        sem_wait(&sem_ordem_leitor); // so um leitor por vez tenta passar pelo bloqueio dos escritores
        
        // se tiver um sr. escritor na fila, fecha este semaforo e o leitor fica em stand by
        esperar_avisando(&sem_bloqueio_leitores, "LEITOR ", id, "tem escritor na fila ou escrevendo (os escritores têm preferencia)");
        sem_wait(&sem_mutex_leitor);
        g_num_leitores_ativos++;

        // este trecho ja esta protegido por sem_mutex_leitor, ent n precisa de outro lock
        if(g_num_leitores_ativos > g_max_leitores_simultaneos){

            g_max_leitores_simultaneos = g_num_leitores_ativos;
        }

        // so o primeiro leitor trava a area de dados contra os escritore, os outros vao entrar junto com ele
        if(g_num_leitores_ativos == 1){

            printf(ANSI_CYAN "[LEITOR   %ld] Primeiro leitor ativo. Bloqueando dados para gravacao.\n" ANSI_RESET, id);
            sem_wait(&sem_acesso_dados);
        }
        int ativos_agora = g_num_leitores_ativos; // guarda o valor no lock
        sem_post(&sem_mutex_leitor);
        sem_post(&sem_bloqueio_leitores);
        sem_post(&sem_ordem_leitor);

        // sessao critica da leitura, quando mts leitores podem brigar pela existencia
        printf(ANSI_BOLD ANSI_CYAN "[LEITOR   %ld] Entrou na Seção Crítica de Leitura (Leitores ativos no momento: %d)\n" ANSI_RESET, id, ativos_agora);
        
        double saldo_lido;
        int ops_lidas;
        char status_lido[64];
        long escritor_ativo_id = -1;

        // consulta oriunda atraves do tad, o DELAY Do leitor acontece nessa funcao do tad
        int leitura_suja = conta_consultar(g_conta, &saldo_lido, &ops_lidas, status_lido, &escritor_ativo_id, g_delay_leitor_ms);
 

        // nessa versao, n deve aparecer esse bloco da leitura suja. mas aq está o tratamento
        if (leitura_suja){
            pthread_mutex_lock(&mutex_estatisticas);
            g_total_leituras_sujas++;
            pthread_mutex_unlock(&mutex_estatisticas);
            printf(ANSI_BOLD ANSI_RED "[LEITOR   %ld] ERRO CRÍTICO: Leitor leu durante escrita do Escritor %ld!\n" ANSI_RESET,id, escritor_ativo_id);
        }

        printf(ANSI_GREEN "[LEITOR   %ld] CONSULTA %s : Saldo Lido = R$ %.2f | Total Ops = %d | Status = [%s]\n" ANSI_RESET, id, (leitura_suja ? "SUJA" : "SEGURA"), saldo_lido, ops_lidas, status_lido);

        // protocolo de saida do leitor
        sem_wait(&sem_mutex_leitor);
        g_num_leitores_ativos--;
        printf(ANSI_CYAN "[LEITOR   %ld] SAIU da Seção Crítica de Leitura (Restam %d leitores ativos)\n" ANSI_RESET, id, g_num_leitores_ativos);

        // só o ultimo leitor a sair libera os dados para os escritores!!!!!!!!
        if (g_num_leitores_ativos == 0) {
            printf(ANSI_CYAN "[LEITOR   %ld] Ultimo leitor saindo. Liberando acesso aos dados para escritores!\n" ANSI_RESET, id);
            sem_post(&sem_acesso_dados);
        }
        sem_post(&sem_mutex_leitor);
        printf(ANSI_CYAN "[LEITOR   %ld] Thread finalizada.\n" ANSI_RESET, id);
        pthread_exit(NULL);
}

// Lê os parâmetros
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
        if (scanf("%d", &g_num_leitores) != 1) g_num_leitores = 5;
        printf("Quantidade de threads escritoras: ");
        if (scanf("%d", &g_num_escritores) != 1) g_num_escritores = 3;
        printf("Tempo de processamento do escritor (ms, ex: 250): ");
        if (scanf("%d", &g_delay_escritor_ms) != 1) g_delay_escritor_ms = 250;
        printf("Tempo de leitura do leitor (ms, ex: 80): ");
        if (scanf("%d", &g_delay_leitor_ms) != 1) g_delay_leitor_ms = 80;
        }
    }
}
 
// aq é pra conferir se os valores digitados fazem sentido antes de criar as threads. EI mas de 100 estoura o vetor valores_escritores e negativo quebra QUBRA o programa
static int parametros_validos(void) {
    return !(g_num_leitores < 1 || g_num_leitores > MAX_THREADS || g_num_escritores < 1 || g_num_escritores > MAX_THREADS || g_delay_leitor_ms < 0 || g_delay_escritor_ms < 0);
}
 
//Define a operação de cada escritor e calcula o saldo que deveria sobrar no final
static void preparar_escritores(void) {
    printf("Operacoes programadas para os Escritores:\n");
    for (int i = 0; i < g_num_escritores; i++) {
        // escritores pares depositam, ímpares sacam eba
        if (i % 2 == 0) {
            valores_escritores[i] = 200.00 * (i + 1);
        } else {
            valores_escritores[i] = -100.00 * (i + 1);
        }
        saldo_esperado_matematico += valores_escritores[i];
        printf("  Escritor %d: %s R$ %.2f\n", i,
               (valores_escritores[i] >= 0 ? "Deposito de" : "Saque de   "),
               (valores_escritores[i] >= 0 ? valores_escritores[i] : -valores_escritores[i]));
    }
    printf("Saldo final teorico esperado matematicamente: R$ %.2f\n\n", saldo_esperado_matematico);
}
 
//Mostra o resultado final e confere se está tudo crto
static void mostrar_resultado(void) {
    double saldo_real = conta_obter_saldo(g_conta);
 
    printf("\n=== RESULTADO FINAL DA EXECUÇÃO (VERSÃO 2 - COM TAD) ===\n");
    printf("Operacoes realizadas: %d de %d programadas.\n", conta_obter_operacoes(g_conta), g_num_escritores);
    printf("Saldo teorico esperado: R$ %.2f\n", saldo_esperado_matematico);
    printf("Saldo real final:       R$ %.2f\n", saldo_real);
    printf("Pico de leitores simultaneos na area de dados: %d\n", g_max_leitores_simultaneos);
    printf("Total de Leituras Sujas detectadas: %d\n", g_total_leituras_sujas);
 
    // compara com tolerância porque double não é exato (não usar == parece)
    int saldo_ok = fabs(saldo_real - saldo_esperado_matematico) < 0.005;
 
    if (saldo_ok && g_total_leituras_sujas == 0) {
        printf(ANSI_BOLD ANSI_GREEN "\n[SUCESSO - CONSISTÊNCIA PERFEITA]\n" ANSI_RESET);
        printf(ANSI_GREEN "- Zero Leitura Suja: Nenhum leitor teve acesso enquanto escritores estavam ativos.\n"
               "- Prioridade para Escritores: Novos leitores foram bloqueados assim que escritores solicitaram acesso.\n"
               "- Paralelismo de Leitores: Multiplos leitores puderam ler simultaneamente pelo TAD quando nao havia escritores.\n"
               "- Integridade Bancaria: O saldo final bateu 100%% com o valor teorico esperado.\n" ANSI_RESET);
    } else {
        printf(ANSI_BOLD ANSI_RED "\n[FALHA] Saldo diferente do esperado ou leitura suja detectada!!!!!!!\n" ANSI_RESET);
    }
}
 
int main(int argc, char* argv[]) {
    srand(time(NULL));
 
    printf("=================================================================\n");
    printf(" VERSÃO 2: ESCRITORES COM PREFERÊNCIA SOBRE LEITORES\n");
    printf("=================================================================\n\n");
 
    ler_parametros(argc, argv);
 
    if (!parametros_validos()) {
        fprintf(stderr, "Erro: leitores e escritores devem estar entre 1 e %d e os atrasos devem ser >= 0\n", MAX_THREADS);
        return EXIT_FAILURE;
    }
 
    // Inicialização da Conta 
    g_conta = conta_cria(54321, "Conta UFAM - Prioridade Escritores", 1000.00);
    saldo_esperado_matematico = conta_obter_saldo(g_conta);
 
    // todos começam com 1 ABERTOSSSSSSS, pois funcionam como portas 
    sem_init(&sem_mutex_leitor, 0, 1);
    sem_init(&sem_mutex_escritor, 0, 1);
    sem_init(&sem_bloqueio_leitores, 0, 1);
    sem_init(&sem_acesso_dados, 0, 1);
    sem_init(&sem_ordem_leitor, 0, 1);
    pthread_mutex_init(&mutex_estatisticas, NULL);
 
    printf("--- DADOS INICIAIS DA CONTA ---\n");
    printf("Conta: %d | Titular: %s | Saldo Inicial: R$ %.2f\n",
           conta_obter_numero(g_conta), conta_obter_titular(g_conta), conta_obter_saldo(g_conta));
    printf("Configuracao: %d Leitores | %d Escritores | Delay Leitor: %dms | Delay Escritor: %dms\n\n",
           g_num_leitores, g_num_escritores, g_delay_leitor_ms, g_delay_escritor_ms);
 
    preparar_escritores();
 
    printf("Iniciando execucao concorrente com controle estrito...\n");
    printf("-----------------------------------------------------------------\n");
 
    pthread_t threads_l[g_num_leitores];
    pthread_t threads_e[g_num_escritores];
    ThreadArg args_l[g_num_leitores];
    ThreadArg args_e[g_num_escritores];
 
    // cria leitor e escritor intercalados para misturar os dois tipos desde o início
    int max_threads = (g_num_leitores > g_num_escritores) ? g_num_leitores : g_num_escritores;
    for (int i = 0; i < max_threads; i++) {
        if (i < g_num_leitores) {
            args_l[i].id = i;
            args_l[i].valor_operacao = 0;
            pthread_create(&threads_l[i], NULL, thread_leitora, &args_l[i]);
        }
        if (i < g_num_escritores) {
            args_e[i].id = i;
            args_e[i].valor_operacao = valores_escritores[i];
            pthread_create(&threads_e[i], NULL, thread_escritora, &args_e[i]);
        }
    }
 
    // espera todas terminarem antes de olhar o resultado final
    for (int i = 0; i < g_num_escritores; i++) {
        pthread_join(threads_e[i], NULL);
    }
    for (int i = 0; i < g_num_leitores; i++) {
        pthread_join(threads_l[i], NULL);
    }
 
    sem_destroy(&sem_mutex_leitor);
    sem_destroy(&sem_mutex_escritor);
    sem_destroy(&sem_bloqueio_leitores);
    sem_destroy(&sem_acesso_dados);
    sem_destroy(&sem_ordem_leitor);
    pthread_mutex_destroy(&mutex_estatisticas);
 
    printf("-----------------------------------------------------------------\n");
    mostrar_resultado();
 
    conta_libera(g_conta);
 
    printf("=================================================================\n");
    return 0;
}

