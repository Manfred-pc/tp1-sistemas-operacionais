#ifndef CONTA_H
#define CONTA_H

typedef struct conta ContaBancaria;

// funções
ContaBancaria* conta_cria(int numero, const char* titular, double saldo_inicial);

void conta_libera(ContaBancaria* c);

// retorna o numero da conta
int conta_obter_numero(const ContaBancaria* c);

// retorna o nome do sr titular da conta
const char* conta_obter_titular(const ContaBancaria*c );

// retorna o saldo consolidado atual da conta 
double conta_obter_saldo(const ContaBancaria* c);

// retornar a qtd total de operações concluidas na conta

int conta_consultar(const ContaBancaria*c, double* saldo_lido, int* ops_lidas, char* status_lido, long* escritar_id, int delays_ms);

double conta_atualizar_desprotegido(ContaBancaria*c, long escritor_id, double valor, int delay_ms);

#endif