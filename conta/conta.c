#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "conta.h"


// cores para ajudar no destaque no terminal
#define ANSI_RESET "\x1b[0m"
#define ANSI_BOLD "\x1b[1m"
#define ANSI_MAGENTA "\x1b[35m"

// sctruct da conta
struct conta
{
    int numero;
    char titular[64];
    double saldo;
    int operacoes_realizadas;

    int em_transacao;
    long escritor_ativo_id;
    char status_transacao[64];
    double saldo_provisorio;

};

ContaBancaria* conta_cria(int numero, const char* titular, double saldo_inicial){
    ContaBancaria* c = (ContaBancaria*)malloc(sizeof(ContaBancaria));

    if(c == NULL){
        fprintf(stderr, "Falha na alocação de memoria para ContaBancaria\n");
        exit(EXIT_FAILURE);
    }
    c->numero = numero;
    strncpy(c->titular, titular, sizeof(c->titular)-1);
    c->titular[sizeof(c->titular) - 1] = '\0';
    c->saldo = saldo_inicial;
    c->operacoes_realizadas = 0;
    c->em_transacao = 0;
    c->escritor_ativo_id = -1;
    strncpy(c->status_transacao, "confirmado", sizeof(c->status_transacao));
    c->saldo_provisorio = saldo_inicial;

    return c;
}

void conta_libera(ContaBancaria *c){
    if(c != NULL){
        free(c);
    }
}

int conta_obter_numero(const ContaBancaria* c){
    return c->numero;
}

const char* conta_obter_titular(const ContaBancaria* c){
    return c->titular;
}

double conta_obter_saldo(const ContaBancaria* c){
    return c->saldo;
}

int conta_obter_operacoes(const ContaBancaria* c){
    return c->operacoes_realizadas;
}

int conta_consultar(const ContaBancaria* c, double* saldo_lido, int* ops_lidas, char* status_lido, long* escritor_id, int delay_ms){
    int transacao_em_curso = c->em_transacao;

    if(escritor_id){
        *escritor_id = c->escritor_ativo_id;
    }
    if(status_lido){
        strncpy(status_lido, c->status_transacao, 64);
    }
    if(ops_lidas){
        *ops_lidas = c->operacoes_realizadas;
    }
    if(saldo_lido){
        *saldo_lido = transacao_em_curso ? c->saldo_provisorio : c->saldo;
    }

    if(delay_ms > 0){
        usleep(delay_ms * 1000);
    }
    return transacao_em_curso;
}

double conta_atualizar(ContaBancaria* c, long escritor_id, double valor, int delay_ms){
    c->em_transacao = 1;
    c->escritor_ativo_id = escritor_id;
    c->saldo_provisorio = c->saldo + valor;
    snprintf(c->status_transacao, sizeof(c->status_transacao), "processando_%s", (valor >= 0 ? "deposito" : "saque"));

    printf(ANSI_MAGENTA "[CONTA] Escritor %ld iniciou %s de R$ %.2f (provisorio: R$ %.2f)\n" ANSI_RESET, escritor_id, (valor >= 0 ? "deposito" : "saque"), (valor >= 0 ? valor : -valor), c->saldo_provisorio);

    if(delay_ms > 0){
        usleep(delay_ms * 1000);
    }

    c->saldo = c->saldo_provisorio;
    c->operacoes_realizadas++;
    strncpy(c->status_transacao, "confirmado", sizeof(c->status_transacao));
    c->em_transacao = 0;
    c->escritor_ativo_id = -1;

    return c->saldo;
}

double conta_atualizar_desprotegido(ContaBancaria *c, long escritor_id, double valor, int delay_ms){
    double saldo_antigo = c->saldo;
    printf(ANSI_MAGENTA "[CONTA] Escritor %ld leu o saldo desprotegido = R$ %.2f\n" ANSI_RESET, escritor_id, saldo_antigo);

    if(delay_ms > 0){
        usleep(delay_ms * 1000);
    }

    c->saldo = saldo_antigo + valor; 
    c->operacoes_realizadas++;
    return c->saldo;
}