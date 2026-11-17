#include <string.h>
#include <strings.h>

struct mime
{
    const char *extension;
    const char *tipo_mime;
};

const char *identificarTipo(const char *ruta_archivo)
{
    char *tipo_file = strrchr(ruta_archivo, '.');
    const char *sin_exten = "text/plain";

    if (tipo_file == NULL)
        return sin_exten;

    struct mime mapeo_tipo_file[] = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
    };

    for (int i = 0; i < sizeof(mapeo_tipo_file) / sizeof(mapeo_tipo_file[0]); i++)
    {
        if (strcasecmp(tipo_file, mapeo_tipo_file[i].extension) == 0)
        {
            return mapeo_tipo_file[i].tipo_mime;
        }
    }

    return sin_exten;
}