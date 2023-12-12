#include "cache.h"

char HIT = '1';
const char *MISS_CLEAN = "2a";
const char *MISS_DIRTY = "2b";
const int PENALTY_MISS = 100;
const char LECTURA = 'R';

int log2_int(int numero)
{
	int resultado = 0;

	while (numero >>= 1)
	{
		resultado++;
	}

	return resultado;
}

int bitMask(int highbit, int lowbit)
{
	int mascara = 255;
	int mascara_32 = mascara << 24 | mascara << 16 | mascara << 8 | mascara;
	int filtro_highbit = mascara_32 << highbit;
	int filtro_lowbit = mascara_32 << lowbit;
	int filtro = (filtro_highbit << 1) ^ (filtro_lowbit);
	int resta = highbit + (~lowbit + 1);
	int signo_resta = (resta >> 31);
	return (~(signo_resta)) & filtro;
}

metricas_t *inicializar_metricas()
{
	metricas_t *metricas = (metricas_t *)malloc(sizeof(metricas_t));
	metricas->lecturas = 0;
	metricas->escrituras = 0;
	metricas->total_operaciones = 0;
	metricas->rmiss = 0;
	metricas->wmiss = 0;
	metricas->dirty_rmiss = 0;
	metricas->dirty_wmiss = 0;
	metricas->bytes_leidos = 0;
	metricas->bytes_escritos = 0;
	metricas->tiempo_lecturas = 0;
	metricas->tiempo_escrituras = 0;
	return metricas;
}

void inicializar_linea(linea_t *linea)
{

	linea->dirty = 0;
	linea->dirty_previo = linea->dirty;
	linea->tag = -1;
	linea->tag_previo = linea->tag;
	linea->valid_bit = 0;
	linea->valid_previo = linea->valid_bit;
	linea->last_used = 0;
}

linea_t *inicializar_lineas(int asociatividad)
{
	linea_t *lineas = (linea_t *)malloc(asociatividad * sizeof(linea_t));
	for (int i = 0; i < asociatividad; i++)
	{
		inicializar_linea(&lineas[i]);
	}
	return lineas;
}

set_t *inicializar_sets(int numero_sets, int asociatividad)
{
	set_t *sets = malloc(numero_sets * sizeof(set_t));
	for (int i = 0; i < numero_sets; i++)
	{
		sets[i].lineas = inicializar_lineas(asociatividad);
		sets[i].cantidad_lineas = asociatividad;
	}
	return sets;
}

transaccion_t *inicializar_transaccion(char *linea, int last_used)
{
	transaccion_t *transaccion = (transaccion_t *)malloc(sizeof(transaccion_t));
	strtok(linea, " ");
	char *operacion = (strtok(NULL, " "));
	transaccion->operacion = operacion[0];

	char *direccion = strtok(NULL, " ");
	transaccion->direccion = (unsigned long)strtol(direccion, NULL, 16);
	transaccion->last_used = last_used;
	transaccion->last_used_previo = last_used;
	transaccion->caso = NULL;

	return transaccion;
}

cache_t *inicializar_cache(int tamanio, int numero_sets, int asociatividad)
{
	cache_t *cache = (cache_t *)malloc(sizeof(cache_t));
	cache->tamanio = tamanio;
	cache->asociatividad = asociatividad;
	cache->sets = inicializar_sets(numero_sets, asociatividad);
	cache->metricas = inicializar_metricas();
	cache->block_offset = tamanio / (numero_sets * asociatividad);
	cache->cantidad_sets = numero_sets;
	cache->set_index_bits = log2_int(cache->cantidad_sets);
	cache->set_block_offset_bits = log2_int(cache->block_offset);
	cache->hit = false;

	return cache;
}

void manipular_cache(cache_t *cache, transaccion_t *transaccion, bool modo_verboso)
{
	actualizar_set(cache, transaccion);
	if (modo_verboso)
	{
		imprimir_modo_verboso(cache, transaccion);
	}
}

