# Trabalho 1 — Sistemas Operacionais
 
Trabalho Prático 1 da disciplina de **Sistemas Operacionais**, professor João Marcos Bastos Cavalcanti. O trabalho tem o intuito de utilizar-mos programação concorrente com threads e semáforos, usando o tema de uma **conta bancária compartilhada**. Dessa forma, os escritores fazem depósitos e saques, enquanto os leitores consultam o saldo.
 
## Equipe

- Benjamim Isaac Ribeiro Lima 
- Manfred Lima Veiga

## Estrutura do projeto
 
```
.
├── Makefile
├── README.md
├── conta/
│   ├── conta.h                     # interface do TAD ContaBancaria
│   └── conta.c                     # implementação do TAD
└── Leitores_Escritores/
    ├── versao1_leitura_suja.c      # versão 1
    ├── versao2.c                   # versão 2
    └── versao3.c                   # versão 3
```
 
## Versões implementadas (Leitores × Escritores)
 
| Versão | Descrição |
|---|---|
| 1 | Leitores e escritores **sem preferência**. Escritores têm exclusão mútua entre si e os leitores não sincronizam, então pode ocorrer **leitura suja** durante a execuçãp. |
| 2 | **Escritores com preferência** sobre leitores, com 5 semáforos. Não vai ocorre leitura suja. |
| 3 | **Sem controle de concorrência**, para mostrar a condição de corrida e a perda de atualização. |
 
## Como compilar e executar
 
```bash
make          # compila o TAD e as versões 1, 2 e 3
make run1     # executa a versão 1
make run2     # executa a versão 2
make run3     # executa a versão 3
make run      # executa as três em sequência
make clean    # remove executáveis e arquivos .o
```
 
Também é possível executar direto, informando a quantidade de threads e os atrasos (em ms):
 
```bash
./versao1_leitura_suja <leitores> <escritores> <delay_leitor_ms> <delay_escritor_ms>
./versao2              <leitores> <escritores> <delay_leitor_ms> <delay_escritor_ms>
./versao3              <leitores> <escritores> <delay_leitor_ms> <delay_escritor_ms>
```
 
Sem argumentos, o programa pergunta se deve usar a configuração padrão ou pede os valores pelo teclado. Leitores e escritores devem estar entre 1 e 100, e os atrasos devem ser maiores ou iguais a 0.
 
> Curiosidade: a saída vai usar cores ANSI e atrasos aleatórios de chegada, então a ordem dos eventos muda a cada execução :0
 
## Produtores × Consumidores
 
pipipipopopo
 
## Fontes consultadas
 
Stallings, *Operating Systems: Internals and Design Principles*; 
Silberschatz et al., *Operating System Concepts*; Tanenbaum, *Modern Operating Systems*.
Foi também usado o Claude para revisão de código, correção de bugs, criação do Makefile e organizar este README.
