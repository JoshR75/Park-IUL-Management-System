/****************************************************************************************
 ** ISCTE-IUL: Trabalho prático 3 de Sistemas Operativos 2024/2025, Enunciado Versão 1+
 **
 ** Aluno: Nº:124549       Nome:Josh Rufino 
 ** Nome do Módulo: cliente.c
 ** Descrição/Explicação do Módulo:
 **
 **
 ***************************************************************************************/

// #define SO_HIDE_DEBUG                // Uncomment this line to hide all @DEBUG statements
#include "defines.h"

/*** Variáveis Globais ***/
int msgId = -1;                         // Variável que tem o ID da Message Queue
MsgContent clientRequest;               // Pedido enviado do Cliente para o Servidor
int recebeuRespostaServidor = FALSE;    // Variável que determina se o Cliente já recebeu uma resposta do Servidor

/**
 * @brief Processamento do processo Cliente.
 *        OS ALUNOS NÃO DEVERÃO ALTERAR ESTA FUNÇÃO.
 */
int main () {
    so_debug("<");

    // c1_IniciaCliente:
    c1_1_GetMsgQueue(IPC_KEY, &msgId);
    c1_2_ArmaSinaisCliente();

    // c2_CheckinCliente:
    c2_1_InputEstacionamento(&clientRequest);
    c2_2_EscrevePedido(msgId, clientRequest);

    c3_ProgramaAlarme(MAX_ESPERA);

    // c4_AguardaRespostaServidor:
    c4_1_EsperaRespostaServidor(msgId, &clientRequest);
    c4_2_DesligaAlarme();

    c5_MainCliente(msgId, &clientRequest);

    so_error("Cliente", "O programa nunca deveria ter chegado a este ponto!");
    so_debug(">");
    return 0;
}

/**
 * @brief c1_1_GetMsgQueue Ler a descrição da tarefa C1.1 no enunciado
 * @param ipcKey (I) Identificador de IPC a ser usada para o projeto
 * @param pmsgId (O) identificador aberto de IPC
 */
void c1_1_GetMsgQueue(key_t ipcKey, int *pmsgId) {
    so_debug("< [@param ipcKey:0x0%x]", ipcKey);

    /* Tenta obter a fila de mensagens associada à chave ipcKey */
    *pmsgId = msgget(ipcKey, 0666);
    if (*pmsgId == -1) {
        so_error("C1.1", "Falha ao obter a Message Queue.");
        exit(EXIT_FAILURE);
    }

    so_success("C1.1", "Message Queue obtida com sucesso.");
    so_debug("> [@return *pmsgId:%d]", *pmsgId);
}

/**
 * @brief c1_2_ArmaSinaisCliente Ler a descrição da tarefa C1.2 no enunciado
 */
void c1_2_ArmaSinaisCliente() {
    so_debug("<");

    /* Associa o sinal SIGINT (CTRL+C) à função c6_TrataCtrlC */
    if (signal(SIGINT, c6_TrataCtrlC) == SIG_ERR) {
        so_error("C1.2", "Erro ao associar SIGINT.");
        exit(EXIT_FAILURE);
    }

    /* Associa o sinal SIGALRM ao timeout de check-in, com a função c7_TrataAlarme */
    if (signal(SIGALRM, c7_TrataAlarme) == SIG_ERR) {
        so_error("C1.2", "Erro ao associar SIGALRM.");
        exit(EXIT_FAILURE);
    }

    so_success("C1.2", "Sinais armados com sucesso");
    so_debug(">");
}

/**
 * @brief c2_1_InputEstacionamento Ler a descrição da tarefa C2.1 no enunciado
 * @param pclientRequest (O) pedido a ser enviado por este Cliente ao Servidor
 */
