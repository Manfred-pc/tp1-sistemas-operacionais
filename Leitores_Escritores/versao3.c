#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "../conta/conta.h"

// Cores ANSI para o terminal
#define ANSI_RESET "\x1b[0m"
#define ANSI_BOLD "\x1b[1m"
#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN "\x1b[36m"

#define MAX_THREADS 100 // LIMITE DO VETOR valores_escritores

// instancia da conta bancaria
ContaBancaria* g_conta = NULL;

// parametros
int g_num_leitores= 3;
int g_num_escritores = 4;
int g_delay_leitor_ms = 50;
int g_delay_escritor_ms = 150;

double valores_escritores[MAX_THREADS]; // operacao de cada escritor
double saldo_esperado_matematico = 0.0; // saldo que deveria sobrar

typedef struct 
{
  long id;
  double valor_operacao;
} ThreadArg;

// func executada pelas threads escritoras
void* thread_escritora(void*arg){
    ThreadArg* dados = (ThreadArg*) arg;
    long id = dados->id;
    double valor = dados ->valor_operacao;

    printf(ANSI_YELLOW "[ESCRITOR %ld] Thread criada. Pretende %s R$ %.2f\n" ANSI_RESET, id, (valor >= 0 ? "DEPOSITAR" : "SACAR"), (valor >= 0 ? valor : -valor));

    //chegada aleatoria curta menor que o delay do escritor. os escritores sobrepoem e o lost update
    usleep((rand() % 50) * 1000);

    // entrada na sessão crítica 
    printf(ANSI_BOLD ANSI_MAGENTA "[ESCRITOR %ld] ENTROU na Seção Crítica (SEM PROTEÇÃO)\n" ANSI_RESET, id);

    // le o saldo, espera o delay e grave . SEM lock no meio

    double saldo_escrito = conta_atualizar_desprotegido(g_conta, id, valor, g_delay_escritor_ms);
    printf(ANSI_BOLD ANSI_MAGENTA "[ESCRITOR %ld] Gravou saldo como R$ %.2f (Total Ops: %d)\n" ANSI_RESET, id, saldo_escrito, conta_obter_operacoes(g_conta));

    //saida
    printf(ANSI_MAGENTA "[ESCRITOR %ld] SAIU da Seção Crítica\n" ANSI_RESET, id);
    printf(ANSI_YELLOW "[ESCRITOR %ld] Thread finalizada.\n" ANSI_RESET, id);
    pthread_exit(NULL);

}
    //funcao a ser executada pelas threads leitoras 
void* thread_leitora(void*arg){
    ThreadArg* dados = (ThreadArg*) arg;
    long id = dados ->id; 

    printf(ANSI_CYAN "[LEITOR %ld] Thread criada. Consulta ao saldo bancário. \n"ANSI_RESET, id);
    usleep( ( rand() % 40) *1000);

    // entrada para a consulta 
    printf(ANSI_BOLD ANSI_CYAN "[LEITOR   %ld] ENTROU para consultar saldo\n" ANSI_RESET, id);
    double saldo_lido;
    int ops_lidas;
    char status_lido[64];
    long escritor_ativo_id = -1;

    //o leitor so le, o saldo que ele esta leendo poode ser um valor que vai ser sobresscrito
    conta_consultar(g_conta, &saldo_lido, &ops_lidas, status_lido, &escritor_ativo_id, g_delay_leitor_ms);
    printf(ANSI_CYAN "[LEITOR   %ld] CONSULTA: Conta %d (%s) | Saldo Lido = R$ %.2f | Ops = %d\n" ANSI_RESET,id, conta_obter_numero(g_conta), conta_obter_titular(g_conta), saldo_lido, ops_lidas);

    // saida 
    printf(ANSI_CYAN "[LEITOR   %ld] SAIU da consulta\n" ANSI_RESET, id);
    printf(ANSI_CYAN "[LEITOR   %ld] Thread finalizada.\n" ANSI_RESET, id);
    pthread_exit(NULL);
}

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
            if (scanf("%d", &g_num_leitores) != 1) g_num_leitores = 3;
            printf("Quantidade de threads escritoras: ");
             if (scanf("%d", &g_num_escritores) != 1) g_num_escritores = 4;
            printf("Tempo de processamento do escritor (ms, ex: 200): ");
             if (scanf("%d", &g_delay_escritor_ms) != 1) g_delay_escritor_ms = 200;
            printf("Tempo de leitura do leitor (ms, ex: 50): ");
            if (scanf("%d", &g_delay_leitor_ms) != 1) g_delay_leitor_ms = 50;
         }
     }
}

// confere se os parametros de entrada sao validos. > 100 e <0 quebra o programa por ora
static int parametros_validos(void) {
    return !(g_num_leitores < 1 || g_num_leitores > MAX_THREADS || g_num_escritores < 1 || g_num_escritores > MAX_THREADS || g_delay_leitor_ms < 0 || g_delay_escritor_ms < 0);
}

