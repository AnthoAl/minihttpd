#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

int obtenerRuta(const char *ruta_cliente, char *ruta_valida, size_t tam_ruta_max, off_t *tamano_salida)
{
    char ruta_base[PATH_MAX];
    char posible_ruta[PATH_MAX];
    char ruta_real[PATH_MAX];

    if (strcmp(ruta_cliente, "/") == 0)
    {
        ruta_cliente = "/index.html";
    }

    if (realpath("./www", ruta_base) == NULL)
    {
        return 500;
    }

    int bytes_escritos = snprintf(posible_ruta, sizeof(posible_ruta), "%s/%s",
                                  ruta_base, ruta_cliente);

    if (bytes_escritos < 0 || (size_t)bytes_escritos >= sizeof(posible_ruta))
    {
        return 500;
    }

    if (realpath(posible_ruta, ruta_real) == NULL)
    {
        if (errno == ENOENT || errno == ENOTDIR)
        {
            return 404;
        }
        if (errno == EACCES)
        {
            return 403;
        }
        return 500;
    }

    size_t len_base = strlen(ruta_base);
    if (strncmp(ruta_real, ruta_base, len_base) != 0 ||
        (ruta_real[len_base] != '/' && ruta_real[len_base] != '\0'))
    {
        return 403;
    }

    struct stat info;
    if (stat(ruta_real, &info) != 0)
    {
        return 500;
    }

    snprintf(ruta_valida, tam_ruta_max, "%s", ruta_real);
    if (tamano_salida != NULL)
    {
        *tamano_salida = info.st_size;
    }

    return 200;
}