int indice_set(cache_t *cache, unsigned long direccion)
{
	return ((direccion & bitMask(cache->set_index_bits + cache->set_block_offset_bits - 1, cache->set_block_offset_bits)) >> cache->set_block_offset_bits);
}

int obtener_tag(cache_t *cache, unsigned long direccion)
{
	direccion = direccion >> (cache->set_index_bits + cache->set_block_offset_bits);

	return (direccion & bitMask(31 - (cache->set_index_bits + cache->set_block_offset_bits), 0));
}

void actualizar_set(cache_t *cache, transaccion_t *transaccion)
{
	int tag = obtener_tag(cache, transaccion->direccion);
	int posicion_set = indice_set(cache, transaccion->direccion);
	set_t set = cache->sets[posicion_set];
	int posicion = posicion_linea(cache, set, tag);
	linea_t linea = set.lineas[posicion];

	actulizar_linea(cache, transaccion, tag, &linea);

	set.lineas[posicion] = linea;
	cache->sets[posicion_set] = set;
}

int linea_menos_accedida(set_t set)
{
	int posicion = 0;
	int last_used = set.lineas[0].last_used;
	for (int i = 0; i < set.cantidad_lineas; i++)
	{
		if (set.lineas[i].last_used < last_used && set.lineas[i].valid_bit == 1)
		{
			posicion = i;
			last_used = set.lineas[i].last_used;
		}
	}
	return posicion;
}

int linea_a_operar(set_t set)
{
	int posicion = 0;
	bool encontrado = false;
	int posicion_linea = -1;

	while (posicion < set.cantidad_lineas && !encontrado)
	{
		if (set.lineas[posicion].valid_bit == 0)
		{
			encontrado = true;
			posicion_linea = posicion;
		}
		posicion++;
	}
	if (posicion_linea == -1)
	{
		posicion_linea = linea_menos_accedida(set);
	}
	return posicion_linea;
}

int posicion_linea(cache_t *cache, set_t set, int tag)
{
	int posicion = -1;

	for (int i = 0; i < set.cantidad_lineas; i++)
	{
		if (set.lineas[i].valid_bit == 1)
		{
			if (set.lineas[i].tag == tag)
			{
				posicion = i;
				cache->hit = true;
			}
		}
	}
	if (posicion == -1)
	{
		posicion = linea_a_operar(set);
		cache->hit = false;
	}
	return posicion;
}

void actulizar_linea(cache_t *cache, transaccion_t *transaccion, int tag, linea_t *linea)
{
	int tiempo = 0;
	linea->valid_previo = linea->valid_bit;
	linea->dirty_previo = linea->dirty;
	linea->tag_previo = linea->tag;

	if (cache->hit)
	{
		actualizar_metricas_hit(cache, transaccion);
	}
	else
	{
		linea->valid_bit = 1;
		linea->tag = tag;

		if (linea->dirty == 1)
		{
			transaccion->caso = MISS_DIRTY;
			tiempo = (1 + 2 * PENALTY_MISS);
		}
		else
		{
			transaccion->caso = MISS_CLEAN;
			tiempo = (1 + PENALTY_MISS);
		}

		actualizar_metricas_miss(cache, tiempo, transaccion);

		if (transaccion->operacion == LECTURA)
		{
			linea->dirty = 0;
		}
	}

	if (transaccion->operacion != LECTURA)
	{
		linea->dirty = 1;
	}

	transaccion->last_used_previo = linea->last_used;
	linea->last_used = transaccion->last_used;
}