void c2_1_InputEstacionamento(MsgContent *pclientRequest) {
    so_debug("<");
    char input[100];
    int i, valid;

    /* Matrícula */
    printf("Introduza a Matrícula: ");
    if (fgets(input, sizeof(input), stdin) == NULL) {
        so_error("C2.1", "Erro a ler Matrícula.");
        exit(EXIT_FAILURE);
    }
    input[strcspn(input, "\n")] = '\0';
    valid = 0;
    for (i = 0; input[i] != '\0'; i++) {
        if (!isspace((unsigned char)input[i])) { valid = 1; break; }
    }
    if (!valid) {
        so_error("C2.1", "Matrícula inválida.");
        exit(EXIT_FAILURE);
    }
    strcpy(pclientRequest->msgData.est.viatura.matricula, input);

    /* País */
    printf("Introduza o País: ");
    if (fgets(input, sizeof(input), stdin) == NULL) {
        so_error("C2.1", "Erro a ler País.");
        exit(EXIT_FAILURE);
    }
    input[strcspn(input, "\n")] = '\0';
    valid = 0;
    for (i = 0; input[i] != '\0'; i++) {
        if (!isspace((unsigned char)input[i])) { valid = 1; break; }
    }
    if (!valid) {
        so_error("C2.1", "País inválido.");
        exit(EXIT_FAILURE);
    }
    strcpy(pclientRequest->msgData.est.viatura.pais, input);

    /* Categoria – espera apenas um carácter */
    printf("Introduza a Categoria: ");
    if (fgets(input, sizeof(input), stdin) == NULL) {
        so_error("C2.1", "Erro a ler Categoria.");
        exit(EXIT_FAILURE);
    }
    valid = 0;
    for (i = 0; input[i] != '\0'; i++) {
        if (!isspace((unsigned char)input[i])) {
            valid = 1;
            break;
        }
    }
    if (!valid) {
        so_error("C2.1", "Categoria inválida.");
        exit(EXIT_FAILURE);
    }
    pclientRequest->msgData.est.viatura.categoria = input[i];

    /* Nome do Condutor */
    printf("Introduza o Nome do Condutor: ");
    if (fgets(input, sizeof(input), stdin) == NULL) {
        so_error("C2.1", "Erro a ler Nome do Condutor.");
        exit(EXIT_FAILURE);
    }
    input[strcspn(input, "\n")] = '\0';
    valid = 0;
    for (i = 0; input[i] != '\0'; i++) {
        if (!isspace((unsigned char)input[i])) { valid = 1; break; }
    }
    if (!valid) {
        so_error("C2.1", "Nome do Condutor inválido.");
        exit(EXIT_FAILURE);
    }
    strcpy(pclientRequest->msgData.est.viatura.nomeCondutor, input);

    /* Preenche os identificadores */
    pclientRequest->msgData.est.pidCliente = getpid();
    pclientRequest->msgData.est.pidServidorDedicado = -1;

    so_success("C2.1", "%s %s %c %s %d %d",
               pclientRequest->msgData.est.viatura.matricula,
               pclientRequest->msgData.est.viatura.pais,
               pclientRequest->msgData.est.viatura.categoria,
               pclientRequest->msgData.est.viatura.nomeCondutor,
               pclientRequest->msgData.est.pidCliente,
               pclientRequest->msgData.est.pidServidorDedicado);

    so_debug("> [*pclientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]",
             pclientRequest->msgType,
             pclientRequest->msgData.status,
             pclientRequest->msgData.est.viatura.matricula,
             pclientRequest->msgData.est.viatura.pais,
             pclientRequest->msgData.est.viatura.categoria,
             pclientRequest->msgData.est.viatura.nomeCondutor,
             pclientRequest->msgData.est.pidCliente,
             pclientRequest->msgData.est.pidServidorDedicado,
             pclientRequest->msgData.infoTarifa);
}

/**
 * @brief c2_2_EscrevePedido Ler a descrição da tarefa C2.2 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param clientRequest (I) pedido a ser enviado por este Cliente ao Servidor
 */
void c2_2_EscrevePedido(int msgId, MsgContent clientRequest) {
    so_debug("< [@param msgId:%d, clientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]",
             msgId,
             clientRequest.msgType,
             clientRequest.msgData.status,
             clientRequest.msgData.est.viatura.matricula,
             clientRequest.msgData.est.viatura.pais,
             clientRequest.msgData.est.viatura.categoria,
             clientRequest.msgData.est.viatura.nomeCondutor,
             clientRequest.msgData.est.pidCliente,
             clientRequest.msgData.est.pidServidorDedicado,
             clientRequest.msgData.infoTarifa);

    clientRequest.msgType = MSGTYPE_LOGIN;
    if (msgsnd(msgId, &clientRequest, sizeof(MsgContent) - sizeof(long), 0) == -1) {
        so_error("C2.2", "Erro a enviar pedido para o Servidor.");
        exit(EXIT_FAILURE);
    }
    so_success("C2.2", "Pedido enviado com sucesso para o Servidor");

    so_debug(">");
}

/**
 * @brief c3_ProgramaAlarme Ler a descrição da tarefa C3 no enunciado
 * @param segundos (I) número de segundos a programar no alarme
 */
void c3_ProgramaAlarme(int segundos) {
    so_debug("< [@param segundos:%d]", segundos);

    alarm(segundos);
    so_success("C3", "Espera resposta em %d segundos", segundos);

    so_debug(">");
}

/**
 * @brief c4_1_EsperaRespostaServidor Ler a descrição da tarefa C4.1 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param pclientRequest (O) mensagem enviada por um Servidor Dedicado
 */
