/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 2 de Sistemas Operativos 2024/2025, Enunciado Versão 4+
 **
 ** Aluno: Nº:124549       Nome:Josh Rufino
 ** Nome do Módulo: servidor.c
 ** Descrição/Explicação do Módulo:
 **
 **
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "common.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // unlink
#include <sys/types.h>
#include <sys/stat.h>   // mkfifo
#include <errno.h>
#include <time.h>
#include <sys/wait.h>

/* Variáveis Globais */
Estacionamento clientRequest;           // Pedido enviado do Cliente para o Servidor
Estacionamento *lugaresEstacionamento;  // Array de Lugares de Estacionamento do parque
int dimensaoMaximaParque;               // Dimensão Máxima do parque (BD), recebida por argumento do programa
int indexClienteBD;                     // Índice do cliente que fez o pedido ao servidor/servidor dedicado na BD
long posicaoLogfile;                    // Posição no ficheiro Logfile para escrever o log da entrada corrente
LogItem logItem;                        // Informação da entrada corrente a escrever no logfile

/**
 * @brief  Processamento do processo Servidor e dos processos Servidor Dedicado
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 * @param  argc (I) número de Strings do array argv
 * @param  argv (I) array de lugares de estacionamento que irá servir de BD
 * @return Success (0) or not (<> 0)
 */
int main(int argc, char *argv[]) {
    so_debug("<");

    s1_IniciaServidor(argc, argv);
    s2_MainServidor();

    so_error("Servidor", "O programa nunca deveria ter chegado a este ponto!");
    so_debug(">");
    return 0;
}

/**
 * @brief  s1_iniciaServidor Ler a descrição da tarefa S1 no enunciado.
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 * @param  argc (I) número de Strings do array argv
 * @param  argv (I) array de lugares de estacionamento que irá servir de BD
 */
void s1_IniciaServidor(int argc, char *argv[]) {
    so_debug("<");

    s1_1_ObtemDimensaoParque(argc, argv, &dimensaoMaximaParque);
    s1_2_CriaBD(dimensaoMaximaParque, &lugaresEstacionamento);
    s1_3_ArmaSinaisServidor();
    s1_4_CriaFifoServidor(FILE_REQUESTS);

    so_debug(">");
}

/**
 * @brief  s1_1_ObtemDimensaoParque Ler a descrição da tarefa S1.1 no enunciado
 * @param  argc (I) número de Strings do array argv
 * @param  argv (I) array de lugares de estacionamento que irá servir de BD
 * @param  pdimensaoMaximaParque (O) número máximo de lugares do parque, especificado pelo utilizador
 */
void s1_1_ObtemDimensaoParque(int argc, char *argv[], int *pdimensaoMaximaParque) {
    so_debug("< [@param argc:%d, argv:%p]", argc, argv);

    if (argc < 2) {
        so_error("S1.1", "Faltam argumentos: indique a dimensão do parque.");
        exit(EXIT_FAILURE);
    }

    char *endptr;
    errno = 0;
    long valor = strtol(argv[1], &endptr, 10);

    if (errno != 0 || *endptr != '\0') {
        so_error("S1.1", "O argumento fornecido não é um número válido.");
        exit(EXIT_FAILURE);
    }

    if (valor <= 0 || valor > INT_MAX) {
        so_error("S1.1", "A dimensão do parque deve ser um inteiro positivo válido.");
        exit(EXIT_FAILURE);
    }

    *pdimensaoMaximaParque = (int)valor;

    char msg[100];
    snprintf(msg, sizeof(msg), "Dimensão do parque definida para %d lugares.", *pdimensaoMaximaParque);
    so_success("S1.1","%s", msg);

    so_debug("> [@param +pdimensaoMaximaParque:%d]", *pdimensaoMaximaParque);
    so_debug("> [@param *pdimensaoMaximaParque:%d]", *pdimensaoMaximaParque);
}

/**
 * @brief  s1_2_CriaBD Ler a descrição da tarefa S1.2 no enunciado
 * @param  dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 * @param  plugaresEstacionamento (O) array de lugares de estacionamento que irá servir de BD
 */
