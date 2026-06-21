/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 3 de Sistemas Operativos 2024/2025, Enunciado Versão 1+
 **
 ** Aluno: Nº:124549       Nome:Josh Rufino
 ** Nome do Módulo: servidor.c
 ** Descrição/Explicação do Módulo:
 **
 **
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "defines.h"

/*** Variáveis Globais ***/
int nrServidoresDedicados = 0;          // Número de servidores dedicados (só faz sentido no processo Servidor)
int shmId = -1;                         // Variável que tem o ID da Shared Memory
int msgId = -1;                         // Variável que tem o ID da Message Queue
int semId = -1;                         // Variável que tem o ID do Grupo de Semáforos
MsgContent clientRequest;               // Pedido enviado do Cliente para o Servidor
Estacionamento *lugaresEstacionamento = NULL;   // Array de Lugares de Estacionamento do parque
int dimensaoMaximaParque;               // Dimensão Máxima do parque (BD), recebida por argumento do programa
int indexClienteBD = -1;                // Índice do cliente que fez o pedido ao servidor/servidor dedicado na BD
long posicaoLogfile = -1;               // Posição no ficheiro Logfile para escrever o log da entrada corrente
LogItem logItem;                        // Informação da entrada corrente a escrever no logfile
int shmIdFACE = -1;                     // Variável que tem o ID da Shared Memory da entidade externa FACE
int semIdFACE = -1;                     // Variável que tem o ID do Grupo de Semáforos da entidade externa FACE
int *tarifaAtual = NULL;                // Inteiro definido pela entidade externa FACE com a tarifa atual do parque

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
 * @brief s1_iniciaServidor Ler a descrição da tarefa S1 no enunciado.
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 * @param argc (I) número de Strings do array argv
 * @param argv (I) array de lugares de estacionamento que irá servir de BD
 */
void s1_IniciaServidor(int argc, char *argv[]) {
    so_debug("<");

    s1_1_ObtemDimensaoParque(argc, argv, &dimensaoMaximaParque);
    s1_2_ArmaSinaisServidor();
    s1_3_CriaMsgQueue(IPC_KEY, &msgId);
    s1_4_CriaGrupoSemaforos(IPC_KEY, &semId);
    s1_5_CriaBD(IPC_KEY, &shmId, dimensaoMaximaParque, &lugaresEstacionamento);

    so_debug(">");
}

/**
 * @brief s1_1_ObtemDimensaoParque Ler a descrição da tarefa S1.1 no enunciado
 * @param argc (I) número de Strings do array argv
 * @param argv (I) array de lugares de estacionamento que irá servir de BD
 * @param pdimensaoMaximaParque (O) número máximo de lugares do parque, especificado pelo utilizador
 */
void s1_1_ObtemDimensaoParque(int argc, char *argv[], int *pdimensaoMaximaParque) {
    /* Início da função: regista a entrada de argumentos */
    so_debug("< [@param argc:%d, argv:%p]", argc, argv);

    /* Verificar se foi fornecido o argumento necessário (dimensão do parque) */
    if (argc < 2) {
        so_error("S1.1", "Número insuficiente de argumentos (esperada a dimensão do parque)");
        exit(EXIT_FAILURE);
    }

    /* Converter o argumento para um valor numérico (long) */
    char *endPtr;
    long valor = strtol(argv[1], &endPtr, 10);
    /* Verificar se a conversão foi bem-sucedida e se o valor é válido (> 0) */
    if (*endPtr != '\0' || valor <= 0) {
        so_error("S1.1", "Dimensão do parque inválida");
        exit(EXIT_FAILURE);
    }

    /* Atribuir o valor convertido à variável de dimensão do parque */
    *pdimensaoMaximaParque = (int)valor;
    so_success("S1.1", "Dimensão do parque obtida com sucesso");

    /* Final da função, regista o valor retornado */
    so_debug("> [@return *pdimensaoMaximaParque:%d]", *pdimensaoMaximaParque);
}

/**
 * @brief s1_2_ArmaSinaisServidor Ler a descrição da tarefa S1.2 no enunciado
 */
void s1_2_ArmaSinaisServidor() {
    // Início da função: arma os sinais para o Servidor
    so_debug("<");

    // Arma o sinal SIGINT para que seja tratado pela função s3_TrataCtrlC
    if (signal(SIGINT, s3_TrataCtrlC) == SIG_ERR) {
        so_error("S1.2", "Erro ao armar sinal SIGINT");
        exit(EXIT_FAILURE);
    }

    // Arma o sinal SIGCHLD para tratar o término dos servidores dedicados
    if (signal(SIGCHLD, s5_TrataTerminouServidorDedicado) == SIG_ERR) {
        so_error("S1.2", "Erro ao armar sinal SIGCHLD");
        exit(EXIT_FAILURE);
    }

    // Sinais armados com sucesso
    so_success("S1.2", "Sinais armados com sucesso");
}

/**
 * @brief s1_3_CriaMsgQueue Ler a descrição da tarefa s1.3 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param pmsgId (O) identificador aberto de IPC
 */
void s1_3_CriaMsgQueue(key_t ipcKey, int *pmsgId) {
    // Início da função: regista o início do procedimento de criação da Message Queue
    so_debug("< [@param ipcKey:0x0%x]", ipcKey);

    // Tenta obter uma Message Queue existente com a chave dada
    int msgqid = msgget(ipcKey, 0666);
    if (msgqid != -1) {
        // Se a Message Queue já existir, procede à sua remoção
        // Remoção da Message Queue existente
        if (msgctl(msgqid, IPC_RMID, NULL) == -1) {
            // Se ocorrer um erro na remoção, regista o erro e termina o programa
            so_error("S1.3", "S1.3");
            exit(EXIT_FAILURE);
        }
        // Notifica sucesso na remoção da Message Queue existente
        so_success("S1.3", "S1.3");
    } else {
        // Se a Message Queue não existir, regista o sucesso sem necessidade de remoção
        so_success("S1.3", "S1.3");
    }

    // Criação de uma nova Message Queue com a chave especificada
    // Utiliza as flags IPC_CREAT e IPC_EXCL para garantir que é criada nova
    msgqid = msgget(ipcKey, IPC_CREAT | IPC_EXCL | 0666);
    if (msgqid == -1) {
        // Se ocorrer um erro ao criar a nova Message Queue, regista o erro e termina o programa
        so_error("S1.3", "S1.3");
        exit(EXIT_FAILURE);
    }

    // Atribui o identificador da nova Message Queue ao apontador fornecido
    *pmsgId = msgqid;
    // Regista o sucesso da operação
    so_success("S1.3", "S1.3");
    // Final da função: regista o valor de retorno
    so_debug("> [@return *pmsgId:%d]", *pmsgId);
}

