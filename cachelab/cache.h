#ifndef CACHE_H
#define CACHE_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

extern char HIT;
extern const char *MISS_CLEAN;
extern const char *MISS_DIRTY;
extern const int PENALTY_MISS;
extern const char LECTURA;

typedef struct linea
{
    int last_used;
    int dirty;
    int dirty_previo;
    int tag;
    int tag_previo;
    int valid_bit;
    int valid_previo;
} linea_t;

typedef struct set
{
    int cantidad_lineas;
    linea_t *lineas;
} set_t;

typedef struct metricas
{
    int lecturas;
    int escrituras;
    int total_operaciones;
    int rmiss;
    int wmiss;
    int dirty_rmiss;
    int dirty_wmiss;
    int bytes_leidos;
    int bytes_escritos;
    int tiempo_lecturas;
    int tiempo_escrituras;
} metricas_t;

typedef struct cache
{
    set_t *sets;
    metricas_t *metricas;

    int tamanio;
    int asociatividad;
    int cantidad_sets;
    int block_offset;
    int set_index_bits;
    int set_block_offset_bits;
    bool hit;
} cache_t;

typedef struct transaccion
{
    char operacion;
    unsigned long direccion;
    int last_used;
    int last_used_previo;
    char *caso;
} transaccion_t;

transaccion_t *inicializar_transaccion(char *linea, int last_used);
cache_t *inicializar_cache(int tamanio, int cantidad_sets, int asociatividad);
set_t *inicializar_sets(int cantidad_sets, int asociatividad);
metricas_t *inicializar_metricas();
void inicializar_linea(linea_t *linea);
linea_t *inicializar_lineas(int asociatividad);

void manipular_cache(cache_t *cache, transaccion_t *transaccion, bool modo_verboso);

void actulizar_linea(cache_t *cache, transaccion_t *transaccion, int tag, linea_t *linea);
void actualizar_set(cache_t *cache, transaccion_t *transaccion);
void actualizar_metricas_miss(cache_t *cache, int tiempo, transaccion_t *transaccion);
void actualizar_metricas_hit(cache_t *cache, transaccion_t *transaccion);

int indice_set(cache_t *cache, unsigned long direccion);
int log2_int(int numero);

int bitMask(int highbit, int lowbit);
int obtener_tag(cache_t *cache, unsigned long direccion);
int linea_menos_accedida(set_t set);
int linea_a_operar(set_t set);
int posicion_linea(cache_t *cache, set_t set, int tag);

void imprimir_resumen(cache_t *cache);
void imprimir_metrica(metricas_t *metrica);
void imprimir_modo_verboso(cache_t *cache, transaccion_t *transaccion);

void liberar_memoria(cache_t *cache);

#endif