void s1_2_CriaBD(int dimensaoMaximaParque, Estacionamento **plugaresEstacionamento) {
    so_debug("< [@param dimensaoMaximaParque:%d]", dimensaoMaximaParque);

    /* Validate input parameter */
    if (dimensaoMaximaParque <= 0) {
        so_error("S1.2", "Dimensão do parque inválida (deve ser > 0)");
        exit(EXIT_FAILURE);
    }

    /* Allocate memory for the parking lot */
    *plugaresEstacionamento = (Estacionamento *)malloc(dimensaoMaximaParque * sizeof(Estacionamento));
    if (*plugaresEstacionamento == NULL) {
        so_error("S1.2", "Falha na alocação de memória para o parque de estacionamento");
        exit(EXIT_FAILURE);
    }

    /* Initialize all parking spots (assuming Estacionamento is a simple type) */
    for (int i = 0; i < dimensaoMaximaParque; i++) {
        // (*plugaresEstacionamento)[i].ocupado = FALSE; // Mark the spot as available
        (*plugaresEstacionamento)[i].viatura.matricula[0] = '\0'; // Initialize matricula as an empty string
        (*plugaresEstacionamento)[i].viatura.pais[0] = '\0';      // Initialize pais as an empty string
        (*plugaresEstacionamento)[i].viatura.categoria = '\0';    // Initialize categoria as empty
        (*plugaresEstacionamento)[i].viatura.nomeCondutor[0] = '\0'; // Initialize nomeCondutor as an empty string
        (*plugaresEstacionamento)[i].pidCliente = -1;              // Initialize pidCliente to an invalid value
        (*plugaresEstacionamento)[i].pidServidorDedicado = -1;     // Initialize pidServidorDedicado to an invalid value
    }

    so_success("S1.2", "Parque de estacionamento criado com sucesso");
    so_debug("> [*plugaresEstacionamento:%p]", *plugaresEstacionamento);
}

/**
 * @brief  s1_3_ArmaSinaisServidor Ler a descrição da tarefa S1.3 no enunciado
 */
void s1_3_ArmaSinaisServidor() {
    so_debug("<");

    if (signal(SIGINT, s3_TrataCtrlC) == SIG_ERR) {
        so_error("S1.3", "Erro ao armar sinal SIGINT");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGCHLD, s5_TrataTerminouServidorDedicado) == SIG_ERR) {
        so_error("S1.3", "Erro ao armar sinal SIGCHLD");
        exit(EXIT_FAILURE);
    }
    
    so_success("S1.3", "Sinais armados com sucesso");
    so_debug(">");
}

/**
 * @brief  s1_4_CriaFifoServidor Ler a descrição da tarefa S1.4 no enunciado
 * @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
 */
void s1_4_CriaFifoServidor(char *filenameFifoServidor) {
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);

    /* Remove a FIFO anterior, se existir */
    if (unlink(filenameFifoServidor) == -1 && errno != ENOENT) {
        so_error("S1.4", "Erro ao remover FIFO anterior");
        exit(EXIT_FAILURE);
    }
    
    /* Cria a FIFO com permissões 0666 (leitura e escrita para todos) */
    if (mkfifo(filenameFifoServidor, 0666) == -1) {
        so_error("S1.4", "Erro ao criar FIFO");
        exit(EXIT_FAILURE);
    }
    
    so_success("S1.4", "FIFO criada com sucesso");
    so_debug(">");
}

/**
 * @brief  s2_MainServidor Ler a descrição da tarefa S2 no enunciado.
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO, exceto depois de
 *         realizada a função s2_1_AbreFifoServidor(), altura em que podem
 *         comentar o statement sleep abaixo (que, neste momento está aqui
 *         para evitar que os alunos tenham uma espera ativa no seu código)
 */
void s2_MainServidor() {
    so_debug("<");

    FILE *fFifoServidor;
    while (TRUE) {
        s2_1_AbreFifoServidor(FILE_REQUESTS, &fFifoServidor);
        s2_2_LePedidosFifoServidor(fFifoServidor);
        // sleep(10);  // TEMPORÁRIO, os alunos deverão comentar este statement apenas
                    // depois de terem a certeza que não terão uma espera ativa
    }

    so_debug(">");
}

/**
 * @brief  s2_1_AbreFifoServidor Ler a descrição da tarefa S2.1 no enunciado
 * @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
 * @param  pfFifoServidor (O) descritor aberto do ficheiro do FIFO do servidor
 */