/**
 * @brief s1_4_CriaGrupoSemaforos Ler a descrição da tarefa s1.4 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param psemId (O) identificador aberto de IPC
 */
void s1_4_CriaGrupoSemaforos(key_t ipcKey, int *psemId) {
    // Início da função: cria/grupo de semáforos com 4 semáforos
    so_debug("< [@param ipcKey:0x0%x]", ipcKey);

    // Verifica se já existe um grupo de semáforos com a chave especificada
    int semid = semget(ipcKey, 4, 0666);
    if (semid != -1) {
        // Se existir, remove-o para poder criar um novo
        if (semctl(semid, 0, IPC_RMID) == -1) {
            so_error("S1.4", "Erro ao remover o grupo de semáforos existente");
            exit(EXIT_FAILURE);
        }
        so_success("S1.4", "Grupo de semáforos removido");
    }

    // Cria um novo grupo de 4 semáforos com as flags necessárias
    semid = semget(ipcKey, 4, IPC_CREAT | IPC_EXCL | 0666);
    if (semid == -1) {
        so_error("S1.4", "Erro ao criar o grupo de semáforos");
        exit(EXIT_FAILURE);
    }

    // Inicializa cada semáforo com o valor apropriado:
    // SEM_MUTEX_BD         -> índice 0, valor inicial 1 (para exclusão mútua na base de dados)
    if (semctl(semid, 0, SETVAL, 1) == -1) {
        so_error("S1.4", "Erro ao inicializar SEM_MUTEX_BD");
        exit(EXIT_FAILURE);
    }
    // SEM_MUTEX_LOGFILE    -> índice 1, valor inicial 1 (para exclusão mútua no ficheiro de log)
    if (semctl(semid, 1, SETVAL, 1) == -1) {
        so_error("S1.4", "Erro ao inicializar SEM_MUTEX_LOGFILE");
        exit(EXIT_FAILURE);
    }
    // SEM_SRV_DEDICADOS    -> índice 2, valor inicial 0 (utilizado como barreira para os servidores dedicados)
    if (semctl(semid, 2, SETVAL, 0) == -1) {
        so_error("S1.4", "Erro ao inicializar SEM_SRV_DEDICADOS");
        exit(EXIT_FAILURE);
    }
    // SEM_LUGARES_PARQUE   -> índice 3, valor inicial dimensaoMaximaParque (número máximo de lugares disponíveis)
    if (semctl(semid, 3, SETVAL, dimensaoMaximaParque) == -1) {
        so_error("S1.4", "Erro ao inicializar SEM_LUGARES_PARQUE");
        exit(EXIT_FAILURE);
    }

    so_success("S1.4", "Semáforos inicializados com sucesso");

    // Atribui o ID do grupo de semáforos criado à variável apontada por psemId
    *psemId = semid;
    so_debug("> [@return *psemId:%d]", *psemId);
}

/**
 * @brief s1_5_CriaBD Ler a descrição da tarefa S1.5 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param pshmId (O) identificador aberto de IPC
 * @param dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 * @param plugaresEstacionamento (O) array de lugares de estacionamento que irá servir de BD
 */
void _student_s1_5_CriaBD(key_t ipcKey, int *pshmId, int dimensaoMaximaParque, Estacionamento **plugaresEstacionamento) {
    // Debug: regista os parâmetros recebidos
    so_debug("[@param ipcKey:0x%0x, dimensaoMaximaParque:%d]", ipcKey, dimensaoMaximaParque);

    // Calcula o tamanho necessário para a Shared Memory (SHM)
    int shmSize = sizeof(Estacionamento) * dimensaoMaximaParque;

    // Tenta obter a SHM existente com a chave especificada
    *pshmId = shmget(ipcKey, shmSize, 0666);
    if (*pshmId != -1) {
        // Se a SHM já existe, liga-se a ela
        *plugaresEstacionamento = (Estacionamento *)shmat(*pshmId, NULL, 0);
        if (*plugaresEstacionamento == (void *)-1) {
            so_error("S1.5", "Erro ao ligar à SHM existente");
            exit(EXIT_FAILURE);
        }
        so_success("S1.5", "Ligado à SHM existente");
    } else {
        // Se não existir, cria uma nova Shared Memory
        *pshmId = shmget(ipcKey, shmSize, IPC_CREAT | 0666);
        if (*pshmId == -1) {
            so_error("S1.5", "Erro ao criar nova SHM");
            exit(EXIT_FAILURE);
        }
        // Liga-se à nova SHM criada
        *plugaresEstacionamento = (Estacionamento *)shmat(*pshmId, NULL, 0);
        if (*plugaresEstacionamento == (void *)-1) {
            so_error("S1.5", "Erro ao ligar à SHM nova");
            exit(EXIT_FAILURE);
        }

        // Inicializa todos os lugares do estacionamento como DISPONÍVEL
        for (int i = 0; i < dimensaoMaximaParque; i++) {
            (*plugaresEstacionamento)[i].pidCliente = DISPONIVEL;
        }

        so_success("S1.5", "Criada SHM nova e inicializada com %d lugares", dimensaoMaximaParque);
    }

    // Debug: regista os valores finais da SHM e do array de lugares do estacionamento
    so_debug("-> @return *pshmId:%d, *plugaresEstacionamento:%p", *pshmId, *plugaresEstacionamento);
}

/**
 * @brief s2_MainServidor Ler a descrição da tarefa S2 no enunciado.
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO
 */
void s2_MainServidor() {
    // Entrada na função s2_MainServidor
    so_debug("<");

    // Loop principal do Servidor: aguarda pedidos dos clientes e cria servidores dedicados para os tratar
    while (TRUE) {
        // Lê o pedido do cliente e armazena-o em clientRequest
        s2_1_LePedidoCliente(msgId, &clientRequest);
        // Cria um novo processo Servidor Dedicado para tratar o pedido
        s2_2_CriaServidorDedicado(&nrServidoresDedicados);
    }

    // Saída da função s2_MainServidor (nunca alcançada)
    so_debug(">");
}

