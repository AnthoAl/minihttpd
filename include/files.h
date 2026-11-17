#include <stdlib.h>
#include <sys/types.h>

#ifndef FILES_H
#define FILES_H
int obtenerRuta(const char *ruta_cliente, char *ruta_valida, size_t tam_ruta_max, off_t *tamano_salida);
#endif