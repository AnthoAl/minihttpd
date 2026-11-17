#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include "../include/http.h"
#include "../include/mime.h"
#include "../include/files.h"

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

int parseoSolicitud(const char *bufferSoli, int sock)
{
    if (bufferSoli == NULL || strstr(bufferSoli, "\r\n\r\n") == NULL)
        return generarRespuesta(sock, 400, NULL, "close");

    struct solicitud_http sol_cliente = {0};

    struct formato_headers headers_basicos[] = {
        {"Host:", 5, sol_cliente.host, sizeof(sol_cliente.host), &sol_cliente.tiene_host},
        {"Connection:", 11, sol_cliente.connection, sizeof(sol_cliente.connection), NULL},
        {"User-Agent:", 11, sol_cliente.user_agent, sizeof(sol_cliente.user_agent), NULL}};

    char *fin_lsol, *fin_headers;

    fin_lsol = strstr(bufferSoli, "\r\n");
    size_t tam_linerq = fin_lsol - bufferSoli;
    if (tam_linerq > 100)
    {
        return generarRespuesta(sock, 400, NULL, "close");
    }

    char sol_line[tam_linerq + 1];
    strncpy(sol_line, bufferSoli, tam_linerq);
    sol_line[tam_linerq] = '\0';

    if (sscanf(sol_line, "%15s %29s %9s", sol_cliente.metodo, sol_cliente.recurso, sol_cliente.version) != 3)
        return generarRespuesta(sock, 400, NULL, "close");

    if (strcmp(sol_cliente.metodo, "GET") != 0)
        return generarRespuesta(sock, 405, NULL, "close");

    if (strcmp(sol_cliente.version, "HTTP/1.1") != 0)
        return generarRespuesta(sock, 400, NULL, "close");

    fin_headers = strstr(bufferSoli, "\r\n\r\n");

    char *inicio_headers = fin_lsol + 2;
    char *aux;
    fin_headers = fin_headers + 2;

    while (inicio_headers < fin_headers)
    {
        aux = strstr(inicio_headers, "\r\n");
        if (aux == NULL || aux > fin_headers)
            return generarRespuesta(sock, 400, NULL, "close");

        size_t tam_header = aux - inicio_headers;
        char header[tam_header + 1];
        strncpy(header, inicio_headers, tam_header);
        header[tam_header] = '\0';

        for (int i = 0; i < sizeof(headers_basicos) / sizeof(headers_basicos[0]); i++)
        {
            if (strncasecmp(header, headers_basicos[i].field_name, headers_basicos[i].len_fname) == 0)
            {
                const char *field_value = header + headers_basicos[i].len_fname;

                while (*field_value == ' ')
                    field_value++;

                if (snprintf(headers_basicos[i].destino, headers_basicos[i].max_destino, "%s", field_value) >= headers_basicos[i].max_destino)
                {
                    return generarRespuesta(sock, 400, NULL, "close");
                }

                if (headers_basicos[i].bandera != NULL)
                    *headers_basicos[i].bandera = 1;
            }
        }

        inicio_headers = aux + 2;
    }

    if (!sol_cliente.tiene_host || sol_cliente.host[0] == '\0')
    {
        return generarRespuesta(sock, 400, NULL, "close");
    }

    if (sol_cliente.connection[0] == '\0')
    {
        snprintf(sol_cliente.connection, sizeof(sol_cliente.connection), "keep-alive");
    }

    return generarRespuesta(sock, 200, sol_cliente.recurso, sol_cliente.connection);
}

/*
    Retorna 1 cerrar la conexión y 0 mantenerla
*/
int generarRespuesta(int sock, int codigo, const char *ruta_cliente, const char *conexion)
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

        send(sock, cabecera, len_cabecera, 0);
        send(sock, mensaje_error, len_mensaje, 0);
        return 1;
    }

    char ruta_file[PATH_MAX];
    off_t tam_file;

    int codigo_ruta = obtenerRuta(ruta_cliente, ruta_file, sizeof(ruta_file), &tam_file);

    if (codigo_ruta != 200)
    {
        return generarRespuesta(sock, codigo_ruta, NULL, "close");
    }

    int file_d = open(ruta_file, O_RDONLY);

    if (file_d < 0)
    {
        return generarRespuesta(sock, 500, NULL, "close");
    }

    int len_cabecera = snprintf(cabecera, sizeof(cabecera),
                                "HTTP/1.1 %d %s\r\n"
                                "Content-Type: %s\r\n"
                                "Content-Length: %lld\r\n"
                                "Connection: %s"
                                "\r\n\r\n",
                                codigo, mapeo_mensaje(codigo), identificarTipo(ruta_file), (long long)tam_file, conexion);

    send(sock, cabecera, len_cabecera, 0);

    char bloque[4096];
    ssize_t bytes_leidos;
    while ((bytes_leidos = read(file_d, bloque, sizeof(bloque))) > 0)
    {
        send(sock, bloque, bytes_leidos, 0);
    }

    close(file_d);

    if (strcasecmp(conexion, "close") == 0)
    {
        return 1;
    }

    return 0;
}

char *mapeo_mensaje(int codigo)
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