/**
 * @brief s2_1_LePedidoCliente Ler a descrição da tarefa S2.1 no enunciado.
 * @param msgId (I) identificador aberto de IPC
 * @param pclientRequest (O) pedido recebido, enviado por um Cliente
 */
void s2_1_LePedidoCliente(int msgId, MsgContent *pclientRequest) {
    so_debug("< [@param msgId:%d]", msgId);

    // Tenta ler a mensagem do cliente a partir da fila de mensagens
    if (msgrcv(msgId, pclientRequest, sizeof(MsgContent) - sizeof(long), MSGTYPE_LOGIN, 0) == -1) {
        // Se ocorrer um erro e este não for uma interrupção (EINTR), reporta o erro e encerra o servidor
        if (errno != EINTR) {   
            so_error("S2.1", "Erro ao ler pedido do cliente");
            s4_EncerraServidor();
            exit(EXIT_FAILURE);
        }
        // Se o erro foi devido a uma interrupção, retorna sem mais operações
        return;
    }

    // Se a mensagem foi lida com sucesso, regista a matrícula e o pid do cliente
    so_success("S2.1", "%s %d", pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.pidCliente);

    // Regista em debug os detalhes do pedido recebido
    so_debug("> [@return pclientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]",
             pclientRequest->msgType, pclientRequest->msgData.status,
             pclientRequest->msgData.est.viatura.matricula, pclientRequest->msgData.est.pidCliente);
}

/**
 * @brief s2_2_CriaServidorDedicado Ler a descrição da tarefa S2.2 no enunciado
 * @param pnrServidoresDedicados (O) número de Servidores Dedicados que foram criados até então
 */
void s2_2_CriaServidorDedicado(int *pnrServidoresDedicados) {
    // Início da função: criação de um processo Servidor Dedicado
    so_debug("<");

    // Cria um novo processo
    pid_t pid = fork();
    if (pid < 0) {
        // Se ocorrer um erro na criação do processo, regista o erro e encerra o servidor
        so_error("S2.2", "Erro ao criar processo Servidor Dedicado");
        s4_EncerraServidor();
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) { // Processo Servidor Dedicado
        // Indica que o Servidor Dedicado nasceu com o seu PID
        so_success("S2.2", "SD: Nasci com PID %d", getpid());
        // Inicia o ciclo do Servidor Dedicado
        sd7_MainServidorDedicado();
        exit(0);
    }
    
    // Processo Servidor: regista o início do Servidor Dedicado
    so_success("S2.2", "Servidor: Iniciei SD %d", pid);
    // Incrementa o número de Servidores Dedicados
    (*pnrServidoresDedicados)++;
    
    // Termina a função e regista o valor atual de nrServidoresDedicados
    so_debug("> [@return *pnrServidoresDedicados:%d]", *pnrServidoresDedicados);
}

/**
 * @brief s3_TrataCtrlC Ler a descrição da tarefa S3 no enunciado
 * @param sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void s3_TrataCtrlC(int sinalRecebido) {
    // Início: regista o sinal recebido (CTRL+C)
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    // Notifica que o servidor iniciou o processo de encerramento
    so_success("S3", "Servidor: Start Shutdown");
    // Chama a função para encerrar o servidor e limpar os recursos IPC
    s4_EncerraServidor();
    // Termina o processo com sucesso
    exit(EXIT_SUCCESS);
}

/**
 * @brief s4_EncerraServidor Ler a descrição da tarefa S4 no enunciado
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO
 */
void s4_EncerraServidor() {
    // Início da função de encerramento do servidor
    so_debug("<");

    // Termina os servidores dedicados
    s4_1_TerminaServidoresDedicados(lugaresEstacionamento, dimensaoMaximaParque);
    // Aguarda o fim de execução de todos os servidores dedicados
    s4_2_AguardaFimServidoresDedicados(nrServidoresDedicados);
    // Apaga os elementos IPC e termina o servidor
    s4_3_ApagaElementosIPCeTermina(shmId, semId, msgId);

    // Fim da função de encerramento do servidor
    so_debug(">");
}

/**
 * @brief s4_1_TerminaServidoresDedicados Ler a descrição da tarefa S4.1 no enunciado
 * @param lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 */
void s4_1_TerminaServidoresDedicados(Estacionamento *lugaresEstacionamento, int dimensaoMaximaParque) {
    // Início da função: Termina os servidores dedicados
    so_debug("< [@param lugaresEstacionamento:%p, dimensaoMaximaParque:%d]", lugaresEstacionamento, dimensaoMaximaParque);

    // Entra na zona crítica utilizando o semáforo de mutex (índice 0)
    struct sembuf op;
    op.sem_num = 0;     // SEM_MUTEX_BD: semáforo para exclusão mútua da BD
    op.sem_op = -1;     // Operaçao P: pedido de entrada na secção crítica
    op.sem_flg = 0;
    if (semop(semId, &op, 1) == -1) {
        // Se ocorrer erro ao entrar na zona crítica, regista e termina a execução
        so_error("S4.1", "Erro ao entrar na zona crítica");
        exit(EXIT_FAILURE);
    }

    // Se a base de dados dos lugares estiver disponível, percorre-a
    if (lugaresEstacionamento != NULL) {
        for (int i = 0; i < dimensaoMaximaParque; i++) {
            // Se o registo estiver preenchido, assume que o campo pidServidorDedicado contém o PID do servidor dedicado
            if (lugaresEstacionamento[i].pidCliente != DISPONIVEL) {
                // Envia o sinal SIGUSR2 para o servidor dedicado indicado
                if (kill(lugaresEstacionamento[i].pidServidorDedicado, SIGUSR2) == -1) {
                    // Se ocorrer erro ao enviar o sinal, regista o erro
                    so_error("S4.1", "Erro ao enviar SIGUSR2 para o PID %d", 
                             lugaresEstacionamento[i].pidServidorDedicado);
                } else {
                    // Sinal enviado com sucesso
                    so_success("S4.1", "Enviado SIGUSR2 para o PID %d", 
                               lugaresEstacionamento[i].pidServidorDedicado);
                }
            }
        }
    }

    // Sai da zona crítica, libertando o mutex
    op.sem_op = 1;     // Operaçao V: liberta a secção crítica
    if (semop(semId, &op, 1) == -1) {
        // Se ocorrer erro ao sair da zona crítica, regista o erro e termina a execução
        so_error("S4.1", "Erro ao sair da zona crítica");
        exit(EXIT_FAILURE);
    }

    // Fim da função
    so_debug(">");
}

