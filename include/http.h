#ifndef HTTP
#define HTTP
#include "../include/server.h"
void parseoSolicitud(struct sesion_sock *cliente);
void generarRespuesta(struct sesion_sock *cliente, int codigo, const char *ruta_cliente, const char *conexion);
#endif