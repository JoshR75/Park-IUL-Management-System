#!/bin/bash
# SO_HIDE_DEBUG=1                   ## Uncomment this line to hide all @DEBUG statements
# SO_HIDE_COLOURS=1                 ## Uncomment this line to disable all escape colouring
. so_utils.sh                       ## This is required to activate the macros so_success, so_error, and so_debug

#####################################################################################
## ISCTE-IUL: Trabalho prático de Sistemas Operativos 2024/2025, Enunciado Versão 1
##
## Aluno: Nº:124549       Nome:Josh Pinto Rufino
## Nome do Módulo: S3. Script: stats.sh
## Descrição/Explicação do Módulo:
##
##
#####################################################################################

## Este script obtém informações sobre o sistema Park-IUL, afixando os resultados das estatísticas pedidas no formato standard HTML no Standard Output e no ficheiro stats.html. Cada invocação deste script apaga e cria de novo o ficheiro stats.html, e poderá resultar em uma ou várias estatísticas a serem produzidas, todas elas deverão ser guardadas no mesmo ficheiro stats.html, pela ordem que foram especificadas pelos argumentos do script.

## S3.1. Validações:
## O script valida se, na diretoria atual, existe algum ficheiro com o nome arquivo-<Ano>-<Mês>.park, gerado pelo Script: manutencao.sh. Se não existirem ou não puderem ser lidos, dá so_error S3.1 <descrição do erro>, indicando o erro em questão, e termina. Caso contrário, dá so_success S3.1.

# Verificar se existe pelo menos um arquivo arquivo-*.park

if ! ls arquivo-*.park >/dev/null 2>&1; then
    so_error S3.1 "Ficheiro arquivo-*.park não encontrado"
    exit 1
fi

for file in arquivo-*.park; do
    if [ ! -r $file ]; then
        so_error S3.1 "Ficheiro $file não tem permissão de leitura"
        exit 1
    fi
done

# Verificar se o arquivo paises.txt existe
if [ ! -f paises.txt ]; then
    so_error S3.1 
    exit 1
fi

if [ ! -r paises.txt ]; then
    so_error S3.1 "Ficheiro paises.txt não tem premissao de leitura"
    exit 1
fi


if [ ! -f estacionamentos.txt ]; then
    so_error S3.1 
    exit 1
fi

if [ ! -r estacionamentos.txt ]; then
    so_error S3.1 "Ficheiro estacionamentos.txt não tem premissao de leitura"
    exit 1
fi

# Validar argumentos fornecidos
for arg in "$@"; do
    if ! [[ "$arg" =~ ^[1-7]$ ]]; then
        so_error S3.1 "Argumento inválido: $arg. Apenas valores entre 1 e 7 são permitidos."
        exit 1
    fi
done

# Exibir mensagem de sucesso apenas se todas as validações forem concluídas


if [ $# -eq 0 ]; then
    # Processar todas as estatísticas (1 a 7)
    echo -e "$case1" >> stats.html
    echo -e "$case2" >> stats.html
    echo -e "$case3" >> stats.html
    echo -e "$case4" >> stats.html
    echo -e "$case5" >> stats.html
    echo -e "$case6" >> stats.html
    echo -e "$case7" >> stats.html
else
    # Processar estatísticas específicas
    for i in "$@"; do
        case $i in
            1) echo -e "$case1" >> stats.html;;
            2) echo -e "$case2" >> stats.html;;
            3) echo -e "$case3" >> stats.html;;
            4) echo -e "$case4" >> stats.html;;
            5) echo -e "$case5" >> stats.html;;
            6) echo -e "$case6" >> stats.html;;
            7) echo -e "$case7" >> stats.html;;
        esac
    done
fi

## S3.2. Estatísticas:
## Cada uma das estatísticas seguintes diz respeito à extração de informação dos ficheiros do sistema Park-IUL. Caso não haja informação suficiente para preencher a estatística, poderá apresentar uma lista vazia.
## S3.2.1.  Obter uma lista das matrículas e dos nomes de todos os condutores cujas viaturas estão ainda estacionadas no parque, ordenados alfabeticamente por nome de condutor:
## <h2>Stats1:</h2>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b>Condutor:</b> <Nome do Condutor></li>
## <li><b>Matrícula:</b> <Matrícula> <b>Condutor:</b> <Nome do Condutor></li>
## ...
## </ul>

# S3.2.1: Obter uma lista das matrículas e dos nomes de todos os condutores cujas viaturas estão ainda estacionadas no parque
case1="<h2>Stats1:</h2>\n<ul>"
declare -A seen  # Array associativo para rastrear condutores únicos

