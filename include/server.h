#ifndef PASSIVE_S
#define PASSIVE_S

#include <stddef.h>
#include <sys/types.h>

enum estados
{
    LEYENDO_PETICION,
    ENVIANDO_CABECERAS,
    ENVIANDO_ARCHIVO
};

struct sesion_sock
{
    int fd_sock;
    char buffer_soli[4096];
    size_t bytes_recibidos;

    char buffer_resp[4096];
    size_t tam_buffer_resp;
    size_t bytes_enviados;
    int fd_file_sol;
    off_t file_offset;
    off_t tam_file;

    enum estados estado;
    int keep_alive;
};

int passivesockTCP(char *service, int qlen);

void aceptarClientes(int passive_sock, int fd_epoll);

void manejarLectura(int fd_epoll, struct sesion_sock *cliente);

void manejarEscritura(int fd_epoll, struct sesion_sock *cliente);

void finalizarEnvio(int fd_epoll, struct sesion_sock *cliente);

void cerrarCliente(int fd_epoll, struct sesion_sock *cliente);

#endif