void s2_1_AbreFifoServidor(char *filenameFifoServidor, FILE **pfFifoServidor) {
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);

    /* Initialize the file pointer to NULL in case fopen fails */
    *pfFifoServidor = NULL;

    /* Attempt to open the FIFO for reading in blocking mode */
    while ((*pfFifoServidor = fopen(filenameFifoServidor, "r")) == NULL) {
        if (errno == EINTR) {
            /* If interrupted by a signal, log and retry */
            so_debug("Abertura do FIFO interrompida por sinal, tentando novamente...");
            continue;
        } else {
            /* If fopen failed for a reason other than EINTR, it's an error */
            so_error("S2.1", "Erro ao abrir FIFO do Servidor para leitura");
            s4_EncerraServidor(filenameFifoServidor);
            exit(EXIT_FAILURE);
        }
    }

    so_success("S2.1", "FIFO do Servidor aberto para leitura com sucesso");
    so_debug("> [*pfFifoServidor:%p]", *pfFifoServidor);
}

/**
 * @brief  s2_2_LePedidosFifoServidor Ler a descrição da tarefa S2.2 no enunciado.
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 * @param  fFifoServidor (I) descritor aberto do ficheiro do FIFO do servidor
 */
void s2_2_LePedidosFifoServidor(FILE *fFifoServidor) {
    so_debug("<");

    int terminaCiclo2 = FALSE;
    while (TRUE) {
        terminaCiclo2 = s2_2_1_LePedido(fFifoServidor, &clientRequest);
        if (terminaCiclo2)
            break;
        s2_2_2_ProcuraLugarDisponivelBD(clientRequest, lugaresEstacionamento, dimensaoMaximaParque, &indexClienteBD);
        s2_2_3_CriaServidorDedicado(lugaresEstacionamento, indexClienteBD);
    }

    so_debug(">");
}

/**
 * @brief  s2_2_1_LePedido Ler a descrição da tarefa S2.2.1 no enunciado
 * @param  fFifoServidor (I) descritor aberto do ficheiro do FIFO do servidor
 * @param  pclientRequest (O) pedido recebido, enviado por um Cliente
 * @return TRUE se não conseguiu ler um pedido porque o FIFO não tem mais pedidos.
 */
int s2_2_1_LePedido(FILE *fFifoServidor, Estacionamento *pclientRequest) {
    int naoHaMaisPedidos = TRUE;
    so_debug("< [@param fFifoServidor:%p]", fFifoServidor);

    if (fread(pclientRequest, sizeof(Estacionamento), 1, fFifoServidor) == 1) {
        naoHaMaisPedidos = FALSE;
        so_success("S2.2.1", "Li Pedido do FIFO");
    } else {
        if (feof(fFifoServidor)) {
            so_success("S2.2.1", "Não há mais registos no FIFO");
            so_debug("Fim do FIFO atingido.");
        } else {
            so_error("S2.2.1", "Erro ao ler do FIFO.");
            s4_EncerraServidor(FILE_REQUESTS);
            exit(EXIT_FAILURE);
        }
    }

    so_debug("> [naoHaMaisPedidos:%d, *pclientRequest:[%s:%s:%c:%s:%d.%d]]",
             naoHaMaisPedidos,
             pclientRequest->viatura.matricula,
             pclientRequest->viatura.pais,
             pclientRequest->viatura.categoria,
             pclientRequest->viatura.nomeCondutor,
             pclientRequest->pidCliente,
             pclientRequest->pidServidorDedicado);
    return naoHaMaisPedidos;
}

