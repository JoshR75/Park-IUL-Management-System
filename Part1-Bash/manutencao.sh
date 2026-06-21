#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº:124549       Nome:Josh Rufino
## Nome do Módulo: S2. Script: manutencao.sh
## Descrição/Explicação do Módulo:
##
##
#####################################################################################

## Este script não recebe nenhum argumento, e permite realizar a manutenção dos registos de estacionamento. 

## S2.1. Validações do script:
## O script valida se, no ficheiro estacionamentos.txt:
## • Todos os registos referem códigos de países existentes no ficheiro paises.txt;
## • Todas as matrículas registadas correspondem à especificação de formato dos países correspondentes;
## • Todos os registos têm uma data de saída superior à data de entrada;
## • Em caso de qualquer erro das condições anteriores, dá so_error S2.1 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S2.1.

## S2.2. Processamento:
## • O script move, do ficheiro estacionamentos.txt, todos os registos que estejam completos (com registo de entrada e registo de saída), mantendo o formato do ficheiro original, para ficheiros separados com o nome arquivo-<Ano>-<Mês>.park, com todos os registos agrupados pelo ano e mês indicados pelo nome do ficheiro. Ou seja, os registos são removidos do ficheiro estacionamentos.txt e acrescentados ao correspondente ficheiro arquivo-<Ano>-<Mês>.park, sendo que o ano e mês em questão são os do campo <DataSaída>. 
## • Quando acrescentar o registo ao ficheiro arquivo-<Ano>-<Mês>.park, este script acrescenta um campo <TempoParkMinutos> no final do registo, que corresponde ao tempo, em minutos, que durou esse registo de estacionamento (correspondente à diferença em minutos entre os dois campos anteriores).
## • Em caso de qualquer erro das condições anteriores, dá so_error S2.2 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S2.2.
## • O registo em cada ficheiro arquivo-<Ano>-<Mês>.park, tem então o formato:
## <Matrícula:string>:<Código País:string>:<Categoria:char>:<Nome do Condutor:string>: <DataEntrada:AAAA-MM-DDTHHhmm>:<DataSaída:AAAA-MM-DDTHHhmm>:<TempoParkMinutos:int>
## • Exemplo de um ficheiro arquivo-<Ano>-<Mês>.park, para janeiro de 2025:

# Definição dos ficheiros necessários para o processamento
ESTACIONAMENTOS="estacionamentos.txt"
PAISES="paises.txt"
TEMP_FILE="temp_estacionamentos.txt"

# 1. Verificação da existência dos ficheiros de entrada

# Verifica se o ficheiro 'paises.txt' existe; caso contrário, gera um erro e termina o script
if [[ ! -f "$PAISES" ]]; then
    so_error S2.1 "Ficheiro paises.txt não encontrado"
    exit 1
fi

# Verifica se o ficheiro 'estacionamentos.txt' existe; se não existir, cria-o vazio
if [[ ! -f "$ESTACIONAMENTOS" ]]; then
    touch "$ESTACIONAMENTOS"
fi

# 2. Processamento do ficheiro 'paises.txt' para carregar as regras de validação das matrículas

declare -A REGRAS_MATRICULA  # Declara um array associativo para armazenar as regras

while IFS= read -r linha; do
    [[ -z "$linha" ]] && continue  # Ignora linhas vazias

    # Separa os campos utilizando "###" como delimitador e remove caracteres '#' extra
    CODIGO=$(echo "$linha" | awk -F"###" '{print $1}' | sed 's/^#//; s/#$//')
    NOME=$(echo "$linha" | awk -F"###" '{print $2}' | sed 's/^#//; s/#$//')
    REGEX=$(echo "$linha" | awk -F"###" '{print $3}' | sed 's/^#//; s/#$//')

    # Se algum campo estiver vazio, gera um erro e termina o script
    if [[ -z "$CODIGO" || -z "$NOME" || -z "$REGEX" ]]; then
         so_error S2.1 "Formato inválido no ficheiro paises.txt na linha: $linha"
         exit 1
    fi

    # Associa o código do país à expressão regular correspondente para validação das matrículas
    REGRAS_MATRICULA["$CODIGO"]="$REGEX"
done < "$PAISES"

# 3. Validação dos registos presentes no ficheiro 'estacionamentos.txt'

