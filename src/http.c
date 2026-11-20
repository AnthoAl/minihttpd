#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include "http.h"
#include "mime.h"
#include "files.h"

struct solicitud_http
{
    char metodo[16];
    char recurso[50];
    char version[10];

    char host[100];
    char connection[100];
    char user_agent[256];

    int tiene_host;
};

struct formato_headers
{
    const char *field_name;
    size_t len_fname;
    char *destino;
    size_t max_destino;
    int *bandera;
};

static const char *mapeo_mensaje(int codigo);

void parseoSolicitud(struct sesion_sock *cliente)
{
    if (cliente == NULL || cliente->bytes_recibidos == 0 || strstr(cliente->buffer_soli, "\r\n\r\n") == NULL)
    {
        generarRespuesta(cliente, 400, NULL, "close");
        cliente->keep_alive = 0;
        return;
    }

    struct solicitud_http sol_cliente = {0};

    struct formato_headers headers_basicos[] = {
        {"Host:", 5, sol_cliente.host, sizeof(sol_cliente.host), &sol_cliente.tiene_host},
        {"Connection:", 11, sol_cliente.connection, sizeof(sol_cliente.connection), NULL},
        {"User-Agent:", 11, sol_cliente.user_agent, sizeof(sol_cliente.user_agent), NULL}};

    char *fin_lsol, *fin_headers;

    fin_lsol = strstr(cliente->buffer_soli, "\r\n");
    size_t tam_linerq = fin_lsol - cliente->buffer_soli;
    if (tam_linerq > 100)
    {
        generarRespuesta(cliente, 400, NULL, "close");
        cliente->keep_alive = 0;
        return;
    }

    char sol_line[tam_linerq + 1];
    strncpy(sol_line, cliente->buffer_soli, tam_linerq);
    sol_line[tam_linerq] = '\0';

    if (sscanf(sol_line, "%15s %29s %9s", sol_cliente.metodo, sol_cliente.recurso, sol_cliente.version) != 3)
    {
        generarRespuesta(cliente, 400, NULL, "close");
        cliente->keep_alive = 0;
        return;
    }

    if (strcmp(sol_cliente.metodo, "GET") != 0)
    {
        generarRespuesta(cliente, 405, NULL, "close");
        cliente->keep_alive = 0;
        return;
    }

    if (strcmp(sol_cliente.version, "HTTP/1.1") != 0)
    {
        generarRespuesta(cliente, 400, NULL, "close");
        cliente->keep_alive = 0;
        return;
    }

    fin_headers = strstr(cliente->buffer_soli, "\r\n\r\n");

    char *inicio_headers = fin_lsol + 2;
    char *aux;
    fin_headers = fin_headers + 2;

    while (inicio_headers < fin_headers)
    {
        aux = strstr(inicio_headers, "\r\n");
        if (aux == NULL || aux > fin_headers)
        {
            generarRespuesta(cliente, 400, NULL, "close");
            return;
        }

        size_t tam_header = aux - inicio_headers;
        char header[tam_header + 1];
        strncpy(header, inicio_headers, tam_header);
        header[tam_header] = '\0';

        for (size_t i = 0; i < sizeof(headers_basicos) / sizeof(headers_basicos[0]); i++)
        {
            if (strncasecmp(header, headers_basicos[i].field_name, headers_basicos[i].len_fname) == 0)
            {
                const char *field_value = header + headers_basicos[i].len_fname;

                while (*field_value == ' ')
                    field_value++;

                if (snprintf(headers_basicos[i].destino, headers_basicos[i].max_destino, "%s", field_value) >= headers_basicos[i].max_destino)
                {
                    generarRespuesta(cliente, 400, NULL, "close");
                    return;
                }

                if (headers_basicos[i].bandera != NULL)
                    *headers_basicos[i].bandera = 1;
            }
        }

        inicio_headers = aux + 2;
    }

    if (!sol_cliente.tiene_host || sol_cliente.host[0] == '\0')
    {
        generarRespuesta(cliente, 400, NULL, "close");
        cliente->keep_alive = 0;
        return;
    }

    if (strcasecmp(sol_cliente.connection, "close") == 0)
    {
        cliente->keep_alive = 0;
    }
    else
    {
        snprintf(sol_cliente.connection, sizeof(sol_cliente.connection), "keep-alive");
        cliente->keep_alive = 1;
    }

    generarRespuesta(cliente, 200, sol_cliente.recurso, sol_cliente.connection);
}

/*
    Retorna 1 cerrar la conexión y 0 mantenerla
*/
void generarRespuesta(struct sesion_sock *cliente, int codigo, const char *ruta_cliente, const char *conexion)
{
    char cabecera[1024];

    // Respuestas de error
    if (codigo != 200)
    {
        char mensaje_error[256];

        int len_mensaje = snprintf(mensaje_error, sizeof(mensaje_error),
                                   "<html><body><h1>%d %s</h1></body></html>",
                                   codigo, mapeo_mensaje(codigo));

        int len_cabecera = snprintf(cabecera, sizeof(cabecera),
                                    "HTTP/1.1 %d %s\r\n"
                                    "Content-Type: text/html\r\n"
                                    "Content-Length: %d\r\n"
                                    "Connection: %s"
                                    "\r\n\r\n",
                                    codigo, mapeo_mensaje(codigo), len_mensaje, conexion);

        cliente->tam_buffer_resp = len_cabecera + len_mensaje;
        snprintf(cliente->buffer_resp, sizeof(cliente->buffer_resp), "%s%s", cabecera, mensaje_error);
        return;
    }

    char ruta_file[PATH_MAX];

    int codigo_ruta = obtenerRuta(ruta_cliente, ruta_file, sizeof(ruta_file), &cliente->tam_file);

    if (codigo_ruta != 200)
    {
        generarRespuesta(cliente, codigo_ruta, NULL, "close");
        cliente->keep_alive = 0;
        return;
    }

    cliente->fd_file_sol = open(ruta_file, O_RDONLY);

    if (cliente->fd_file_sol < 0)
    {
        generarRespuesta(cliente, 500, NULL, "close");
        cliente->keep_alive = 0;
        return;
    }

    int len_cabecera = snprintf(cabecera, sizeof(cabecera),
                                "HTTP/1.1 %d %s\r\n"
                                "Content-Type: %s\r\n"
                                "Content-Length: %lld\r\n"
                                "Connection: %s"
                                "\r\n\r\n",
                                codigo, mapeo_mensaje(codigo), identificarTipo(ruta_file), (long long)cliente->tam_file, conexion);

    cliente->tam_buffer_resp = len_cabecera;
    snprintf(cliente->buffer_resp, sizeof(cliente->buffer_resp), "%s", cabecera);
}

static const char *mapeo_mensaje(int codigo)
{
    switch (codigo)
    {
    case 200:
        return "OK";

    case 400:
        return "Bad Request";

    case 403:
        return "Forbidden";

    case 404:
        return "Not Found";

    case 405:
        return "Method Not Allowed";

    case 500:
        return "Internal Server Error";

    default:
        return "Código desconocido";
    }
}