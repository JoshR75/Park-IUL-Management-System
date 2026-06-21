#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº:124549       Nome:Josh Rufino
## Nome do Módulo: S4. Script: menu.sh
## Descrição/Explicação do Módulo:
##
##
#####################################################################################

## Este script invoca os scripts restantes, não recebendo argumentos.
## Atenção: Não é suposto que volte a fazer nenhuma das funcionalidades dos scripts anteriores. O propósito aqui é simplesmente termos uma forma centralizada de invocar os restantes scripts.
## S4.1. Apresentação:
## S4.1.1. O script apresenta (pode usar echo, cat ou outro, sem “limpar” o ecrã) um menu com as opções abaixo indicadas.

while true; do
    # Exibe o menu de opções para o utilizador
    echo "MENU:"
    echo "1: Regista passagem de Entrada no estacionamento"
    echo "2: Regista passagem de Saída no estacionamento"
    echo "3: Manutenção"
    echo "4: Estatísticas"
    echo "0: Sair"
    read -p "Opção: " opcao  # Lê a opção escolhida pelo utilizador

    case $opcao in
        0)
            echo "A sair..."  # Mensagem de saída
            so_success S4.2.1 0  # Regista o sucesso da saída
            exit 0  # Encerra o script
            ;;
        1)
            # Regista a entrada no estacionamento
            echo "Regista passagem de Entrada no estacionamento:"
            read -p "Indique a matrícula da viatura: " matricula  # Lê a matrícula da viatura
            read -p "Indique o código do país de origem da viatura: " codigo_pais  # Lê o código do país
            read -p "Indique a categoria da viatura [L(igeiro)|P(esado)|M(otociclo)]: " categoria  # Lê a categoria
            read -p "Indique o nome do condutor da viatura: " nome_condutor  # Lê o nome do condutor
            so_success S4.2.1 1  # Regista o sucesso da entrada
            # Invoca o script regista_passagem.sh para registar a passagem de entrada
            ./regista_passagem.sh "$matricula" "$codigo_pais" "$categoria" "$nome_condutor"
            so_success S4.3 0  # Regista o sucesso da execução
            ;;
        2)
            # Regista a saída do estacionamento
            echo "Regista passagem de Saída no estacionamento:"
            read -p "Indique a matrícula da viatura: " matricula_saida  # Lê a matrícula da viatura
            read -p "Indique o código do país de origem da viatura: " codigo_pais_saida  # Lê o código do país
            so_success S4.2.1 2  # Regista o sucesso da saída
            # Invoca o script regista_passagem.sh para registar a passagem de saída
            ./regista_passagem.sh "$codigo_pais_saida/$matricula_saida"
            so_success S4.4 0  # Regista o sucesso da execução
            ;;
        3)
            # Menu de manutenção
            echo "Manutenção"
            so_success S4.2.1 3  # Regista o sucesso da manutenção
            # Invoca o script manutencao.sh para a manutenção
            ./manutencao.sh
            so_success S4.5 0  # Regista o sucesso da execução
            ;;
        4)
            # Menu de estatísticas
            echo "Estatísticas:"
            echo "1: Matrículas e condutores cujas viaturas estão ainda estacionadas no parque"
            echo "2: Top 3 das matrículas das viaturas que passaram mais tempo estacionadas"
            echo "3: Tempos de estacionamento de ligeiros e pesados agrupados por país"
            echo "4: Top 3 das matrículas das viaturas que estacionaram mais tarde num dia"
            echo "5: Tempo total de estacionamento por utilizador"
            echo "6: Matrículas e tempo total de estacionamento delas, agrupadas por país da matrícula"
            echo "7: Top 3 nomes de condutores mais longos"
            echo "8: Todas as estatísticas anteriores, na ordem numérica indicada"
            read -p "Indique quais as estatísticas a incluir, opções separadas por espaço: " estatisticas  # Lê as opções de estatísticas
            so_success S4.2.1 4  # Regista o sucesso da escolha das estatísticas
            # Verifica se nenhuma opção foi selecionada
            if [ -z "$estatisticas" ]; then
                echo "Nenhuma opção selecionada. Por favor, tente novamente."
                so_error S4.6  # Regista um erro se nenhuma opção foi selecionada
            elif [[ "$estatisticas" =~ (^| )8($| ) ]]; then
                # Se a opção 8 for selecionada, invoca stats.sh sem argumentos
                ./stats.sh
                so_success S4.6 0  # Regista o sucesso da execução
            else
                # Passa as opções selecionadas para o script stats.sh
                ./stats.sh $estatisticas
                num_stats=$(echo "$estatisticas" | wc -w)  # Conta o número de estatísticas selecionadas
                so_success S4.6 $num_stats  # Regista o sucesso com o número de estatísticas
            fi
            ;;
        *)
            # Caso o utilizador insira uma opção inválida
            so_error S4.2.1 "$opcao"  # Regista o erro de opção inválida
            echo "Opção inválida. Por favor, tente novamente."
            ;;
    esac
done