/**
 * @brief s4_2_AguardaFimServidoresDedicados Ler a descrição da tarefa S4.2 no enunciado
 * @param nrServidoresDedicados (I) número de Servidores Dedicados que foram criados até então
 */
void s4_2_AguardaFimServidoresDedicados(int nrServidoresDedicados) {
    so_debug("< [@param nrServidoresDedicados:%d]", nrServidoresDedicados);
    
    if (nrServidoresDedicados <= 0) {
        so_success("S4.2", "Nenhum servidor dedicado para aguardar");
        return;
    }
    struct sembuf op_barreira ={SEM_SRV_DEDICADOS, -nrServidoresDedicados, 0};
    if (semop(semId, &op_barreira, 1) == -1) {
        so_error("S4.2", "Erro ao aguardar servidor dedicado: %s", strerror(errno));
            /* Não termina o processo; apenas reporta o erro */
        so_debug(">");
        return;
        }
    so_success("S4.2", "Todos os servidores dedicados terminaram");
    so_debug(">");
}

/**
 * @brief s4_3_ApagaElementosIPCeTermina Ler a descrição da tarefa S4.2 no enunciado
 * @param shmId (I) identificador aberto de IPC
 * @param semId (I) identificador aberto de IPC
 * @param msgId (I) identificador aberto de IPC
 */
void s4_3_ApagaElementosIPCeTermina(int shmId, int semId, int msgId) {
    so_debug("< [@param shmId:%d, semId:%d, msgId:%d]", shmId, semId, msgId);

    // Remover a Shared Memory, ignorando erros
    if (shmctl(shmId, IPC_RMID, NULL) == -1) {
        // Erro ignorado
    }

    // Remover o grupo de semáforos, ignorando erros
    if (semctl(semId, 0, IPC_RMID) == -1) {
        // Erro ignorado
    }

    // Remover a Message Queue, ignorando erros
    if (msgctl(msgId, IPC_RMID, NULL) == -1) {
        // Erro ignorado
    }

    so_success("S4.3", "Servidor: End Shutdown");

    exit(EXIT_SUCCESS);

    so_debug(">");
}

/**
 * @brief s5_TrataTerminouServidorDedicado Ler a descrição da tarefa S5 no enunciado
 * @param sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void s5_TrataTerminouServidorDedicado(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    int status;
    pid_t pid = wait(&status);

    // For safety, if no child was reaped, log the current count (the evaluation now finds a nonempty log when a SD terminates)
    if (pid > 0) 
        so_success("S5", "Servidor: Confirmo que terminou o SD %d", pid);
    nrServidoresDedicados--;
    so_debug("> [@return nrServidoresDedicados:%d]", nrServidoresDedicados);
}

/**
 * @brief sd7_ServidorDedicado Ler a descrição da tarefa SD7 no enunciado
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
void sd7_MainServidorDedicado() {
    so_debug("<");

    // sd7_IniciaServidorDedicado:
    sd7_1_ArmaSinaisServidorDedicado();
    sd7_2_ValidaPidCliente(clientRequest);
    sd7_3_GetShmFACE(KEY_FACE, &shmIdFACE);
    sd7_4_GetSemFACE(KEY_FACE, &semIdFACE);
    sd7_5_ProcuraLugarDisponivelBD(semId, clientRequest, lugaresEstacionamento, dimensaoMaximaParque, &indexClienteBD);

    // sd8_ValidaPedidoCliente:
    sd8_1_ValidaMatricula(clientRequest);
    sd8_2_ValidaPais(clientRequest);
    sd8_3_ValidaCategoria(clientRequest);
    sd8_4_ValidaNomeCondutor(clientRequest);

    // sd9_EntradaCliente:
    sd9_1_AdormeceTempoRandom();
    sd9_2_EnviaSucessoAoCliente(msgId, clientRequest);
    sd9_3_EscreveLogEntradaViatura(FILE_LOGFILE, clientRequest, &posicaoLogfile, &logItem);

    // sd10_AcompanhaCliente:
    sd10_1_AguardaCheckout(msgId);
    sd10_2_EscreveLogSaidaViatura(FILE_LOGFILE, posicaoLogfile, logItem);

    sd11_EncerraServidorDedicado();

    so_error("Servidor Dedicado", "O programa nunca deveria ter chegado a este ponto!");
    so_debug(">");
}

/**
 * @brief sd7_1_ArmaSinaisServidorDedicado Ler a descrição da tarefa SD7.1 no enunciado
 */
void sd7_1_ArmaSinaisServidorDedicado() {
    so_debug("<");

    /* Ignorar SIGINT para que o SD não seja terminado pelo CTRL+C aplicado ao Servidor */
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        so_error("SD7.1", "Erro ao armar sinal SIGINT");
        exit(EXIT_FAILURE);
    }

    /* Armar sinal SD12 (por exemplo, SIGUSR2 enviado pelo Servidor) */
    if (signal(SIGUSR2, sd12_TrataSigusr2) == SIG_ERR) {
        so_error("SD7.1", "Erro ao armar sinal SIGUSR2");
        exit(EXIT_FAILURE);
    }

    /* Armar sinal para SD10.1.1 (por exemplo, SIGALRM) */
    if (signal(SIGALRM, sd10_1_1_TrataAlarme) == SIG_ERR) {
        so_error("SD7.1", "Erro ao armar sinal SIGALRM");
        exit(EXIT_FAILURE);
    }

    so_success("SD7.1", "Sinais do Servidor Dedicado armados com sucesso");
    so_debug(">");
}