void c4_1_EsperaRespostaServidor(int msgId, MsgContent *pclientRequest) {
    so_debug("< [@param msgId:%d]", msgId);

    /* Leia da fila a mensagem cujo tipo é o PID do cliente */
    if (msgrcv(msgId, pclientRequest, sizeof(MsgContent) - sizeof(long), getpid(), 0) == -1) {
        so_error("C4.1", "Erro a receber resposta do Servidor.");
        exit(EXIT_FAILURE);
    }

    /* Verifica o status da resposta */
    if (pclientRequest->msgData.status == CLIENT_ACCEPTED) {
        so_success("C4.1", "Check-in realizado com sucesso");
    } else if (pclientRequest->msgData.status == ESTACIONAMENTO_TERMINADO) {
        so_success("C4.1", "Não é possível estacionar");
        exit(EXIT_SUCCESS);
    }

    so_debug("> [*pclientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]",
             pclientRequest->msgType,
             pclientRequest->msgData.status,
             pclientRequest->msgData.est.viatura.matricula,
             pclientRequest->msgData.est.viatura.pais,
             pclientRequest->msgData.est.viatura.categoria,
             pclientRequest->msgData.est.viatura.nomeCondutor,
             pclientRequest->msgData.est.pidCliente,
             pclientRequest->msgData.est.pidServidorDedicado,
             pclientRequest->msgData.infoTarifa);
}

/**
 * @brief c4_2_DesligaAlarme Ler a descrição da tarefa C4.2 no enunciado
 */
void c4_2_DesligaAlarme() {
    so_debug("<");

    // Desliga o alarme chamando alarm com 0
    alarm(0);
    so_success("C4.2", "Desliguei alarme");

    so_debug(">");
}

/**
 * @brief c5_MainCliente Ler a descrição da tarefa C5 no enunciado
 * @param msgId (I) identificador aberto de IPC
 * @param pclientRequest (O) mensagem enviada por um Servidor Dedicado
 */
void c5_MainCliente(int msgId, MsgContent *pclientRequest) {
    so_debug("< [@param msgId:%d]", msgId);

    while (1) {
        ssize_t ret = msgrcv(msgId, pclientRequest, sizeof(MsgContent) - sizeof(long), getpid(), IPC_NOWAIT);
        if (ret == -1) {
            // Se o erro não for que não existe mensagem, termina com erro
            if (errno != ENOMSG) {
                so_error("C5", "Erro a receber mensagem.");
                exit(EXIT_FAILURE);
            }
        } else if (ret > 0) {
            if (pclientRequest->msgData.status == INFO_TARIFA) {
                so_success("C5", "%s", pclientRequest->msgData.infoTarifa);
            } else if (pclientRequest->msgData.status == ESTACIONAMENTO_TERMINADO) {
                so_success("C5", "Estacionamento terminado");
                exit(EXIT_SUCCESS);
            }
        }
        // Aguarda brevemente para evitar espera ativa
        usleep(100000); // 0.1 segundo
    }

    // Código abaixo é inatingível devido ao loop infinito
    so_debug(">");
}
 
/**
 * @brief  c6_TrataCtrlC Ler a descrição da tarefa C6 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c6_TrataCtrlC(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d, msgId:%d, clientRequest:[%ld:%d:%s:%s:%c:%s:%d:%d:%s]]", 
             sinalRecebido, 
             msgId, 
             clientRequest.msgType, 
             clientRequest.msgData.status, 
             clientRequest.msgData.est.viatura.matricula, 
             clientRequest.msgData.est.viatura.pais, 
             clientRequest.msgData.est.viatura.categoria, 
             clientRequest.msgData.est.viatura.nomeCondutor, 
             clientRequest.msgData.est.pidCliente, 
             clientRequest.msgData.est.pidServidorDedicado, 
             clientRequest.msgData.infoTarifa);

    /* Atualiza o status para indicar cancelamento do estacionamento */
    clientRequest.msgData.status = TERMINA_ESTACIONAMENTO;

    /* Envia a mensagem de cancelamento ao Servidor Dedicado */
    if (msgsnd(msgId, &clientRequest, sizeof(MsgContent) - sizeof(long), 0) == -1) {
        so_error("C6", "Erro a enviar pedido de término de estacionamento.");
        exit(EXIT_FAILURE);
    }

    so_success("C6", "Cliente: Shutdown");
    return;
}

/**
 * @brief  c7_TrataAlarme Ler a descrição da tarefa C7 no enunciado
 * @param  sinalRecebido (I) número do sinal que é recebido por esta função (enviado pelo SO)
 */
void c7_TrataAlarme(int sinalRecebido) {
    so_debug("< [@param sinalRecebido:%d]", sinalRecebido);

    so_error("C7", "Cliente: Timeout");
    exit(EXIT_SUCCESS);

    so_debug(">");
}