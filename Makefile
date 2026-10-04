CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2 -I$(CONTA_DIR)

SRC_DIR = Leitores_Escritores
CONTA_DIR = conta

TARGETS = versao1_leitura_suja versao2 versao3
OBJS = conta.o

.PHONY: all clean run run1 run2 run3 help

# Alvo padrão: compila  as três versões
all: $(TARGETS)
	@echo "=========================================================="
	@echo "Módulo TAD (conta.o) e Versões 1, 2 e 3 compilados com sucesso!"
	@echo "Execute:"
	@echo "  make run1  -> Versão 1 (sem preferência, leitura suja)"
	@echo "  make run2  -> Versão 2 (escritores com preferência)"
	@echo "  make run3  -> Versão 3 (sem controle de concorrência)"
	@echo "  make run   -> Executa as três em sequência"
	@echo "=========================================================="

# Compilação do Módulo TAD (conta.o)
conta.o: $(CONTA_DIR)/conta.c $(CONTA_DIR)/conta.h
	$(CC) $(CFLAGS) -c $(CONTA_DIR)/conta.c -o $@

# Compilação e Ligação da Versão 1 com o TAD
versao1_leitura_suja: $(SRC_DIR)/versao1_leitura_suja.c conta.o
	$(CC) $(CFLAGS) $(SRC_DIR)/versao1_leitura_suja.c conta.o -o $@

# Compilação e Ligação da Versão 2 com o TAD
versao2: $(SRC_DIR)/versao2.c conta.o
	$(CC) $(CFLAGS) $(SRC_DIR)/versao2.c conta.o -o $@

# Compilação e Ligação da Versão 3 com o TAD
versao3: $(SRC_DIR)/versao3.c conta.o
	$(CC) $(CFLAGS) $(SRC_DIR)/versao3.c conta.o -o $@

# Executa a Versão 1 (4 leitores, 3 escritores, delays 60ms e 200ms)
run1: versao1_leitura_suja
	./versao1_leitura_suja 4 3 60 200

# Executa a Versão 2 (5 leitores, 3 escritores, delays 80ms e 250ms)
run2: versao2
	./versao2 5 3 80 250

# Executa a Versão 3 (3 leitores, 4 escritores, delays 50ms e 150ms)
run3: versao3
	./versao3 3 4 50 150

# Executa as três versões em sequência
run: run1 run2 run3

# Limpeza dos binários e arquivos objeto
clean:
	rm -f $(TARGETS) $(OBJS)
	@echo "Executáveis e arquivos .o removidos com sucesso."

# Ajuda com os comandos disponíveis
help:
	@echo "Comandos disponíveis no Makefile:"
	@echo "  make       - Compila o TAD conta.o e as Versões 1, 2 e 3"
	@echo "  make run1  - Executa a Versão 1"
	@echo "  make run2  - Executa a Versão 2"
	@echo "  make run3  - Executa a Versão 3"
	@echo "  make run   - Executa as três versões em sequência"
	@echo "  make clean - Remove os executáveis e arquivos temporários (.o)"