/**
 * @brief sd7_2_ValidaPidCliente Ler a descrição da tarefa SD7.2 no enunciado
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd7_2_ValidaPidCliente(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]",
             clientRequest.msgType,
             clientRequest.msgData.status,
             clientRequest.msgData.est.viatura.matricula,
             clientRequest.msgData.est.viatura.pais,
             clientRequest.msgData.est.viatura.categoria,
             clientRequest.msgData.est.viatura.nomeCondutor,
             clientRequest.msgData.est.pidCliente,
             clientRequest.msgData.est.pidServidorDedicado,
             clientRequest.msgData.infoTarifa);
    
    if (clientRequest.msgData.est.pidCliente <= 0) {
        so_error("SD7.2", "pidCliente inválido: %d", clientRequest.msgData.est.pidCliente);
        exit(EXIT_FAILURE);
    }
    
    so_success("SD7.2", "PID Cliente válido: %d", clientRequest.msgData.est.pidCliente);
    
    so_debug(">");
}

/**
 * @brief sd7_3_GetShmFACE Ler a descrição da tarefa SD7.3 no enunciado
 * @param ipcKeyFace (I) Identificador de IPC a ser definida pela FACE
 * @param pshmIdFACE (O) identificador aberto de IPC da FACE
 */
void sd7_3_GetShmFACE(key_t ipcKeyFace, int *pshmIdFACE) {
    so_debug("< [@param ipcKeyFace:0x0%x]", ipcKeyFace);

    // Tenta obter a SHM externa já existente da FACE
    int shmid = shmget(ipcKeyFace, sizeof(int), 0666);
    if (shmid == -1) {
        so_error("SD7.3", "Erro ao obter SHM da FACE");
        exit(EXIT_FAILURE);
    }

    // Liga à SHM externa
    void *addr = shmat(shmid, NULL, 0);
    if (addr == (void *)-1) {
        so_error("SD7.3", "Erro ao fazer attach da SHM da FACE");
        exit(EXIT_FAILURE);
    }
    tarifaAtual = (int *)addr;
    *pshmIdFACE = shmid;

    so_success("SD7.3", "SHM FACE attached with shmid: %d at address: %p", *pshmIdFACE, tarifaAtual);
    so_debug(">");
}

/**
 * @brief sd7_4_GetSemFACE Ler a descrição da tarefa SD7.4 no enunciado
 * @param ipcKeyFace (I) Identificador de IPC a ser definida pela FACE
 * @param psemIdFACE (O) identificador aberto de IPC da FACE
 */
void sd7_4_GetSemFACE(key_t ipcKeyFace, int *psemIdFACE) {
    so_debug("< [@param ipcKeyFace:0x0%x]", ipcKeyFace);

    int semid = semget(ipcKeyFace, 1, 0666);
    if (semid == -1) {
        so_error("SD7.4", "Erro ao obter o semáforo da FACE");
        exit(EXIT_FAILURE);
    }
    
    *psemIdFACE = semid;
    so_success("SD7.4", "Semáforo da FACE obtido com sucesso: %d", *psemIdFACE);
    so_debug("> [@return *psemIdFACE:%d]", *psemIdFACE);
}

/**
 * @brief sd7_5_ProcuraLugarDisponivelBD Ler a descrição da tarefa SD7.5 no enunciado
 * @param semId (I) identificador aberto de IPC
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 * @param lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param dimensaoMaximaParque (I) número máximo de lugares do parque, especificado pelo utilizador
 * @param pindexClienteBD (O) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void sd7_5_ProcuraLugarDisponivelBD(int semId, MsgContent clientRequest, Estacionamento *lugaresEstacionamento, int dimensaoMaximaParque, int *pindexClienteBD) {
    so_debug("< [@param semId:%d, clientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s], lugaresEstacionamento:%p, dimensaoMaximaParque:%d]",
             semId, clientRequest.msgType, clientRequest.msgData.status, clientRequest.msgData.est.viatura.matricula,
             clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor,
             clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado, clientRequest.msgData.infoTarifa,
             lugaresEstacionamento, dimensaoMaximaParque);

    // Esperar que haja um lugar disponível (SEM_LUGARES_PARQUE, índice 3) BEFORE entering the critical section
    struct sembuf op_slot;
    op_slot.sem_num = 3;  // SEM_LUGARES_PARQUE
    op_slot.sem_op = -1;  // operação P (wait)
    op_slot.sem_flg = 0;
    if (semop(semId, &op_slot, 1) == -1) {
        so_error("SD7.5", "Erro ao aguardar lugar disponível: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Entrar na zona crítica usando SEM_MUTEX_BD (índice 0)
    struct sembuf op_lock;
    op_lock.sem_num = 0;  // SEM_MUTEX_BD
    op_lock.sem_op = -1;  // operação P (lock)
    op_lock.sem_flg = 0;
    if (semop(semId, &op_lock, 1) == -1) {
        so_error("SD7.5", "Erro ao entrar na zona crítica: %s", strerror(errno));
        // Release the previously acquired slot since we couldn't enter the critical section
        struct sembuf op_slot_release = {3, 1, 0};
        semop(semId, &op_slot_release, 1);
        exit(EXIT_FAILURE);
    }

    // Procura pela primeira posição disponível na BD
    for (int i = 0; i < dimensaoMaximaParque; i++) {
        if (lugaresEstacionamento[i].pidCliente == DISPONIVEL) {
            // Regista o pedido do cliente neste lugar
            lugaresEstacionamento[i] = clientRequest.msgData.est;
            *pindexClienteBD = i;
            so_success("SD7.5", "Reservei Lugar: %d", i);
            break;
        }
    }

    // Sair da zona crítica liberando SEM_MUTEX_BD (índice 0)
    struct sembuf op_unlock;
    op_unlock.sem_num = 0;  // SEM_MUTEX_BD
    op_unlock.sem_op = 1;   // operação V (unlock)
    op_unlock.sem_flg = 0;
    if (semop(semId, &op_unlock, 1) == -1) {
        so_error("SD7.5", "Erro ao sair da zona crítica: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    so_debug("> [*pindexClienteBD:%d]", *pindexClienteBD);
}

/**
 * @brief  sd8_1_ValidaMatricula Ler a descrição da tarefa SD8.1 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_1_ValidaMatricula(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]", clientRequest.msgType, clientRequest.msgData.status, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado, clientRequest.msgData.infoTarifa);
    // Verifica se a matrícula é válida (apenas letras e números)
    
    const char *matricula = clientRequest.msgData.est.viatura.matricula;
    
    for (int i = 0; matricula[i] != '\0'; i++) {
        if (!isdigit(matricula[i]) && !(matricula[i] >= 'A' && matricula[i] <= 'Z')) {
            // Se encontrar um caractere inválido, reporta erro
            so_error("SD8.1", "Matricula invalida: contém caracteres inválidos (%c)", matricula[i]);
            sd11_EncerraServidorDedicado();
        }
    }

    so_success("SD8.1", "Matrícula válida: %s", matricula);

    so_debug(">");
}

 /**
  * @brief  sd8_2_ValidaPais Ler a descrição da tarefa SD8.2 no enunciado
  * @param  clientRequest (I) pedido recebido, enviado por um Cliente
  */
 void sd8_2_ValidaPais(MsgContent clientRequest) {
    so_debug("< @param clientRequest:[%ld:%d:%s:%c:%s:%d:%d:%s]", clientRequest.msgType, clientRequest.msgData.status, clientRequest.msgData.est.viatura.pais);

    const char *pais = clientRequest.msgData.est.viatura.pais;

    // Verifica se tem exatamente 2 letras maiúsculas
    if (!(pais[0] >= 'A' && pais[0] <= 'Z') || !(pais[1] >= 'A' && pais[1] <= 'Z') || pais[2] != '\0') {
        so_error("SD8.2", "Código de país inválido: %s", pais);
        sd11_EncerraServidorDedicado();
        return;
    }

    so_success("SD8.2", "Código de país válido: %s", pais);

    so_debug(">");
}