while IFS=: read -r MATRICULA COD_PAIS CATEGORIA CONDUTOR DATA_ENTRADA DATA_SAIDA; do
    [[ -z "$MATRICULA" ]] && continue  # Ignora linhas vazias

    # Remove espaços em branco no início e no fim das datas
    DATA_ENTRADA=$(echo "$DATA_ENTRADA" | sed 's/^[[:space:]]//; s/[[:space:]]$//')
    DATA_SAIDA=$(echo "$DATA_SAIDA" | sed 's/^[[:space:]]//; s/[[:space:]]$//')

    # Verifica se o código de país consta nas regras carregadas
    if [[ -z "${REGRAS_MATRICULA[$COD_PAIS]}" ]]; then
         so_error S2.1 "Código de país inválido: $COD_PAIS"
         exit 1
    fi

    # Verifica se a matrícula corresponde à expressão regular do país associado
    if ! [[ "$MATRICULA" =~ ${REGRAS_MATRICULA[$COD_PAIS]} ]]; then
         so_error S2.1 "Matrícula inválida para o país $COD_PAIS: $MATRICULA"
         exit 1
    fi

    # Se houver data de saída, converte as datas para formato timestamp e valida a ordem cronológica
    if [[ -n "$DATA_SAIDA" ]]; then
         TS_ENTRADA=$(date -d "$(echo "$DATA_ENTRADA" | sed -e 's/T/ /' -e 's/h/:/')" +%s 2>/dev/null)
         TS_SAIDA=$(date -d "$(echo "$DATA_SAIDA" | sed -e 's/T/ /' -e 's/h/:/')" +%s 2>/dev/null)

         # Se a conversão falhar, gera erro
         if [[ -z "$TS_ENTRADA" || -z "$TS_SAIDA" ]]; then
              so_error S2.1 "Formato de data inválido para $MATRICULA"
              exit 1
         fi

         # Verifica se a data de saída é posterior à data de entrada
         if [[ "$TS_SAIDA" -le "$TS_ENTRADA" ]]; then
              so_error S2.1 "Data de saída menor ou igual à data de entrada para $MATRICULA"
              exit 1
         fi
    fi
done < "$ESTACIONAMENTOS"

so_success S2.1 "Validação concluída com sucesso!"

# 4. Processamento dos registos para armazenamento dos estacionamentos finalizados

# Verifica se o diretório atual tem permissões de escrita
if [ ! -w "$(pwd)" ]; then
    so_error S2.2 "Sem permissões para escrita na diretoria local"
    exit 1
fi

# Cria um ficheiro temporário para armazenar os registos ativos (viaturas ainda não saíram)
touch "$TEMP_FILE"

while IFS=: read -r MATRICULA COD_PAIS CATEGORIA CONDUTOR DATA_ENTRADA DATA_SAIDA; do
    [[ -z "$MATRICULA" ]] && continue  # Ignora linhas vazias

    # Remove espaços em branco das datas
    DATA_ENTRADA=$(echo "$DATA_ENTRADA" | sed 's/^[[:space:]]//; s/[[:space:]]$//')
    DATA_SAIDA=$(echo "$DATA_SAIDA" | sed 's/^[[:space:]]//; s/[[:space:]]$//')

    # Se houver data de saída, calcula o tempo de estacionamento e armazena o registo no ficheiro correspondente
    if [[ -n "$DATA_SAIDA" ]]; then
         TS_ENTRADA=$(date -d "$(echo "$DATA_ENTRADA" | sed -e 's/T/ /' -e 's/h/:/')" +%s)
         TS_SAIDA=$(date -d "$(echo "$DATA_SAIDA" | sed -e 's/T/ /' -e 's/h/:/')" +%s)
         TEMPO_PARK=$(( (TS_SAIDA - TS_ENTRADA) / 60 ))  # Tempo de estacionamento em minutos
         
         # Determina o nome do ficheiro onde será armazenado o registo (baseado no ano e mês)
         ANO_MES=$(echo "$DATA_SAIDA" | cut -d'T' -f1 | cut -d'-' -f1,2)
         ARQUIVO="arquivo-${ANO_MES}.park"

         # Regista a informação da viatura no ficheiro correspondente ao mês e ano da saída
         if ! echo "$MATRICULA:$COD_PAIS:$CATEGORIA:$CONDUTOR:$DATA_ENTRADA:$DATA_SAIDA:$TEMPO_PARK" >> "$ARQUIVO"; then
              so_error S2.2 "Erro ao escrever no ficheiro $ARQUIVO"
              exit 1
         fi
    else
         # Se a viatura ainda não saiu, mantém o registo no ficheiro temporário
         echo "$MATRICULA:$COD_PAIS:$CATEGORIA:$CONDUTOR:$DATA_ENTRADA" >> "$TEMP_FILE"
    fi
done < "$ESTACIONAMENTOS"

# Substitui o ficheiro original de estacionamentos pelo ficheiro temporário atualizado
mv "$TEMP_FILE" "$ESTACIONAMENTOS"

so_success S2.2 "Manutenção concluída com sucesso!"




