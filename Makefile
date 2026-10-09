CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2 -I$(CONTA_DIR)

SRC_DIR = Leitores_Escritores
CONTA_DIR = conta
PC_DIR = Produtores_Consumidores

TARGETS = le_versao1_leitura_suja le_versao2 le_versao3 pc_versao1 pc_versao2 pc_versao3
OBJS = conta.o

.PHONY: all clean run_le run_pc1 run_pc2 run_pc3 help

# Alvo padrão: compila todas as versões
all: $(TARGETS)
	@echo "Módulo TAD (conta.o) e Versões compilados com sucesso!"
	@echo "Execute:"
	@echo "  make run_le1 -> Versão 1 (Leitores/Escritores - leitura suja)"
	@echo "  make run_le2 -> Versão 2 (Leitores/Escritores - preferência)"
	@echo "  make run_le3 -> Versão 3 (Leitores/Escritores - sem controlo)"
	@echo "  make run_le  -> Executa as três versões LE em sequência"
	@echo "  make run_pc1 -> Versão 1 (Produtores/Consumidores)"
	@echo "  make run_pc2 -> Versão 2 (Produtores/Consumidores)"
	@echo "  make run_pc3 -> Versão 3 (Produtores/Consumidores)"

# Compilação do Módulo TAD (conta.o)
conta.o: $(CONTA_DIR)/conta.c $(CONTA_DIR)/conta.h
	$(CC) $(CFLAGS) -c $(CONTA_DIR)/conta.c -o $@

# Compilação e Ligação da Versão 1 (Leitores/Escritores) com o TAD
le_versao1_leitura_suja: $(SRC_DIR)/versao1_leitura_suja.c conta.o
	$(CC) $(CFLAGS) $(SRC_DIR)/versao1_leitura_suja.c conta.o -o $@

# Compilação e Ligação da Versão 2 (Leitores/Escritores) com o TAD
le_versao2: $(SRC_DIR)/versao2.c conta.o
	$(CC) $(CFLAGS) $(SRC_DIR)/versao2.c conta.o -o $@

# Compilação e Ligação da Versão 3 (Leitores/Escritores) com o TAD
le_versao3: $(SRC_DIR)/versao3.c conta.o
	$(CC) $(CFLAGS) $(SRC_DIR)/versao3.c conta.o -o $@

# Compilação das Versões de Produtores e Consumidores (Sem TAD conta.o)
pc_versao1: $(PC_DIR)/versao1.c
	$(CC) $(CFLAGS) $(PC_DIR)/versao1.c -o $@

pc_versao2: $(PC_DIR)/versao2.c
	$(CC) $(CFLAGS) $(PC_DIR)/versao2.c -o $@

pc_versao3: $(PC_DIR)/versao3.c
	$(CC) $(CFLAGS) $(PC_DIR)/versao3.c -o $@

# Executa a Versão 1 LE (4 leitores, 3 escritores, delays 60ms e 200ms)
run_le1: le_versao1_leitura_suja
	./le_versao1_leitura_suja 4 3 60 200

# Executa a Versão 2 LE (5 leitores, 3 escritores, delays 80ms e 250ms)
run_le2: le_versao2
	./le_versao2 5 3 80 250

# Executa a Versão 3 LE (3 leitores, 4 escritores, delays 50ms e 150ms)
run_le3: le_versao3
	./le_versao3 3 4 50 150

# Executa as três versões LE em sequência
run_le: run_le1 run_le2 run_le3

# Executa as versões de Produtores e Consumidores
run_pc1: pc_versao1
	./pc_versao1

run_pc2: pc_versao2
	./pc_versao2

run_pc3: pc_versao3
	./pc_versao3

# Limpeza dos binários e ficheiros objeto
clean:
	rm -f $(TARGETS) $(OBJS)
	@echo "Executáveis e ficheiros .o removidos com sucesso."

# Ajuda com os comandos disponíveis
help:
	@echo "Comandos disponíveis no Makefile:"
	@echo "  make - Compila tudo"
	@echo "  make run_le1 - Executa a Versão 1 de Leitores/Escritores"
	@echo "  make run_le2 - Executa a Versão 2 de Leitores/Escritores"
	@echo "  make run_le3 - Executa a Versão 3 de Leitores/Escritores"
	@echo "  make run_le - Executa as três versões de LE em sequência"
	@echo "  make run_pc1 - Executa a Versão 1 de Produtores/Consumidores"
	@echo "  make run_pc2 - Executa a Versão 2 de Produtores/Consumidores"
	@echo "  make run_pc3 - Executa a Versão 3 de Produtores/Consumidores"
	@echo "  make clean - Remove os executáveis e ficheiros temporários"