/**
 * @brief  s2_2_2_ProcuraLugarDisponivelBD Ler a descrição da tarefa S2.2.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 * @param  lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param  dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 * @param  pindexClienteBD (O) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void s2_2_2_ProcuraLugarDisponivelBD(Estacionamento clientRequest, Estacionamento *lugaresEstacionamento, int dimensaoMaximaParque, int *pindexClienteBD) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d], lugaresEstacionamento:%p, dimensaoMaximaParque:%d]",
             clientRequest.viatura.matricula,
             clientRequest.viatura.pais,
             clientRequest.viatura.categoria,
             clientRequest.viatura.nomeCondutor,
             clientRequest.pidCliente,
             clientRequest.pidServidorDedicado,
             lugaresEstacionamento,
             dimensaoMaximaParque);

    *pindexClienteBD = -1;

    for (int i = 0; i < dimensaoMaximaParque; i++) {
        if (lugaresEstacionamento[i].pidCliente == -1) {
            lugaresEstacionamento[i] = clientRequest;
            *pindexClienteBD = i;
            so_success("S2.2.2", "Reservei Lugar: %d", i);
            break;
        }
    }

    so_debug("> [*pindexClienteBD:%d]", *pindexClienteBD);
}

/**
 * @brief  s2_2_3_CriaServidorDedicado Ler a descrição da tarefa S2.2.3 no enunciado
 * @param  lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param  indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void s2_2_3_CriaServidorDedicado(Estacionamento *lugaresEstacionamento, int indexClienteBD) {
    so_debug("< [@param lugaresEstacionamento:%p, indexClienteBD:%d]", lugaresEstacionamento, indexClienteBD);

    pid_t pid = fork();

    if (pid < 0) {
        so_error("S2.2.3", "Erro ao criar processo Servidor Dedicado.");
        s4_EncerraServidor(FILE_REQUESTS);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        so_success("S2.2.3", "SD: Nasci com PID %d", getpid());
        sd7_MainServidorDedicado();
        exit(EXIT_SUCCESS);
    } else {
        if (indexClienteBD >= 0) {
            lugaresEstacionamento[indexClienteBD].pidServidorDedicado = pid;
        }
        so_success("S2.2.3", "Servidor: Iniciei SD %d", pid);
    }

    so_debug(">");
}

/**
 * @brief  s3_TrataCtrlC Ler a descrição da tarefa S3 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void s3_TrataCtrlC(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("S3", "Servidor: Start Shutdown");

    if (indexClienteBD >= 0 && lugaresEstacionamento[indexClienteBD].pidServidorDedicado > 0) {
        if (kill(lugaresEstacionamento[indexClienteBD].pidServidorDedicado, SIGUSR2) == -1) {
            so_error("S3", "Erro ao enviar SIGUSR2 a Servidor Dedicado");
        }
    }

    fflush(stdout);
    fflush(stderr);

    s4_EncerraServidor(FILE_REQUESTS);
    exit(EXIT_SUCCESS);

    so_debug(">");
}

/**
 * @brief  s4_EncerraServidor Ler a descrição da tarefa S4 no enunciado
 * @param  filenameFifoServidor (I) O nome do FIFO do servidor (i.e., FILE_REQUESTS)
 */
void s4_EncerraServidor(char *filenameFifoServidor) {
    so_debug("< [@param filenameFifoServidor:%s]", filenameFifoServidor);

    if (unlink(filenameFifoServidor) == -1) {
        so_error("S4", "Erro ao remover FIFO: %s", filenameFifoServidor);
    } else {
        so_success("S4", "Servidor: End Shutdown");
    }

    exit(EXIT_SUCCESS);

    so_debug(">");
}

/**
 * @brief  s5_TrataTerminouServidorDedicado Ler a descrição da tarefa S5 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void s5_TrataTerminouServidorDedicado(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, 0)) > 0) {
        so_success("S5", "(S5) Servidor: Confirmo que terminou o SD %d", (int)pid);
    }

    so_debug(">");
}

/**
 * @brief  sd7_MainServidorDedicado Ler a descrição da tarefa SD7 no enunciado
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
void sd7_MainServidorDedicado() {
    so_debug("<");

    sd7_1_ArmaSinaisServidorDedicado();
    sd7_2_ValidaPidCliente(clientRequest);
    sd7_3_ValidaLugarDisponivelBD(indexClienteBD);

    sd8_1_ValidaMatricula(clientRequest);
    sd8_2_ValidaPais(clientRequest);
    sd8_3_ValidaCategoria(clientRequest);
    sd8_4_ValidaNomeCondutor(clientRequest);

    sd9_1_AdormeceTempoRandom();
    sd9_2_EnviaSigusr1AoCliente(clientRequest);
    sd9_3_EscreveLogEntradaViatura(FILE_LOGFILE, clientRequest, &posicaoLogfile, &logItem);

    sd10_1_AguardaCheckout();
    sd10_2_EscreveLogSaidaViatura(FILE_LOGFILE, posicaoLogfile, logItem);

    sd11_EncerraServidorDedicado();

    so_error("Servidor Dedicado", "O programa nunca deveria ter chegado a este ponto!");
    so_debug(">");
}

/**
 * @brief  sd7_1_ArmaSinaisServidorDedicado Ler a descrição da tarefa SD7.1 no enunciado
 */
