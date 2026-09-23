#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

FILE *log_file = NULL;

void trata_sigterm(int sig) {
    if (log_file != NULL) {
        fprintf(log_file, "=== Daemon encerrado com seguranca (Sinal %d) ===\n", sig);
        fclose(log_file);
    }

    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <n_segundos> [-p|-d]\n", argv[0]);
        fprintf(stderr, "  <n_segundos>  Intervalo de tempo em segundos\n");
        fprintf(stderr, "  -p            Usa o comando ps via pipe\n");
        fprintf(stderr, "  -d            Varre o diretório /proc (PADRAO)\n");
        return 1;
    }

    int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "O intervalo de tempo deve ser um numero positivo.\n");
        return 1;
    }

    int usa_pipe = 0;
    
    if (argc >= 3) {
        if (strcmp(argv[2], "-p") == 0) {
            usa_pipe = 1;
        } else if (strcmp(argv[2], "-d") != 0) {
            fprintf(stderr, "Opcao invalida. Use -p ou -d.\n");
            return 1;
        }
    }

    printf ("Iniciando daemon: intervalo de %d segundos, metodo %s.\n", n, usa_pipe ? "ps via pipe" : "diretorio /proc");

    pid_t pid = fork();

    if (pid < 0) {
        // Se o fork falhar (retornar -1), encerramos a execução com erro
        fprintf(stderr, "Erro ao criar o processo daemon.\n");
        exit(1);
    }

    if (pid > 0) {
        // O pai recebe um PID > 0. Ele chama exit(0) para finalizar e liberar o terminal.
        exit(0);
    }

    for (int i = 1; i <= 31; i++) {
        if (i != SIGKILL && i != SIGSTOP && i != SIGTERM) {
            signal(i, SIG_IGN);
        }
    }

    signal(SIGTERM, trata_sigterm);

    log_file = fopen("zombies.log", "a");

    if (log_file == NULL) {
        exit(1);
    }

    while (1) {
        sleep(n);

        fprintf(log_file, "PID\tPPID\tNome do Programa\n");
        fprintf(log_file, "==========================================\n");
    }


    fflush(log_file);
    return 0;
}