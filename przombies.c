#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    
    return 0;
}