// define a operação de cada escritor e calcula o saldo que deveria sobrar no final 
static void preparar_escritores(void){
    printf("Operacoes programadas para os escritores: \n");
    for (int i = 0; i < g_num_escritores; i++){
        // escritores pares depositam, se forem impar sacam


        if(i%2 ==0){
            valores_escritores[i] = 100.00 * (i+1);
        } else{
            valores_escritores[i] = -30 * (i+1);
        }
        saldo_esperado_matematico += valores_escritores[i];
        printf("  Escritor %d: %s R$ %.2f\n", i,(valores_escritores[i] >= 0 ? "Deposito de" : "Saque de   "), (valores_escritores[i] >= 0 ? valores_escritores[i] : -valores_escritores[i]));
    }
    printf("Saldo final teorico esperado: R$ %.2f\n\n", saldo_esperado_matematico);
}

//Compara o saldo real com o esperado para mostrar se houve perda dps de att
static void mostrar_resultado(void) {
    double saldo_real = conta_obter_saldo(g_conta);
    double diferenca = saldo_real - saldo_esperado_matematico;
    printf("\nRESULTADO FINAL DA EXECUÇÃO (VERSÃO 3) \n");
    printf("Operacoes registradas: %d de %d esperadas.\n", conta_obter_operacoes(g_conta), g_num_escritores);
    printf("Saldo teorico esperado: R$ %.2f\n", saldo_esperado_matematico);
    printf("Saldo real obtido:      R$ %.2f\n", saldo_real);
    printf("Diferençaa (real - esperado): R$ %.2f\n", diferenca);
 
    // compara com tolerância porque double não é exato
    int saldo_errado = fabs(diferenca) >= 0.005;
 
    if (saldo_errado || conta_obter_operacoes(g_conta) != g_num_escritores) {
        printf(ANSI_BOLD ANSI_RED "\n[FALHA DE INTEGRIDADE CONFIRMADA]\n" ANSI_RESET);
        printf(ANSI_RED "Houve Condicao de Corrida e Perda de Atualizacao !\n"
               "Como as threads escritoras nao foram coordenadas com semaforos,\n"
               "uma thread sobrescreveu a operacao de outra em memoria compartilhada.\n" ANSI_RESET);
        printf(ANSI_RED "Observe que todas as operacoes foram executadas, mas o saldo nao reflete todas elas de forma exata\n" ANSI_RESET);
    } else {
        // a corrida vai depennder  do escalonamento, então às vezes não dá erro
        printf(ANSI_BOLD ANSI_YELLOW "\n[SEM ERRO NESTA EXECUCAO!!!!] Os escritores nao se sobrepuseram por sorte.\n" ANSI_RESET);
        printf(ANSI_YELLOW "A condicao de corrida continua existindo, rode de novo ou aumente o delay do escritor ou o numero de escritores.\n" ANSI_RESET);
    }
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    printf("=================================================================\n");
    printf(" VERSÃO 3: SEM CONTROLE DE CONCORRÊNCIA (CONDIÇÃO DE CORRIDAa)\n");
    printf("=================================================================\n\n");
 
    ler_parametros(argc, argv);
 
    if (!parametros_validos()) {
        fprintf(stderr, "Erro: leitores e escritores devem estar entre 1 e %d e os atrasos devem ser >= 0.\n", MAX_THREADS);
        return EXIT_FAILURE;
    }
 
    // Inicialização da Conta
    g_conta = conta_cria(12345, "Conta Compartilhada UFAM", 1000.00);
    saldo_esperado_matematico = conta_obter_saldo(g_conta);
 
    printf("--- DADOS INICIAIS DA CONTA---\n");
    printf("Conta: %d | Titular: %s | Saldo Inicial: R$ %.2f\n",
           conta_obter_numero(g_conta), conta_obter_titular(g_conta), conta_obter_saldo(g_conta));
    printf("Configuracao: %d Leitores | %d Escritores | Delay Leitor: %dms | Delay Escritor: %dms\n\n",
           g_num_leitores, g_num_escritores, g_delay_leitor_ms, g_delay_escritor_ms);
 
    preparar_escritores();
 
    printf("Iniciando execucao das threads concorrentes...\n");
    printf("-----------------------------------------------------------------\n");
 
    pthread_t threads_l[g_num_leitores];
    pthread_t threads_e[g_num_escritores];
    ThreadArg args_l[g_num_leitores];
    ThreadArg args_e[g_num_escritores];
 
    // cria escritor e leitor intercalados para misturar os dois tipos desde o vinício
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
    // espera todas terminarem antes de olhar o resultado final
    for (int i = 0; i < g_num_escritores; i++) {
        pthread_join(threads_e[i], NULL);
    }
    for (int i = 0; i < g_num_leitores; i++) {
        pthread_join(threads_l[i], NULL);
    }
    printf("-----------------------------------------------------------------\n");
    mostrar_resultado();
    conta_libera(g_conta);
    printf("=================================================================\n");
    return 0;
}