void actualizar_metricas_miss(cache_t *cache, int tiempo, transaccion_t *transaccion)
{
	cache->metricas->total_operaciones++;

	if (transaccion->operacion == LECTURA)
	{
		cache->metricas->lecturas += 1;
		cache->metricas->rmiss += 1;
		cache->metricas->tiempo_lecturas += tiempo;

		if (tiempo == (1 + 2 * PENALTY_MISS))
		{
			cache->metricas->dirty_rmiss += 1;
			cache->metricas->bytes_escritos += cache->block_offset;
		}
	}
	else
	{
		cache->metricas->escrituras += 1;
		cache->metricas->wmiss += 1;
		cache->metricas->tiempo_escrituras += tiempo;
		if (tiempo == (1 + 2 * PENALTY_MISS))
		{
			cache->metricas->dirty_wmiss += 1;
			cache->metricas->bytes_escritos += cache->block_offset;
		}
	}

	cache->metricas->bytes_leidos += cache->block_offset;
}

void actualizar_metricas_hit(cache_t *cache, transaccion_t *transaccion)
{
	cache->metricas->total_operaciones++;

	if (transaccion->operacion == LECTURA)
	{
		cache->metricas->lecturas++;
		cache->metricas->tiempo_lecturas += 1;
	}
	else
	{
		cache->metricas->escrituras++;
		cache->metricas->tiempo_escrituras += 1;
	}
}

void imprimir_resumen(cache_t *cache)
{
	int size_KiB = cache->tamanio / 1000;
	int cantidad_sets = cache->cantidad_sets;
	if (cache->asociatividad == 1)
	{
		printf("direct-mapped, %d sets, size = %dKB", cantidad_sets, size_KiB);
	}
	else
	{
		printf("%d-way, %d sets, size = %dKB", cache->asociatividad, cantidad_sets, size_KiB);
	}
	printf("\n");
	imprimir_metrica(cache->metricas);
}

void imprimir_metrica(metricas_t *metrica)
{
	int total_miss = metrica->rmiss + metrica->wmiss;
	float miss_rate = total_miss / (float)metrica->total_operaciones;
	printf("loads %d stores %d total %d\n", metrica->lecturas, metrica->escrituras, metrica->total_operaciones);
	printf("rmiss %d wmiss %d total %d\n", metrica->rmiss, metrica->wmiss, total_miss);
	printf("dirty rmiss %d dirty wmiss %d\n", metrica->dirty_rmiss, metrica->dirty_wmiss);
	printf("bytes read %d bytes written %d\n", metrica->bytes_leidos, metrica->bytes_escritos);
	printf("read time %d write time %d\n", metrica->tiempo_lecturas, metrica->tiempo_escrituras);
	printf("miss rate %f", miss_rate);
}

void imprimir_modo_verboso(cache_t *cache, transaccion_t *transaccion)
{
	int cache_tag = obtener_tag(cache, transaccion->direccion);
	int cache_index = indice_set(cache, transaccion->direccion);
	int cache_line = posicion_linea(cache, cache->sets[cache_index], cache_tag);
	int line_tag = cache->sets[cache_index].lineas[cache_line].tag_previo;
	int valid_bit = cache->sets[cache_index].lineas[cache_line].valid_previo;
	int dirty_bit = cache->sets[cache_index].lineas[cache_line].dirty_previo;

	printf("%d ", transaccion->last_used);
	if (transaccion->caso == MISS_CLEAN)
	{
		printf("%s", MISS_CLEAN);
	}
	else if (transaccion->caso == MISS_DIRTY)
	{
		printf("%s", MISS_DIRTY);
	}
	else
	{
		printf("%c", HIT);
	}

	printf(" %x %x %d ", cache_index, cache_tag, cache_line);

	if (line_tag == -1)
	{
		printf("%d", line_tag);
	}
	else
	{
		printf("%x", line_tag);
	}

	printf(" %d %d", valid_bit, dirty_bit);

	if (cache->asociatividad > 1)
	{
		printf(" %d", transaccion->last_used_previo);
	}
	printf("\n");
}

void liberar_memoria(cache_t *cache)
{
	for (int i = 0; i < cache->cantidad_sets; i++)
	{
		free(cache->sets[i].lineas);
	}
	free(cache->sets);
	free(cache->metricas);
	free(cache);
}