#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "../include/http.h"
#include "../include/server.h"

struct epoll_event ev;

int passivesockTCP(char *service, int qlen)
{
    struct addrinfo hints, *result, *rep;
    int sfd = -1;
    int state;
    int opt = 1;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_socktype = SOCK_STREAM;

    state = getaddrinfo(NULL, service, &hints, &result);
    if (state != 0)
    {
        fprintf(stderr, "Fallo en getaddrinfo: %s\n", gai_strerror(state));
        exit(EXIT_FAILURE);
    }

    for (rep = result; rep != NULL; rep = rep->ai_next)
    {
        sfd = socket(rep->ai_family, rep->ai_socktype, rep->ai_protocol);
        if (sfd < 0)
        {
            continue;
        }

        if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            perror("Error en setsockopt");
            close(sfd);
            exit(EXIT_FAILURE);
        }

        if (bind(sfd, rep->ai_addr, rep->ai_addrlen) == 0)
            break;

        close(sfd);
        sfd = -1;
    }

    freeaddrinfo(result);

    if (sfd < 0)
    {
        perror("Error en bind");
        exit(EXIT_FAILURE);
    }

    if (listen(sfd, qlen) < 0)
    {
        close(sfd);
        fprintf(stderr, "Fallo al hacer listen\n");
        exit(EXIT_FAILURE);
    }

    if (configurarNoBloqueante(sfd) == -1)
    {
        close(sfd);
        return -1;
    }

    return sfd;
}

static int configurarNoBloqueante(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void aceptarClientes(int passive_sock, int fd_epoll)
{
    configurarNoBloqueante(passive_sock);

    while (1)
    {
        int new_cliente = accept(passive_sock, NULL, NULL);
        if (new_cliente < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            perror("accept");
            break;
        }

        configurarNoBloqueante(new_cliente);

        struct sesion_sock *scliente = calloc(1, sizeof(struct sesion_sock));

        scliente->fd_sock = new_cliente;
        scliente->fd_file_sol = -1;
        scliente->estado = LEYENDO_PETICION;
        ev.events = EPOLLIN;
        ev.data.fd = new_cliente;
        epoll_ctl(fd_epoll, EPOLL_CTL_ADD, new_cliente, &ev);
    };
}

void manejarLectura(int fd_epoll, struct sesion_sock *cliente)
{
    while (1)
    {
        ssize_t readed = read(cliente->fd_sock, cliente->buffer_soli, sizeof(cliente->buffer_soli));

        if (readed > 0)
        {
            cliente->bytes_recibidos += readed;
            cliente->buffer_soli[cliente->bytes_recibidos] = '\0';

            if (strchr(cliente->buffer_soli, '\r\n\r\n') != NULL)
            {
                cliente->estado = ENVIANDO_CABECERAS;
                ev.events = EPOLLOUT;
                ev.data.ptr = cliente;
                epoll_ctl(fd_epoll, EPOLL_CTL_MOD, cliente->fd_sock, &ev);
                break;
            }
            else if (cliente->bytes_recibidos == sizeof(cliente->buffer_soli))
            {
                generarRespuesta(cliente->fd_sock, 400, NULL, "close");
            }
        }
        else if (readed == 0)
        {
            generarRespuesta(cliente->fd_sock, 400, NULL, "close");
            cerrarCliente(fd_epoll, cliente);
            break;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            cerrarCliente(fd_epoll, cliente);
            break;
        }
    }
}

void cerrarCliente(int fd_epoll, struct sesion_sock *cliente)
{
    epoll_ctl(fd_epoll, EPOLL_CTL_DEL, cliente->fd_sock, NULL);
    close(cliente->fd_sock);
    free(cliente);
}