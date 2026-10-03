CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2 -I$(CONTA_DIR)

SRC_DIR = Leitores_Escritores
CONTA_DIR = conta

TARGETS = versao1_leitura_suja
OBJS = conta.o

.PHONY: all clean run help

# Alvo padrão: compila o TAD e a Versão 1
all: $(TARGETS)
	@echo "=========================================================="
	@echo "Módulo TAD (conta.o) e Versão 1 compilados com sucesso!"
	@echo "Execute:"
	@echo "  make run   -> Executa a Versão 1"
	@echo "=========================================================="

# Compilação do Módulo TAD (conta.o)
conta.o: $(CONTA_DIR)/conta.c $(CONTA_DIR)/conta.h
	$(CC) $(CFLAGS) -c $(CONTA_DIR)/conta.c -o $@

# Compilação e Ligação da Versão 1 com o TAD
versao1_leitura_suja: $(SRC_DIR)/versao1_leitura_suja.c conta.o
	$(CC) $(CFLAGS) $(SRC_DIR)/versao1_leitura_suja.c conta.o -o $@

# Executa a Versão 1 (4 leitores, 3 escritores, delays 60ms e 200ms)
run: versao1_leitura_suja
	./versao1_leitura_suja 4 3 60 200

# Limpeza dos binários e arquivos objeto
clean:
	rm -f $(TARGETS) $(OBJS)
	@echo "Executáveis e arquivos .o removidos com sucesso."

# Ajuda com os comandos disponíveis
help:
	@echo "Comandos disponíveis no Makefile:"
	@echo "  make       - Compila o TAD conta.o e o programa da Versão 1"
	@echo "  make run   - Executa a demonstração da Versão 1"
	@echo "  make clean - Remove os executáveis e arquivos temporários (.o)"
