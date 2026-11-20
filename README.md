# Proyecto Minihttpd

En este proyecto se desarrolló un servidor web concurrente con la versión HTTP/1.1. La implementación del servidor emplea sockets no bloqueantes y un arquitectura orientada a eventos, gestionados por `epoll` y un solo hilo de ejecución.

## Estructura del proyecto

```text
minihttpd/
├── Makefile
├── README.md
├── include/
│      ├── http.h
│      ├── server.h
│      ├── mime.h
│      └── files.h
├── src/
│    ├── main.c
│    ├── server.c
│    ├── http.c
│    ├── mime.c
│    └── files.c
├── www/
      ├── index.html
      ├── style.css
      └── image.png             
```

## Compilación y ejecución

1. Compile el proyecto desde la carpeta raíz del repositorio para generar el ejecutable `minihttpd`:
    
    ```bash
    make
    ```
2. Inicie el servidor desde la raíz del proyecto:
    ```bash
    ./minihttpd
    ```