void sd7_1_ArmaSinaisServidorDedicado() {
    so_debug("<");

    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        so_error("SD7.1", "Falha ao armar SIGINT");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGCHLD, SIG_DFL) == SIG_ERR) {
        so_error("SD7.1", "Falha ao armar SIGCHLD");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGUSR2, sd12_TrataSigusr2) == SIG_ERR) {
        so_error("SD7.1", "Falha ao configurar handler para SIGUSR2");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGUSR1, sd13_TrataSigusr1) == SIG_ERR) {
        so_error("SD7.1", "Falha ao configurar handler para SIGUSR1");
        exit(EXIT_FAILURE);
    }

    so_success("SD7.1", "Sinais armados com sucesso");
    so_debug(">");
}

/**
 * @brief  sd7_2_ValidaPidCliente Ler a descrição da tarefa SD7.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd7_2_ValidaPidCliente(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]",
             clientRequest.viatura.matricula,
             clientRequest.viatura.pais,
             clientRequest.viatura.categoria,
             clientRequest.viatura.nomeCondutor,
             clientRequest.pidCliente,
             clientRequest.pidServidorDedicado);

    if (clientRequest.pidCliente <= 0) {
        so_error("SD7.2", "PID do cliente inválido [%d]", clientRequest.pidCliente);
        exit(EXIT_FAILURE);
    }

    so_success("SD7.2", "PID do cliente válido [%d]", clientRequest.pidCliente);
    so_debug(">");
}

/**
 * @brief  sd7_3_ValidaLugarDisponivelBD Ler a descrição da tarefa SD7.3 no enunciado
 * @param  indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void sd7_3_ValidaLugarDisponivelBD(int indexClienteBD) {
    so_debug("< [@param indexClienteBD:%d]", indexClienteBD);

    if (indexClienteBD < 0) {
        so_error("SD7.3", "Não foi encontrado lugar disponível na BD [indexClienteBD:%d]", indexClienteBD);
        if (kill(clientRequest.pidCliente, SIGHUP) == -1) {
            perror("kill");
            so_error("SD7.3", "Falha ao enviar SIGHUP ao cliente PID:%d", clientRequest.pidCliente);
        }
        exit(EXIT_FAILURE);
    }

    so_success("SD7.3", "Lugar disponível na BD [%d]", indexClienteBD);
    so_debug(">");
}

/**
 * @brief  sd8_1_ValidaMatricula Ler a descrição da tarefa SD8.1 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_1_ValidaMatricula(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]",
             clientRequest.viatura.matricula,
             clientRequest.viatura.pais,
             clientRequest.viatura.categoria,
             clientRequest.viatura.nomeCondutor,
             clientRequest.pidCliente,
             clientRequest.pidServidorDedicado);

    char *matricula = clientRequest.viatura.matricula;

    for (int i = 0; matricula[i] != '\0'; i++) {
        if (!(isupper(matricula[i]) || isdigit(matricula[i]))) {
            so_error("SD8.1", "Matrícula inválida [%s] - contém caracteres não permitidos", matricula);
            sd11_EncerraServidorDedicado();
            return;
        }
    }

    so_success("SD8.1", "Matrícula válida [%s]", matricula);
    so_debug(">");
}

/**
 * @brief  sd8_2_ValidaPais Ler a descrição da tarefa SD8.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_2_ValidaPais(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]",
             clientRequest.viatura.matricula,
             clientRequest.viatura.pais,
             clientRequest.viatura.categoria,
             clientRequest.viatura.nomeCondutor,
             clientRequest.pidCliente,
             clientRequest.pidServidorDedicado);

    char *pais = clientRequest.viatura.pais;

    if (strlen(pais) != 2) {
        so_error("SD8.2", "País inválido [%s] - deve ter exatamente 2 letras", pais);
        sd11_EncerraServidorDedicado();
        return;
    }
    
    if (!isupper(pais[0]) || !isupper(pais[1])) {
        so_error("SD8.2", "País inválido [%s] - deve conter apenas letras maiúsculas", pais);
        sd11_EncerraServidorDedicado();
        return;
    }

    so_success("SD8.2", "País válido [%s]", pais);
    so_debug(">");
}

/**
 * @brief  sd8_3_ValidaCategoria Ler a descrição da tarefa SD8.3 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_3_ValidaCategoria(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]",
             clientRequest.viatura.matricula,
             clientRequest.viatura.pais,
             clientRequest.viatura.categoria,
             clientRequest.viatura.nomeCondutor,
             clientRequest.pidCliente,
             clientRequest.pidServidorDedicado);

    char categoria = clientRequest.viatura.categoria;

    if (categoria != 'P' && categoria != 'L' && categoria != 'M') {
        so_error("SD8.3", "Categoria inválida [%c] - valores permitidos: P, L ou M", categoria);
        sd11_EncerraServidorDedicado();
        return;
    }

    so_success("SD8.3", "Categoria válida [%c]", categoria);
    so_debug(">");
}

/**
 * @brief  sd8_4_ValidaNomeCondutor Ler a descrição da tarefa SD8.4 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_4_ValidaNomeCondutor(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]",
             clientRequest.viatura.matricula,
             clientRequest.viatura.pais,
             clientRequest.viatura.categoria,
             clientRequest.viatura.nomeCondutor,
             clientRequest.pidCliente,
             clientRequest.pidServidorDedicado);

    char *nomeCondutor = clientRequest.viatura.nomeCondutor;

    if (nomeCondutor == NULL || nomeCondutor[0] == '\0') {
        so_error("SD8.4", "Nome do condutor não pode estar vazio");
        sd11_EncerraServidorDedicado();
        return;
    }

    FILE *passwd = fopen("/etc/passwd", "r");
    if (passwd == NULL) {
        so_error("SD8.4", "Não foi possível abrir /etc/passwd");
        sd11_EncerraServidorDedicado();
        return;
    }

    char linha[1024];
    int nomeEncontrado = 0;

    while (fgets(linha, sizeof(linha), passwd) != NULL) {
        char *campo = strtok(linha, ":");
        for (int i = 1; i <= 4 && campo != NULL; i++) {
            campo = strtok(NULL, ":");
        }

        if (campo != NULL) {
            char *nomeCompleto = strdup(campo);
            char *virgula = strchr(nomeCompleto, ',');
            if (virgula != NULL)
                *virgula = '\0';

            if (strcmp(nomeCompleto, nomeCondutor) == 0) {
                nomeEncontrado = 1;
                free(nomeCompleto);
                break;
            }
            free(nomeCompleto);
        }
    }

    fclose(passwd);

    if (!nomeEncontrado) {
        so_error("SD8.4", "Nome do condutor [%s] não encontrado em /etc/passwd", nomeCondutor);
        sd11_EncerraServidorDedicado();
        return;
    }

    so_success("SD8.4", "Nome do condutor válido [%s]", nomeCondutor);
    so_debug(">");
}

/**
 * @brief  sd9_1_AdormeceTempoRandom Ler a descrição da tarefa SD9.1 no enunciado
 */
