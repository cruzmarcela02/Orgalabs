#include <stdio.h>
#include "cache.h"

bool presencia_errores(int argc, char *argv[])
{
    FILE *traza = fopen(argv[1], "r");
    if (traza == NULL)
    {
        fprintf(stderr, "Error. No se especifico el archivo de trazas.\n");
        return true;
    }
    fclose(traza);
    if (argc != 5 && argc != 8)
    {
        fprintf(stderr, "Error. Cantidad de argumentos invalida");
        return true;
    }
    int size = atoi(argv[2]);
    int asociatividad = atoi(argv[3]);
    int cantidad_sets = atoi(argv[4]);
    if (size % 2 != 0 || (asociatividad % 2 != 0 && asociatividad != 1) || cantidad_sets % 2 != 0)
    {
        fprintf(stderr, "sizes :%d, asociatividad: %d, cantidad_sets: %d\n", size, asociatividad, cantidad_sets);
        fprintf(stderr, "Error. Los parametros C, E y S deben ser potencias de dos.\n");
        return false;
    }
    if (size < (asociatividad * cantidad_sets))
    {
        fprintf(stderr, "Error. Combinacion invalida de argumentos.\n");
        return true;
    }
    if (argc == 8)
    {
        int n = atoi(argv[5]);
        int m = atoi(argv[6]);
        if (n < 0 || m < n || m < 0)
        {
            fprintf(stderr, "Error. Se debe cumplir que 0 ≤ n ≤ m\n");
            return true;
        }
    }
    return false;
}

int main(int argc, char *argv[])
{
    if (presencia_errores(argc, argv))
    {
        return 1;
    }

    int size = atoi(argv[2]);
    int asociatividad = atoi(argv[3]);
    int cantidad_sets = atoi(argv[4]);
    bool modo_verboso = false;
    int n;
    int m;
    cache_t *cache;
    transaccion_t *transaccion;
    int contador = 0;
    char *linea = NULL;
    FILE *traza = fopen(argv[1], "r");
    size_t x;

    if (argc == 8)
    {
        n = atoi(argv[6]);
        m = atoi(argv[7]);
        modo_verboso = true;
    }

    cache = inicializar_cache(size, cantidad_sets, asociatividad);
    while (getline(&linea, &x, traza) != -1)
    {
        transaccion = inicializar_transaccion(linea, contador);
        manipular_cache(cache, transaccion, modo_verboso);
        contador++;

        if (n <= contador && contador <= m)
        {
            modo_verboso = true;
        }
        else
        {
            modo_verboso = false;
        }
        free(transaccion);
    }
    imprimir_resumen(cache);
    free(linea);
    fclose(traza);

    liberar_memoria(cache);
    return 0;
}