while IFS=: read -r matricula _ _ condutor entrada saida; do
    if [ -z "$saida" ] && [ -z "${seen[$condutor]}" ]; then
        case1="$case1\n<li><b>Matrícula:</b> $matricula <b>Condutor:</b> $condutor</li>"
        seen[$condutor]=1  # Marca o condutor como visto para não repetir
    fi
done < <(awk -F: 'NF==5' estacionamentos.txt | sort -t ':' -k4)  # Filtra linhas com apenas 5 campos (sem saída) e ordena pelo condutor

case1="$case1\n</ul>"


## S3.2.2. Obter uma lista do top3 das matrículas e do tempo estacionado das viaturas que já terminaram o estacionamento e passaram mais tempo estacionadas, ordenados decrescentemente pelo tempo de estacionamento (considere apenas os estacionamentos cujos tempos já foram calculados):
## <h2>Stats2:</h2>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b>Tempo estacionado:</b> <TempoParkMinutos></li>
## <li><b>Matrícula:</b> <Matrícula> <b>Tempo estacionado:</b> <TempoParkMinutos></li>
## <li><b>Matrícula:</b> <Matrícula> <b>Tempo estacionado:</b> <TempoParkMinutos></li>
## </ul>


case2="<h2>Stats2:</h2>\n<ul>"
declare -A tempo_total  # Array associativo para armazenar tempo total por matrícula
declare -A condutores   # Guarda o condutor correspondente a cada matrícula

# Ler os ficheiros arquivo-*.park e somar tempos para cada matrícula
while IFS=: read -r matricula _ _ condutor _ _ tempo; do
    if [ -n "$tempo" ]; then  # Verifica se a 7ª coluna (tempo) está preenchida
        ((tempo_total[$matricula]+=$tempo))  # Soma o tempo ao total da matrícula
        condutores[$matricula]=$condutor  # Armazena o nome do condutor
    fi
done < <(cat arquivo-*.park)

# Criar lista para ordenação (tempo_total:matricula)
for matricula in "${!tempo_total[@]}"; do
    lista+="${tempo_total[$matricula]}:$matricula:${condutores[$matricula]}"$'\n'
done

# Ordena de forma decrescente pelo tempo total e pega os top 3
IFS=$'\n' sorted=($(sort -t ':' -k1 -nr <<< "$lista" | head -n 3))

# Construir a saída HTML
for linha in "${sorted[@]}"; do
    tempo=$(echo "$linha" | cut -d ':' -f1)
    matricula=$(echo "$linha" | cut -d ':' -f2)
    condutor=$(echo "$linha" | cut -d ':' -f3)
    case2="$case2\n<li><b>Matrícula:</b> $matricula <b>Tempo estacionado:</b> $tempo</li>"
done

case2="$case2\n</ul>"


## S3.2.3. Obter as somas dos tempos de estacionamento das viaturas que não são motociclos, agrupadas pelo nome do país da matrícula (considere apenas os estacionamentos cujos tempos já foram calculados):
## <h2>Stats3:</h2>
## <ul>
## <li><b>País:</b> <Nome País> <b>Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## <li><b>País:</b> <Nome País> <b>Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## ...
## </ul>

# Inicializa as variáveis para armazenar o total de tempo de estacionamento por país
PT=0
ES=0
FR=0
UK=0

# Processa os registros dos arquivos 'arquivo-*.park', excluindo os registros de motocicletas
# O 'sed' substitui os espaços por sublinhados para garantir o tratamento adequado dos nomes
for i in $(grep -h -v ":M:" arquivo-*.park | sed 's/ /_/'); do
    # Extrai o país e o tempo de estacionamento do registro
    pais=$(echo $i | cut -d ':' -f2)
    tempo=$(echo $i | cut -d ':' -f7)

    # Atualiza o total de tempo de estacionamento para cada país
    case $pais in
        PT) PT=$((PT+tempo));;  # Atualiza o tempo total para Portugal
        ES) ES=$((ES+tempo));;  # Atualiza o tempo total para Espanha
        FR) FR=$((FR+tempo));;  # Atualiza o tempo total para França
        UK) UK=$((UK+tempo));;  # Atualiza o tempo total para Reino Unido
    esac
done


## S3.2.4. Listar a matrícula, código de país e data de entrada dos 3 estacionamentos, já terminados ou não, que registaram uma entrada mais tarde (hora de entrada) no parque de estacionamento, ordenados crescentemente por hora de entrada:
## <h2>Stats4:</h2>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b>País:</b> <Código País> <b>Data Entrada:</b> <DataEntrada></li>
## <li><b>Matrícula:</b> <Matrícula> <b>País:</b> <Código País> <b>Data Entrada:</b> <DataEntrada></li>
## <li><b>Matrícula:</b> <Matrícula> <b>País:</b> <Código País> <b>Data Entrada:</b> <DataEntrada></li>
## </ul>

# Inicializa a variável para o início do conteúdo HTML das estatísticas (Stats4)
case4="<h2>Stats4:</h2>"
case4="$case4\n<ul>"

