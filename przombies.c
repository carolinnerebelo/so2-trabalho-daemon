#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>
#include <ctype.h>

FILE *log_file = NULL;

void trata_sigterm(int sig) {
    if (log_file != NULL) {
        fprintf(log_file, "=== Daemon encerrado com seguranca (Sinal %d) ===\n", sig);
        fclose(log_file);
    }

    exit(0);
}

// Função auxiliar para verificar se uma string contém apenas números. Vamos usá-la para verificar se o nome do diretório é um número (se for, é o diretório de um processo)
int is_numeric(const char *str) {
    while (*str) {
        if (!isdigit(*str)) return 0;
        str++;
    }
    return 1;
}

// Busca varrendo o diretório /proc
void busca_proc() {
    DIR *dir = opendir("/proc");
    struct dirent *entry;
    char path[512];

    if (dir == NULL) return;

    while ((entry = readdir(dir)) != NULL) {
        if (is_numeric(entry->d_name)) {
            // Monta o caminho exato para o arquivo stat do processo
            snprintf(path, sizeof(path), "/proc/%s/stat", entry->d_name);
            FILE *f = fopen(path, "r");

            if (f != NULL) {
                int pid, ppid;
                char comm[256];
                char state;

                // O arquivo stat contém os dados na ordem: PID Nome Estado PPID
                // Lemos essas 4 primeiras informações
                if (fscanf(f, "%d (%[^)]) %c %d", &pid, comm, &state, &ppid) == 4) {
                    if (state == 'Z') {
                        fprintf(log_file, "%d\t%d\t%s\n", pid, ppid, comm);
                    }
                }

                fclose(f);
            }
        }
    }

    fprintf(log_file, "\n");
    closedir(dir);
}

void busca_pipe() {
    // popen cria um filho, executa o comando e abre um pipe de leitura ("r")
    // usamos -eo para forçar o ps a imprimir apenas as colunas exatas que precisamos
    FILE *fp = popen("ps -eo pid,ppid,stat,comm", "r");
    if (fp == NULL) return;

    char line[256];

    fgets(line, sizeof(line), fp);

    int pid, ppid;
    char state[10];
    char comm[256];

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (sscanf(line, "%d %d %9s %255s", &pid, &ppid, state, comm) == 4) {
            if (state[0] == 'Z') {
                fprintf(log_file, "%d\t%d\t%s\n", pid, ppid, comm);
            }
        }
    }

    fprintf(log_file, "\n");
    pclose(fp);
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

        if (usa_pipe == 1) {
            busca_pipe();
        } else {
            busca_proc();
        }

        fflush(log_file);
    }

    return 0;
}