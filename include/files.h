#ifndef FILES_H
#define FILES_H
#include <stdlib.h>
#include <sys/types.h>
int obtenerRuta(const char *ruta_cliente, char *ruta_valida, size_t tam_ruta_max, off_t *tamano_salida);
#endif