# Para cada linha dos últimos 3 registros no arquivo 'estacionamentos.txt', que são processados após o uso do comando 'tail -3'
# O 'sed' substitui os espaços por sublinhados para garantir que os nomes sejam tratados sem espaços
for i in $(cat estacionamentos.txt | tail -3 | sed 's/ /_/'); do
    # Extrai a matrícula, o país e a data de entrada a partir dos campos separados por ':'
    matricula=$(echo $i | cut -d ':' -f 1)
    pais=$(echo $i | cut -d ':' -f 2)
    data=$(echo $i | cut -d ':' -f 5)

    # Adiciona à lista HTML a matrícula, país e data de entrada
    case4="$case4\n<li><b>Matrícula:</b> $matricula <b>País:</b> $pais <b>Data Entrada:</b> $data</li>"
done

# Fecha a lista HTML
case4="$case4\n</ul>"


## S3.2.5. Tendo em consideração que um utilizador poderá ter várias viaturas, determine o tempo total, medido em dias, horas e minutos gasto por cada utilizador da plataforma (ou seja, agrupe os minutos em dias e horas).
## <h2>Stats5:</h2>
## <ul>
## <li><b>Condutor:</b> <NomeCondutor> <b>Tempo  total:</b> <x> dia(s), <y> hora(s) e <z> minuto(s)</li>
## <li><b>Condutor:</b> <NomeCondutor> <b>Tempo  total:</b> <x> dia(s), <y> hora(s) e <z> minuto(s)</li>
## ...
## </ul>

# Obtém a lista de nomes dos condutores, extraindo o quarto campo (nome) de cada linha, removendo espaços e ordenando-os de forma única
nomes=$(cat arquivo-*.park | cut -d ':' -f 4 | sed 's/ /_/' | sort | uniq)

# Inicializa a variável para o início do conteúdo HTML das estatísticas (Stats5)
case5="<h2>Stats5:</h2>"
case5="$case5\n<ul>"

# Para cada nome de condutor na lista
for i in $nomes; do
    # Substitui o caractere de sublinhado (_) por espaço para restaurar o nome original
    correto=$(echo $i | sed 's/_/ /')

    # Inicializa a variável total para somar o tempo de estacionamento
    total=0

    # Para cada entrada do condutor no arquivo, soma o tempo total de estacionamento
    for j in $(cat arquivo-*.park | grep "$correto" | cut -d ':' -f 7); do
        total=$((total+j))
    done

    # Converte o tempo total de minutos para dias, horas e minutos
    dias=$((total/1440))
    horas=$((total%1440/60))
    minutos=$((total%60))

    # Adiciona à lista HTML o nome do condutor e o tempo total de estacionamento
    case5="$case5\n<li><b>Condutor:</b> $correto <b>Tempo total:</b> $dias dia(s), $horas hora(s) e $minutos minuto(s)</li>"
done

# Fecha a lista HTML
case5="$case5\n</ul>"


## S3.2.6. Liste as matrículas das viaturas distintas e o tempo total de estacionamento de cada uma, agrupadas pelo nome do país com um totalizador de tempo de estacionamento por grupo, e totalizador de tempo global.
## <h2>Stats6:</h2>
## <ul>
## <li><b>País:</b> <Nome País></li>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b> Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## <li><b>Matrícula:</b> <Matrícula> <b> Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## ...
## </ul>
## <li><b>País:</b> <Nome País></li>
## <ul>
## <li><b>Matrícula:</b> <Matrícula> <b> Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## <li><b>Matrícula:</b> <Matrícula> <b> Total tempo estacionado:</b> <ΣTempoParkMinutos></li>
## ...
## </ul>
## ...
## </ul>

# Inicializa a variável para o início do conteúdo HTML das estatísticas (Stats6)
case6="<h2>Stats6:</h2>"
case6="$case6\n<ul>"

# Itera sobre os países (Portugal, Espanha, França, Reino Unido)
for i in PT ES FR UK; do
    # Atribui o nome completo do país com base no código de país (PT, ES, FR, UK)
    case $i in
        PT) nomePais="Portugal";;
        ES) nomePais="Espanha";;
        FR) nomePais="França";;
        UK) nomePais="Reino Unido";;
    esac

    # Obtém as matrículas de veículos que estão registados no país atual
    matriculasNoPais=$(grep -h ":$i:" arquivo-*.park | cut -d ':' -f 1 | sort | uniq)
    
    # Adiciona o nome do país à lista HTML
    case6="$case6\n<li><b>País:</b> $nomePais</li>"
    case6="$case6\n<ul>"

    # Para cada matrícula encontrada para o país, calcula o tempo total de estacionamento
    for j in $matriculasNoPais; do
        total=0
        
        # Soma o tempo total de estacionamento para cada matrícula, verificando as entradas correspondentes no arquivo
        for k in $(grep -h ":$i:" arquivo-*.park | grep "$j" | cut -d ':' -f 7); do
            total=$((total+k))
        done

        # Adiciona à lista HTML a matrícula e o total de tempo estacionado para o veículo
        case6="$case6\n<li><b>Matrícula:</b> $j <b> Total tempo estacionado:</b> $total</li>"
    done

    # Fecha a lista de matrículas para o país atual
    case6="$case6\n</ul>"
