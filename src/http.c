#include <stdio.h>
#include <string.h>

struct solicitud_http
{
    char metodo[16];
    char recurso[100];
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

int parseoSolicitud(const char *bufferSoli, struct solicitud_http *sol_cliente)
{
    if (bufferSoli == NULL || strstr(bufferSoli, "\r\n\r\n") == NULL)
        return -1;

    // struct solicitud_http sol_cliente;
    memset(sol_cliente, 0, sizeof(*sol_cliente));

    struct formato_headers headers_basicos[] = {
        {"Host:", 5, sol_cliente->host, sizeof(sol_cliente->host), &sol_cliente->tiene_host},
        {"Connection:", 11, sol_cliente->connection, sizeof(sol_cliente->connection), NULL},
        {"User-Agent:", 11, sol_cliente->user_agent, sizeof(sol_cliente->user_agent), NULL}};

    char *fin_sol, *fin_headers;

    fin_sol = strstr(bufferSoli, "\r\n");
    fin_headers = strstr(bufferSoli, "\r\n\r\n");

    if (sscanf(bufferSoli, "%15s %99s %9s", sol_cliente->metodo, sol_cliente->recurso, sol_cliente->version) != 3)
        return -1;

    if (strcmp(sol_cliente->metodo, "GET") != 0)
        return -1; // Bad Method

    char *inicio_headers = fin_sol + 2;
    char *aux;
    fin_headers = fin_headers + 2;

    while ((fin_headers - inicio_headers) != 0)
    {
        aux = strstr(inicio_headers, "\r\n");
        if (aux == NULL)
            return -1;

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

                strncpy(headers_basicos[i].destino, field_value, headers_basicos[i].max_destino);

                if (headers_basicos[i].bandera != NULL)
                    *headers_basicos[i].bandera = 1;
            }
        }

        inicio_headers = aux + 2;
    }

    if (sol_cliente->tiene_host == 0 || sol_cliente->user_agent[0] == 0 || sol_cliente->connection[0] == 0)
    {
        return -1;
    }

    return 0;
}

int generarRespuesta(int id_respuesta)
{
}

int main()
{
    struct solicitud_http cliente;

    const char *solicitud =
        "GET /api/v1/usuarios/42 HTTP/1.1\r\n"
        "Host:                                 ejemplo.com\r\n"
        "User-Agent:       Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n"
        "Connection: close            \r\n"
        "\r\n";

    int val = parseoSolicitud(solicitud, &cliente);

    if (val == -1)
    {
        printf("Valor return %i\n", val);
        return -1;
    }

    printf("Metodo: %s\n", cliente.metodo);
    printf("Recurso: %s\n", cliente.recurso);
    printf("Version: %s\n", cliente.version);

    printf("Host: %s\n", cliente.host);
    printf("User-Agent: %s\n", cliente.user_agent);
    printf("Connection: %s\n", cliente.connection);

    return 0;
}