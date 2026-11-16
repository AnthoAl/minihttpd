#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "../include/http.h"

struct solicitud_http
{
    char metodo[16];
    char recurso[30];
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

    char *fin_sol, *fin_headers, sol_line[100];

    if (snprintf(sol_line, sizeof(sol_line), "%s", bufferSoli) >= sizeof(sol_line))
        return generarRespuesta(sock, 400, NULL, "close");

    if (sscanf(sol_line, "%15s %29s %9s", sol_cliente.metodo, sol_cliente.recurso, sol_cliente.version) != 3)
        return generarRespuesta(sock, 400, NULL, "close");

    if (strcmp(sol_cliente.metodo, "GET") != 0)
        return generarRespuesta(sock, 405, NULL, "close");

    if (strcmp(sol_cliente.version, "HTTP/1.1") != 0)
        return generarRespuesta(sock, 400, NULL, "close");

    fin_sol = strstr(bufferSoli, "\r\n");
    fin_headers = strstr(bufferSoli, "\r\n\r\n");

    char *inicio_headers = fin_sol + 2;
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

        for (int i = 0; i < 3; i++)
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
int generarRespuesta(int sock, int codigo, const char *ruta_archivo, const char *conexion)
{
    char cabecera[1024];
    struct stat st;

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

    char rutaFix[50];

    snprintf(rutaFix, sizeof(rutaFix), "../www%s", ruta_archivo);

    int file_d = open(rutaFix, O_RDONLY);
    if (file_d < 0)
    {
        switch (errno)
        {
        case ENOENT:
            return generarRespuesta(sock, 404, NULL, "close");

        case EACCES:
            return generarRespuesta(sock, 403, NULL, "close");

        default:
            return generarRespuesta(sock, 500, NULL, "close");
        }
    }

    if (fstat(file_d, &st) != 0)
    {
        close(file_d);
        return generarRespuesta(sock, 500, NULL, "close");
    }

    int len_cabecera = snprintf(cabecera, sizeof(cabecera),
                                "HTTP/1.1 %d %s\r\n"
                                "Content-Type: text/html\r\n"
                                "Content-Length: %lld\r\n"
                                "Connection: %s"
                                "\r\n\r\n",
                                codigo, mapeo_mensaje(codigo), (long long)st.st_size, conexion);

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