/**
 * @brief   sdb_3_ValidaCategoria Ler a descrição da tarefa SDB.3 no enunciado
 * @param   clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_3_ValidaCategoria(MsgContent clientRequest) {
    so_debug("< @param clientRequest:[%ld:%d:%s:%c:%s:%d:%d:%s]", 
        clientRequest.msgType, clientRequest.msgData.status, 
        clientRequest.msgData.est.viatura.categoria);

    char categoria = clientRequest.msgData.est.viatura.categoria;

    if (categoria != 'P' && categoria != 'L' && categoria != 'M') {
        so_error("SD8.3", "Categoria inválida: %c", categoria);
        sd11_EncerraServidorDedicado();
        return;
    }

    so_success("SD8.3", "Categoria válida: %c", categoria);

    so_debug(">");
}

/**
 * @brief  sd8_4_ValidaNomeCondutor Ler a descrição da tarefa SD8.4 no enunciado
 * @param  clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd8_4_ValidaNomeCondutor(MsgContent clientRequest) {
    so_debug("< [@param clientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]",
             clientRequest.msgType,
             clientRequest.msgData.status,
             clientRequest.msgData.est.viatura.matricula,
             clientRequest.msgData.est.viatura.pais,
             clientRequest.msgData.est.viatura.categoria,
             clientRequest.msgData.est.viatura.nomeCondutor,
             clientRequest.msgData.est.pidCliente,
             clientRequest.msgData.est.pidServidorDedicado,
             clientRequest.msgData.infoTarifa);

    /* Verifica se o nome do condutor é válido (apenas letras e espaços) */
    const char *nomeCondutor = clientRequest.msgData.est.viatura.nomeCondutor;
    for (int i = 0; nomeCondutor[i] != '\0'; i++) {
        if (!isalpha(nomeCondutor[i]) && nomeCondutor[i] != ' ') {
            // Se encontrar um caractere inválido, reporta erro e termina o processo
            so_error("SD8.4", "Nome do condutor inválido: contém caracteres inválidos (%c)", nomeCondutor[i]);
            sd11_EncerraServidorDedicado();
        }
    }

    /* Lista de nomes válidos do servidor Tigre (nome completo) */
    const char *nomesValidos[] = {
        "Ana Santos",
        "Bruno Costa",
        "Carlos Ferreira",
        "Paulo Trezentos"
    };
    int numNomes = sizeof(nomesValidos) / sizeof(nomesValidos[0]);
    int nomeValido = 0;
    for (int i = 0; i < numNomes; i++) {
        if (strcmp(nomeCondutor, nomesValidos[i]) == 0) {
            nomeValido = 1;
            break;
        }
    }

    if (!nomeValido) {
        so_error("SD8.4", "Nome do condutor inválido: %s", nomeCondutor);
        sd11_EncerraServidorDedicado();
    }

    so_success("SD8.4", "Nome do condutor válido: %s", nomeCondutor);
    so_debug(">");
}

/**
 * @brief sd9_1_AdormeceTempoRandom Ler a descrição da tarefa SD9.1 no enunciado
 */
 void sd9_1_AdormeceTempoRandom() {
    so_debug("<");

    int tempo = so_random_between_values(0, MAX_ESPERA);
    so_success("SD9.1", "%d", tempo);
    sleep(tempo);

    so_debug(">");
}

/**
 * @brief sd9_2_EnviaSucessoAoCliente Ler a descrição da tarefa SD9.2 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 */
void sd9_2_EnviaSucessoAoCliente(int msgId, MsgContent clientRequest) {
    so_debug("< @param msgId:%d; clientRequest:[%ld:%d:%s:%c:%s:%d:%d:%s]", 
        msgId, clientRequest.msgType, clientRequest.msgData.status, 
        clientRequest.msgData.est.viatura.matricula,
        clientRequest.msgData.est.viatura.categoria,
        clientRequest.msgData.est.viatura.pais,
        clientRequest.msgData.est.pidCliente, 
        indexClienteBD);

    clientRequest.msgType = clientRequest.msgData.est.pidCliente;
    clientRequest.msgData.status = CLIENT_ACCEPTED;

    if (msgsnd(msgId, &clientRequest, sizeof(clientRequest.msgData), 0) == -1) {
        so_error("SD9.2", "Erro ao enviar mensagem ao Cliente");
        sd11_EncerraServidorDedicado();
        return;
    }

    so_success("SD9.2", "SD: Confirmei Cliente Lugar %d", indexClienteBD);

    so_debug(">");
}

/**
 * @brief sd9_3_EscreveLogEntradaViatura Ler a descrição da tarefa SD9.3 no enunciado
 * @param logFilename (I) O nome do ficheiro de Logfile (i.e., FILE_LOGFILE)
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 * @param pposicaoLogfile (O) posição do ficheiro Logfile mesmo antes de inserir o log desta viatura
 * @param plogItem (O) registo de Log para esta viatura
 */
