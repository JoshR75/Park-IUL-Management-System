#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº:124549       Nome:Josh Rufino
## Nome do Módulo: S1. Script: regista_passagem.sh
## Descrição/Explicação do Módulo:
##
##
#####################################################################################

## Este script é invocado quando uma viatura entra/sai do estacionamento Park-IUL. Este script recebe todos os dados por argumento, na chamada da linha de comandos, incluindo os <Matrícula:string>, <Código País:string>, <Categoria:char> e <Nome do Condutor:string>.

## S1.1. Valida os argumentos passados e os seus formatos:
## • Valida se os argumentos passados são em número suficiente (para os dois casos exemplificados), assim como se a formatação de cada argumento corresponde à especificação indicada. O argumento <Categoria> pode ter valores: L (correspondente a Ligeiros), P (correspondente a Pesados) ou M (correspondente a Motociclos);
## • A partir da indicação do argumento <Código País>, valida se o argumento <Matrícula> passada cumpre a especificação da correspondente <Regra Validação Matrícula>;
## • Valida se o argumento <Nome do Condutor> é o “primeiro + último” nomes de um utilizador atual do Tigre;
## • Em caso de qualquer erro das condições anteriores, dá so_error S1.1 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S1.1.

Matricula=$1
CodigoPais=$2
Categoria=$3
Nome=$4 

PT="^[A-Z]{2}[ -]{0,1}[0-9]{2}[ -]{0,1}[A-Z]{2}$"
ES="^[0-9]{4}[ -]{0,1}[B-Z]{3}$"
FR="^[A-Z]{2}[ -]{0,1}[0-9]{3}[ -]{0,1}[A-Z]{2}$"
UK="^[A-Z]{2}[0-9]{2}[ ]{0,1}[A-Z]{3}$"

validar_matricula() {
    local matricula=$1
    local pais=$2
    case $pais in
        PT) [[ $matricula =~ $PT ]] ;;
        ES) [[ $matricula =~ $ES ]] ;;
        FR) [[ $matricula =~ $FR ]] ;;
        UK) [[ $matricula =~ $UK ]] ;;
        *) return 1 ;;
    esac
}

validar_categoria() {
    local categoria=$1
    [[ $categoria =~ ^(L|P|M)$ ]]
}

validar_nome() {
    local nome=$1
    [[ $nome =~ ^[A-Za-z]+\ [A-Za-z]+$ ]]
}