void sd9_1_AdormeceTempoRandom() {
    so_debug("<");

    srand(time(NULL));
    int tempo = MAX_ESPERA;

    char msg[32];
    snprintf(msg, sizeof(msg), "%d", tempo);
    so_success("SD9.1", "%s", msg);

    sleep(tempo);
    so_debug(">");
}

/**
 * @brief  sd9_2_EnviaSigusr1AoCliente Ler a descrição da tarefa SD9.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd9_2_EnviaSigusr1AoCliente(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]",
             clientRequest.viatura.matricula,
             clientRequest.viatura.pais,
             clientRequest.viatura.categoria,
             clientRequest.viatura.nomeCondutor,
             clientRequest.pidCliente,
             clientRequest.pidServidorDedicado);

    if (kill(clientRequest.pidCliente, SIGUSR1) == -1) {
        so_error("SD9.2", "Erro ao enviar SIGUSR1 ao processo do cliente");
        sd11_EncerraServidorDedicado();
    }

    so_success("SD9.2", "SD: Confirmei Cliente Lugar %d", indexClienteBD);
    so_debug(">");
}

/**
 * @brief  sd9_3_EscreveLogEntradaViatura Ler a descrição da tarefa SD9.3 no enunciado
 * @param  logFilename (I) O nome do ficheiro de Logfile (i.e., FILE_LOGFILE)
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 * @param  pposicaoLogfile (O) posição do ficheiro Logfile mesmo antes de inserir o log desta viatura
 * @param  plogItem (O) registo de Log para esta viatura
 */
