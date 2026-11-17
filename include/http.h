#ifndef HTTP
#define HTTP
int parseoSolicitud(const char *bufferSoli, int sock);
char *mapeo_mensaje(int codigo);
int generarRespuesta(int sock, int codigo, const char *ruta_archivo, const char *conexion);
#endif