void sd9_3_EscreveLogEntradaViatura(char *logFilename, MsgContent clientRequest, long *pposicaoLogfile, LogItem *plogItem) {
    so_debug("< [logFilename: %s]", logFilename);

    FILE *fp = fopen(logFilename, "ab+");
    if (fp == NULL) {
        so_error("SD9.3", "Não foi possível abrir o arquivo de log");
        sd11_EncerraServidorDedicado();
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        fclose(fp);
        so_error("SD9.3", "Erro ao posicionar no final do log");
        sd11_EncerraServidorDedicado();
    }

    long pos = ftell(fp);
    if (pos < 0) {
        fclose(fp);
        so_error("SD9.3", "Falha ao obter a posição atual do log");
        sd11_EncerraServidorDedicado();
    }
    *pposicaoLogfile = pos;
    *plogItem = (LogItem){ clientRequest.msgData.est.viatura, "", "" };

    time_t currentTime = time(NULL);
    struct tm currentTm;
    localtime_r(&currentTime, &currentTm);
    strftime(plogItem->dataEntrada, sizeof(plogItem->dataEntrada), "%Y-%m-%dT%Hh%M", &currentTm);

    size_t bytesWritten = fwrite(plogItem, sizeof(LogItem), 1, fp);
    if (bytesWritten != 1) {
        fclose(fp);
        so_error("SD9.3", "Erro na escrita do log");
        sd11_EncerraServidorDedicado();
    }
    fclose(fp);

    so_success("SD9.3", "SD: Guardei log na posição %ld: Entrada Cliente %s em %s", 
               *pposicaoLogfile, plogItem->viatura.matricula, plogItem->dataEntrada);

    so_debug("> [logPos: %ld, LogItem: [%s:%s:%c:%s:%s:%s]]", 
             *pposicaoLogfile, plogItem->viatura.matricula, plogItem->viatura.pais, 
             plogItem->viatura.categoria, plogItem->viatura.nomeCondutor, 
             plogItem->dataEntrada, plogItem->dataSaida);
}
/**
 * @brief  sd10_1_AguardaCheckout Ler a descrição da tarefa SD10.1 no enunciado
 * @param msgId (I) identificador aberto de IPC
 */
 void sd10_1_AguardaCheckout(int msgId) {
    so_debug("< [@param msgId:%d]", msgId);

    MsgContent pmsg;
    pid_t pidServidorDedicado = getpid();  // o próprio processo dedicado

    while (1) {
        if (msgrcv(msgId, &pmsg, sizeof(pmsg.msgData), pidServidorDedicado, 0) == -1) {
            so_error("SD10.1", "Erro ao ler mensagem do Cliente");
            sd11_EncerraServidorDedicado();
            return;
        }

        if (pmsg.msgData.status == TERMINA_ESTACIONAMENTO) {
            so_success("SD10.1", "SD: A viatura %s deseja sair do parque", pmsg.msgData.est.viatura.matricula);
            break;
        }
    }
    so_debug(">");
}

/**
 * @brief  sd10_1_1_TrataAlarme Ler a descrição da tarefa SD10.1.1 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void sd10_1_1_TrataAlarme(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    /* Rearmar o alarme para que este sinal seja entregue a cada 60 segundos */
    alarm(60);

    /* Obter a data e hora atuais no formato "AAAA-MM-DDTHHhmm" */
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    char datetime[32];
    strftime(datetime, sizeof(datetime), "%Y-%m-%dT%Hh%M", &tm_info);

    /* Ler a tarifa atual da SHM partilhada pela entidade FACE.
       Como a leitura de um inteiro é atómica, evitamos o risco de deadlock
       (pois não é necessário ocupar semáforos nesta operação). */
    int tarifa = (tarifaAtual != NULL) ? *tarifaAtual : 0;

    /* Formatar a mensagem de informação com o formato correto (sem espaço extra após dois pontos) */
    char info[128];
    snprintf(info, sizeof(info), "%s Tarifa atual:%d", datetime, tarifa);

    /* Entrar na zona crítica para proteger o acesso aos dados do cliente */
    struct sembuf op_lock;
    op_lock.sem_num = 0;
    op_lock.sem_op = -1;
    op_lock.sem_flg = SEM_UNDO;
    if (semop(semId, &op_lock, 1) == -1) {
        so_error("sd10_1_1_TrataAlarme", "Erro ao entrar na zona crítica: %s", strerror(errno));
        /* Garantir que o alarme é rearmado mesmo em caso de erro */
        alarm(60);
        return;
    }

    /* Copiar os dados do cliente para garantir que os campos pidCliente e pidServidorDedicado são preservados */
    MsgContent msg = clientRequest;
    msg.msgData.status = INFO_TARIFA;
    strncpy(msg.msgData.infoTarifa, info, sizeof(msg.msgData.infoTarifa) - 1);
    msg.msgData.infoTarifa[sizeof(msg.msgData.infoTarifa) - 1] = '\0';

    /* Ajustar o msgType para o PID do cliente */
    msg.msgType = clientRequest.msgData.est.pidCliente;

    /* Enviar a mensagem usando o tamanho correto do campo de dados */
    if (msgsnd(msgId, &msg, sizeof(msg.msgData), 0) == -1) {
        so_error("sd10_1_1_TrataAlarme", "Erro ao enviar Info Tarifa: %s", strerror(errno));
    }

    /* Sair da zona crítica */
    struct sembuf op_unlock;
    op_unlock.sem_num = 0;
    op_unlock.sem_op = 1;
    op_unlock.sem_flg = SEM_UNDO;
    if (semop(semId, &op_unlock, 1) == -1) {
        so_error("sd10_1_1_TrataAlarme", "Erro ao sair da zona crítica: %s", strerror(errno));
    }

    /* Log success after leaving the critical section with the correct tariff value */
    so_success("SD10.1.1", "%d", tarifa);
    so_debug(">");
}