void sd9_3_EscreveLogEntradaViatura(char *logFilename, Estacionamento clientRequest, long *pposicaoLogfile, LogItem *plogItem) {
    so_debug("< [@param logFilename:%s, clientRequest:[%s:%s:%c:%s:%d:%d]]",
             logFilename, 
             clientRequest.viatura.matricula, 
             clientRequest.viatura.pais, 
             clientRequest.viatura.categoria, 
             clientRequest.viatura.nomeCondutor, 
             clientRequest.pidCliente, 
             clientRequest.pidServidorDedicado);

    FILE *fp = fopen(logFilename, "ab+");
    if (!fp) {
        so_error("SD9.3", "Erro ao abrir ficheiro de log: %s", logFilename);
        sd11_EncerraServidorDedicado();
        exit(EXIT_FAILURE);
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        so_error("SD9.3", "Erro posicionado no ficheiro de log");
        fclose(fp);
        sd11_EncerraServidorDedicado();
        exit(EXIT_FAILURE);
    }

    long pos = ftell(fp);
    if (pos == -1L) {
        so_error("SD9.3", "Erro ao obter posição atual no ficheiro de log");
        fclose(fp);
        sd11_EncerraServidorDedicado();
        exit(EXIT_FAILURE);
    }
    *pposicaoLogfile = pos;

    memset(plogItem, 0, sizeof(LogItem));

    strcpy(plogItem->viatura.matricula, clientRequest.viatura.matricula);
    strcpy(plogItem->viatura.pais, clientRequest.viatura.pais);
    plogItem->viatura.categoria = clientRequest.viatura.categoria;
    strcpy(plogItem->viatura.nomeCondutor, clientRequest.viatura.nomeCondutor);

    time_t agora = time(NULL);
    struct tm *tinfo = localtime(&agora);
    strftime(plogItem->dataEntrada, sizeof(plogItem->dataEntrada), "%Y-%m-%dT%Hh%M", tinfo);

    strcpy(plogItem->dataSaida, "---");

    if (fwrite(plogItem, sizeof(LogItem), 1, fp) != 1) {
        so_error("SD9.3", "Erro ao escrever no ficheiro de log");
        fclose(fp);
        sd11_EncerraServidorDedicado();
        exit(EXIT_FAILURE);
    }

    fclose(fp);

    so_success("SD9.3", "SD: Guardei log na posição %ld: Entrada Cliente %s em %s",
               *pposicaoLogfile,
               plogItem->viatura.matricula,
               plogItem->dataEntrada);

    so_debug("> [*pposicaoLogfile:%ld, *plogItem:[%s:%s:%c:%s:%s:%s]]",
             *pposicaoLogfile,
             plogItem->viatura.matricula,
             plogItem->viatura.pais,
             plogItem->viatura.categoria,
             plogItem->viatura.nomeCondutor,
             plogItem->dataEntrada,
             plogItem->dataSaida);
}

/**
 * @brief  sd10_1_AguardaCheckout Ler a descrição da tarefa SD10.1 no enunciado
 */
void sd10_1_AguardaCheckout() {
    so_debug("<");

    pause();

    const char* matricula = clientRequest.viatura.matricula;

    char mensagem[100];
    snprintf(mensagem, sizeof(mensagem), "SD: A viatura %s deseja sair do parque", matricula);
    so_success("SD10.1", "%s", mensagem);

    so_debug(">");
}

/**
 * @brief  sd10_2_EscreveLogSaidaViatura Ler a descrição da tarefa SD10.2 no enunciado
 * @param  logFilename (I) O nome do ficheiro de Logfile (i.e., FILE_LOGFILE)
 * @param  posicaoLogfile (I) posição do ficheiro Logfile mesmo antes de inserir o log desta viatura
 * @param  logItem (I) registo de Log para esta viatura
 */