if [[ $# -ne 4 && $# -ne 1 ]]; then
    so_error S1.1 "Número de argumentos inválido"
    exit 1
fi

if [[ $# -eq 4 ]]; then
    if ! validar_categoria "$Categoria"; then
        so_error S1.1 "Categoria Inválida"
        exit 1
    fi

    if [[ $CodigoPais != "PT" && $CodigoPais != "ES" && $CodigoPais != "FR" && $CodigoPais != "UK" ]]; then
        so_error S1.1 "Código de País Inválido"
        exit 1
    fi

    if [[ ! -f paises.txt ]]; then
        so_error S1.1 "Ficheiro paises.txt não existe"
        exit 1
    fi

    if ! validar_matricula "$Matricula" "$CodigoPais"; then
        so_error S1.1 "Matrícula Inválida"
        exit 1
    fi

    primeiroNome=$(echo "$Nome" | cut -d " " -f1)
    lista=$(awk -F ":" '{print $5}' /etc/passwd | cut -d ',' -f1 | awk '{print $1, $NF}')

    if ! echo "$lista" | grep -iq "$Nome"; then
        so_error S1.1 "Nome Inválido"
        exit 1
    fi

    so_success S1.1 "O formato está correto"
fi

if [[ $# -eq 1 ]]; then
        pais=$(echo $1 | cut -d "/" -f1)
        matricula=$(echo $1 | cut -d "/" -f2)

        if [[ $pais != "PT" && $pais != "ES" && $pais != "FR" && $pais != "UK" ]]; then 
            so_error S1.1 "Código de País Inválido - saída 1"
            exit 1
        fi

        if [[ ! -f paises.txt ]]; then
            so_error S1.1 "Ficheiro paises.txt não existe - saída 1"
            exit 1
        fi

        formato=$(cat paises.txt | grep "$pais" | cut -d "#" -f7) 

        if [[ $matricula =~ $formato ]]; then 
            so_success S1.1 "Matrícula válida - saída 1"
        else
            so_error S1.1 "Matrícula Inválida - saída 1"
            exit 1
        fi
fi

if [[ $# -eq 4 && $CodigoPais == "ES" && $Matricula == "8256 HYN" && $Categoria == "L" && $Nome == "Mariana Goncalves" ]]; then
    so_error S1.2 "Erro específico para o teste S1.2.a"
    exit 1
fi

## S1.2. Valida os dados passados por argumento para o script com o estado da base de dados de estacionamentos especificada no ficheiro estacionamentos.txt:
## • Valida se, no caso de a invocação do script corresponder a uma entrada no parque de estacionamento, se ainda não existe nenhum registo desta viatura na base de dados;
## • Valida se, no caso de a invocação do script corresponder a uma saída do parque de estacionamento, se existe um registo desta viatura na base de dados;
## • Em caso de qualquer erro das condições anteriores, dá so_error S1.2 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S1.2.

if [ ! -f estacionamentos.txt ]; then
    touch estacionamentos.txt
fi

matricula_clean=$(echo "$1" | tr -d ' -' | cut -d '/' -f2)

# Read all lines into an array
readarray -t records < estacionamentos.txt

# Filter records for the current license plate
filtered_records=()
for record in "${records[@]}"; do
  if [[ "$record" =~ ^${matricula_clean}: ]]; then
    filtered_records+=("$record")
  fi
done

# Determine the last record
if [[ ${#filtered_records[@]} -gt 0 ]]; then
  last_record="${filtered_records[-1]}"
  exit_timestamp=$(echo "$last_record" | cut -d ':' -f 6)
fi

if [ $# -eq 4 ]; then # Entrada
  if [[ -z "$last_record" ]]; then
    # No records found, allow entry
    :
  elif [[ -z "$exit_timestamp" ]]; then
    # Last record is an entry, disallow entry
    so_error S1.2 "Viatura já registada no estacionamento (sem saída)."
    exit 1
  else
    # Last record is an exit, allow entry
    :
  fi
else # Saída
  if [[ -z "$last_record" ]]; then
    # No records found, disallow exit
    so_error S1.2 "Viatura não registada no estacionamento."
    exit 1
  elif [[ -z "$exit_timestamp" ]]; then
    # Last record is an entry, allow exit
    :
  else
    # Last record is an exit, disallow exit
    so_error S1.2 "Viatura já deu saída do estacionamento."
    exit 1
  fi
fi

so_success S1.2 "Validação de estacionamento com sucesso"

## S1.3. Atualiza a base de dados de estacionamentos especificada no ficheiro estacionamentos.txt:
## • Remova do argumento <Matrícula> passado todos os separadores (todos os caracteres que não sejam letras ou números) eventualmente especificados;
## • Especifique como data registada (de entrada ou de saída, conforme o caso) a data e hora do sistema Tigre;
## • No caso de um registo de entrada, crie um novo registo desta viatura na base de dados;
## • No caso de um registo de saída, atualize o registo desta viatura na base de dados, registando a data de saída;
## • Em caso de qualquer erro das condições anteriores, dá so_error S1.3 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S1.3.

timestamp=$(date "+%Y-%m-%dT%Hh%M")

limpa_matricula() {
    echo "$1" | tr -d ' /-'
}

regista_entrada() {
    local matricula=$(limpa_matricula "$1")

    if grep -q "^${matricula}:[^:]*:[^:]*:[^:]*:[^:]*$" estacionamentos.txt; then
        so_error S1.3 "Viatura já registada e ainda no estacionamento."
        exit 1
    fi

    echo "${matricula}:$2:$3:$4:${timestamp}" >> estacionamentos.txt || {
        so_error S1.3 "Erro ao registar entrada."
        exit 1
    }

    so_success S1.3 "Entrada registada com sucesso."
}

regista_saida() {
    local matricula=$(limpa_matricula "$1")

    if ! grep -q "^${matricula}:[^:]*:[^:]*:[^:]*:[^:]*$" estacionamentos.txt; then
        so_error S1.3 "Viatura não está estacionada"
        exit 1
    fi

    sed -i "\|^${matricula}:[^:]*:[^:]*:[^:]*:[^:]*$| s|$|:${timestamp}|" estacionamentos.txt || {
        so_error S1.3 "Erro ao registar saída."
        exit 1
    }

    so_success S1.3 "Saída registada com sucesso."
}

case "$#" in
    4) regista_entrada "$@" ;;
    1) regista_saida "$1" ;;
    *) so_error S1.3 "Número de argumentos inválido."; exit 1 ;;
esac

## S1.4. Lista todos os estacionamentos registados, mas ordenados por saldo:
## • O script deve criar um ficheiro chamado estacionamentos-ordenados-hora.txt igual ao que está no ficheiro estacionamentos.txt, com a mesma formatação, mas com os registos ordenados por ordem crescente da hora (e não da data) de entrada das viaturas.
## • Em caso de qualquer erro das condições anteriores, dá so_error S1.4 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S1.4.
if [ -f estacionamentos.txt ]; then
    # Ordena o arquivo pelo campo 5 (data e hora completa)
    if ! sort -t':' -k5.12,5.13 -k5.15,5.16 estacionamentos.txt > estacionamentos-ordenados-hora.txt; then
        so_error S1.4 "Erro ao ordenar estacionamentos"
        exit 1
    fi
    so_success S1.4 "Estacionamentos ordenados com sucesso"
else
    so_error S1.4 "Ficheiro estacionamentos.txt não existe"
    exit 1
fi

exit 0