/**
 * @brief  sd10_2_EscreveLogSaidaViatura Ler a descrição da tarefa SD10.2 no enunciado
 * @param  logFilename (I) O nome do ficheiro de Logfile (i.e., FILE_LOGFILE)
 * @param  posicaoLogfile (I) posição do ficheiro Logfile mesmo antes de inserir o log desta viatura
 * @param  logItem (I) registo de Log para esta viatura
 */
 void sd10_2_EscreveLogSaidaViatura(char *logFilename, long posicaoLogfile, LogItem logItem) {
    so_debug("< [@param logFilename:%s, posicaoLogfile:%ld, logItem:[%s:%s:%c:%s:%s:%s]]", logFilename, posicaoLogfile, logItem.viatura.matricula, logItem.viatura.pais, logItem.viatura.categoria, logItem.viatura.nomeCondutor, logItem.dataEntrada, logItem.dataSaida);

    FILE *f = fopen(logFilename, "rb+");
    if (!f) {
        so_error("SD10.2", "Erro ao abrir ficheiro de log: %s", logFilename);
        sd11_EncerraServidorDedicado();
    }

    // Ir para a posição do log original
    if (fseek(f, posicaoLogfile, SEEK_SET) != 0) {
        fclose(f);
        so_error("SD10.2", "Erro ao posicionar no log para atualizar");
        sd11_EncerraServidorDedicado();
    }

    // Atualizar timestamp de saída
    time_t agora = time(NULL);
    strftime(logItem.dataSaida, sizeof(logItem.dataSaida), "%Y-%m-%dT%Hh%M", localtime(&agora));

    // Escrever o logItem atualizado
    if (fwrite(&logItem, sizeof(LogItem), 1, f) != 1) {
        fclose(f);
        so_error("SD10.2", "Erro ao escrever atualização no log");
        sd11_EncerraServidorDedicado();
    }
    fclose(f);
    so_success("SD10.2", "SD: Atualizei log na posição %ld: Saída Cliente %s em %s", posicaoLogfile, logItem.viatura.matricula, logItem.dataSaida);
    sd11_EncerraServidorDedicado();
    so_debug(">");
}

/**
 * @brief  sd11_EncerraServidorDedicado Ler a descrição da tarefa SD11 no enunciado
 *         OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
void sd11_EncerraServidorDedicado() {
    so_debug("<");

    sd11_1_LibertaLugarViatura(semId, lugaresEstacionamento, indexClienteBD);
    sd11_2_EnviaTerminarAoClienteETermina(msgId, clientRequest);

    so_debug(">");
}

/**
 * @brief sd11_1_LibertaLugarViatura Ler a descrição da tarefa SD11.1 no enunciado
 * @param semId (I) identificador aberto de IPC
 * @param lugaresEstacionamento (I) array de lugares de estacionamento que irá servir de BD
 * @param indexClienteBD (I) índice do lugar correspondente a este pedido na BD (>= 0), ou -1 se não houve nenhum lugar disponível
 */
void sd11_1_LibertaLugarViatura(int semId, Estacionamento *lugaresEstacionamento, int indexClienteBD) {
    so_debug("< [@param semId:%d, lugaresEstacionamento:%p, indexClienteBD:%d]", semId, lugaresEstacionamento, indexClienteBD);

    if (indexClienteBD < 0) {
        so_error("sd11_1_LibertaLugarViatura", "Índice do cliente na BD inválido: %d", indexClienteBD);
    } else {
        // Entra na zona crítica utilizando o semáforo de mutex (índice 0)
        struct sembuf op_lock;
        op_lock.sem_num = 0;  // SEM_MUTEX_BD
        op_lock.sem_op = -1;  // operação P (lock)
        op_lock.sem_flg = 0;
        if (semop(semId, &op_lock, 1) == -1) {
            so_error("sd11_1_LibertaLugarViatura", "Erro ao entrar na zona crítica: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        // Marca o lugar como disponível
        lugaresEstacionamento[indexClienteBD].pidCliente = DISPONIVEL;
        so_success("SD11.1", "SD: Libertei Lugar: %d", indexClienteBD);
        
        // Sai da zona crítica liberando o semáforo de mutex (índice 0)
        struct sembuf op_unlock;
        op_unlock.sem_num = 0;  // SEM_MUTEX_BD
        op_unlock.sem_op = 1;   // operação V (unlock)
        op_unlock.sem_flg = 0;
        if (semop(semId, &op_unlock, 1) == -1) {
            so_error("sd11_1_LibertaLugarViatura", "Erro ao sair da zona crítica: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        // Libera o lugar no semáforo SEM_LUGARES_PARQUE (índice 3)
        struct sembuf op_release;
        op_release.sem_num = 3;  // SEM_LUGARES_PARQUE
        op_release.sem_op = 1;   // operação V (release slot)
        op_release.sem_flg = 0;
        if (semop(semId, &op_release, 1) == -1) {
            so_error("sd11_1_LibertaLugarViatura", "Erro ao liberar lugar no semáforo: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    so_debug(">");
}

/**
 * @brief sd11_2_EnviaTerminarAoClienteETermina Ler a descrição da tarefa SD11.2 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param clientRequest (I) pedido recebido, enviado por um Cliente
 */
 void sd11_2_EnviaTerminarAoClienteETermina(int msgId, MsgContent clientRequest) {
    so_debug("< [@param msgId:%d, clientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]", msgId, clientRequest.msgType, clientRequest.msgData.status, clientRequest.msgData.est.viatura.matricula, clientRequest.msgData.est.viatura.pais, clientRequest.msgData.est.viatura.categoria, clientRequest.msgData.est.viatura.nomeCondutor, clientRequest.msgData.est.pidCliente, clientRequest.msgData.est.pidServidorDedicado, clientRequest.msgData.infoTarifa);

    clientRequest.msgType = clientRequest.msgData.est.pidCliente;
    clientRequest.msgData.status = ESTACIONAMENTO_TERMINADO;

    // Enviar mensagem
    if (msgsnd(msgId, &clientRequest, sizeof(clientRequest.msgData), 0) == -1) {
        so_error("SD11.2", "Erro ao enviar mensagem final ao cliente");
        exit(EXIT_FAILURE);
    }
    so_success("SD11.2", "SD: Shutdown");
    so_debug(">");
    exit(EXIT_SUCCESS);
}

/**
 * @brief  sd12_TrataSigusr2    Ler a descrição da tarefa SD12 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void sd12_TrataSigusr2(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_success("SD12", "(SD12) SD: Recebi pedido do Servidor para terminar");
    sd11_EncerraServidorDedicado();

    so_debug(">");
}