done

# Fecha a lista geral para todos os países
case6="$case6\n</ul>"


## S3.2.7. Obter uma lista do top3 dos nomes mais compridos de condutores cujas viaturas já estiveram estacionadas no parque (ou que ainda estão estacionadas no parque), ordenados decrescentemente pelo tamanho do nome do condutor:
## <h2>Stats7:</h2>
## <ul>
## <li><b> Condutor:</b> <Nome do Condutor mais comprido></li>
## <li><b> Condutor:</b> <Nome do Condutor segundo mais comprido></li>
## <li><b> Condutor:</b> <Nome do Condutor terceiro mais comprido></li>
## </ul>

# Inicializa a variável para o início do conteúdo HTML das estatísticas (Stats7)
case7="<h2>Stats7:</h2>"
case7="$case7\n<ul>"

# Obtém os nomes dos condutores dos ficheiros de estacionamento, calcula o comprimento de cada nome,
# ordena por ordem decrescente e seleciona os 3 condutores com os nomes mais longos, sem repetições
nomes=$(cat estacionamentos.txt arquivo-*.park | cut -d ':' -f 4 | awk '{ print length, $0 }' | sort -n -r | cut -d ' ' -f 2- | uniq | head -3)

# Para cada nome de condutor encontrado, adiciona-o à lista HTML, substituindo os espaços por underscores
# para garantir a compatibilidade com o formato de entrada
for i in $(echo -e "$nomes" | sed 's/ /_/'); do
    nome=$(echo $i | sed 's/_/ /')  # Reverte os underscores de volta para espaços
    case7="$case7\n<li><b> Condutor:</b> $nome</li>"  # Adiciona o nome do condutor à lista
done

# Fecha a lista HTML e adiciona a mensagem de sucesso ao processamento das estatísticas
case7="$case7\n</ul>"

# Regista o sucesso do processamento das estatísticas
so_success S3.1 "Estatísticas processadas com sucesso"


## S3.3. Processamento do script:
## S3.3.1. O script cria uma página em formato HTML, chamada stats.html, onde lista as várias estatísticas pedidas.
## O ficheiro stats.html tem o seguinte formato:
## <html><head><meta charset="UTF-8"><title>Park-IUL: Estatísticas de estacionamento</title></head>
## <body><h1>Lista atualizada em <Data Atual, formato AAAA-MM-DD> <Hora Atual, formato HH:MM:SS></h1>
## [html da estatística pedida]
## [html da estatística pedida]
## ...
## </body></html>
## Sempre que o script for chamado, deverá:
## • Criar o ficheiro stats.html.
## • Preencher, neste ficheiro, o cabeçalho, com as duas linhas HTML descritas acima, substituindo os campos pelos valores de data e hora pelos do sistema.
## • Ciclicamente, preencher cada uma das estatísticas pedidas, pela ordem pedida, com o HTML correspondente ao indicado na secção S3.2.
## • No final de todas as estatísticas preenchidas, terminar o ficheiro com a última linha “</body></html>”

if [ -f stats.html ]; then
    rm stats.html
fi
touch stats.html

echo "<html><head><meta charset=\"UTF-8\"><title>Park-IUL: Estatísticas de estacionamento</title></head>" >> stats.html
echo "<body><h1>Lista atualizada em $(date +%Y-%m-%d) $(date +%H:%M:%S)</h1>" >> stats.html
if [ $# -eq 0 ]; then
    echo -e "$case1" >> stats.html
    echo -e "$case2" >> stats.html
    echo -e "$case3" >> stats.html
    echo -e "$case4" >> stats.html
    echo -e "$case5" >> stats.html
    echo -e "$case6" >> stats.html
    echo -e "$case7" >> stats.html
else
    for i in $*; do
        case $i in
            1) echo -e "$case1" >> stats.html;;
            2) echo -e "$case2" >> stats.html;;
            3) echo -e "$case3" >> stats.html;;
            4) echo -e "$case4" >> stats.html;;
            5) echo -e "$case5" >> stats.html;;
            6) echo -e "$case6" >> stats.html;;
            7) echo -e "$case7" >> stats.html;;
        esac
    done
fi
echo "</body></html>" >> stats.html
so_success S3.3 "Processamento do script concluído com sucesso"