void sd10_2_EscreveLogSaidaViatura(char *logFilename, long posicaoLogfile, LogItem logItem) {
    so_debug("< [@param logFilename:%s, posicaoLogfile:%ld, logItem:[%s:%s:%c:%s:%s:%s]]",
             logFilename, posicaoLogfile,
             logItem.viatura.matricula,
             logItem.viatura.pais,
             logItem.viatura.categoria,
             logItem.viatura.nomeCondutor,
             logItem.dataEntrada,
             logItem.dataSaida);

    time_t agora = time(NULL);
    struct tm *tm_info = localtime(&agora);
    if (strftime(logItem.dataSaida, sizeof(logItem.dataSaida), "%Y-%m-%dT%Hh%M", tm_info) == 0) {
        so_error("SD10.2", "SD: Falha ao formar timestamp de saída.");
        sd11_EncerraServidorDedicado();
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(logFilename, "r+b");
    if (fp == NULL) {
        so_error("SD10.2", "SD: Erro ao abrir ficheiro de log para escrita.");
        sd11_EncerraServidorDedicado();
        exit(EXIT_FAILURE);
    }

    if (fseek(fp, posicaoLogfile, SEEK_SET) != 0) {
        so_error("SD10.2", "SD: Erro ao posicionar no ficheiro de log.");
        fclose(fp);
        sd11_EncerraServidorDedicado();
        exit(EXIT_FAILURE);
    }

    if (fwrite(&logItem, sizeof(LogItem), 1, fp) != 1) {
        so_error("SD10.2", "SD: Erro ao escrever no ficheiro de log.");
        fclose(fp);
        sd11_EncerraServidorDedicado();
        exit(EXIT_FAILURE);
    }

    fclose(fp);

    char msg[200];
    snprintf(msg, sizeof(msg), "SD: Atualizei log na posição %ld: Saída Cliente %s em %s",
             posicaoLogfile, logItem.viatura.matricula, logItem.dataSaida);
    so_success("SD10.2", "%s", msg);

    sd11_EncerraServidorDedicado();
    so_debug(">");
}

/**
 * @brief  sd11_EncerraServidorDedicado Ler a descrição da tarefa SD11 no enunciado
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
void sd11_EncerraServidorDedicado() {
    so_debug("<");

    sd11_1_LibertaLugarViatura(lugaresEstacionamento, indexClienteBD);
    sd11_2_EnviaSighupAoClienteETermina(clientRequest);

    so_debug(">");
}

/**
 * @brief  sd11_1_LibertaLugarViatura Ler a descrição da tarefa SD11.1 no enunciado
 * @param  lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param  indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void sd11_1_LibertaLugarViatura(Estacionamento *lugaresEstacionamento, int indexClienteBD) {
    so_debug("< [@param lugaresEstacionamento:%p, indexClienteBD:%d]", lugaresEstacionamento, indexClienteBD);

    if (indexClienteBD < 0) {
        so_error("SD11.1", "Índice inválido");
        return;
    }

    lugaresEstacionamento[indexClienteBD].pidCliente = DISPONIVEL;
    so_success("SD11.1", "SD: Libertei Lugar: %d", indexClienteBD);

    so_debug(">");
}

/**
 * @brief  sd11_2_EnviaSighupAoClienteETermina Ler a descrição da tarefa SD11.2 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd11_2_EnviaSighupAoClienteETermina(Estacionamento clientRequest) {
    so_debug("< [@param clientRequest:[%s:%s:%c:%s:%d:%d]]",
             clientRequest.viatura.matricula,
             clientRequest.viatura.pais,
             clientRequest.viatura.categoria,
             clientRequest.viatura.nomeCondutor,
             clientRequest.pidCliente,
             clientRequest.pidServidorDedicado);

    if (kill(clientRequest.pidCliente, SIGHUP) == -1) {
        so_error("SD11.2", "Erro ao enviar SIGHUP ao cliente (PID %d).", clientRequest.pidCliente);
    }

    so_success("SD11.2", "SD: Shutdown");
    exit(0);

    so_debug(">");
}

/**
 * @brief  sd12_TrataSigusr2 Ler a descrição da tarefa SD12 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void sd12_TrataSigusr2(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("SD12", "SD: Recebi pedido do Servidor para terminar");

    sd11_EncerraServidorDedicado();

    so_debug(">");
}

/**
 * @brief  sd13_TrataSigusr1 Ler a descrição da tarefa SD13 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void sd13_TrataSigusr1(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("SD13", "SD: Recebi pedido do Cliente para terminar o estacionamento");

    so_debug(">");
}
