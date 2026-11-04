#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define	QLEN		  5
#define	BUFFERSIZE		4096

extern int	errno;
int	errexit(const char *format, ...);
int	passiveTCP(const char *service, int qlen);

int main(int argc, char *argv[]){
    struct	sockaddr_in fsin;
    unsigned int	alen;
    char rbuffer[BUFFERSIZE];
    char fbuffer[BUFFERSIZE];
    char sdbuffer[BUFFERSIZE];
    int size_read, size_send, passive_sock, new_sock, file_d;
    char url[1024];
    char metodo[1024];

    char	*service = "http";

    passive_sock = passiveTCP(service, QLEN);

    while (1)
    {
        new_sock = accept(passive_sock, (struct sockaddr *)&fsin, &alen);
        size_read = read(new_sock, rbuffer, sizeof rbuffer);

        sscanf(rbuffer, "%9s %49s", metodo, url);

        printf("URL solicitada: %s", url);

        file_d = open(url, O_RDONLY);

        while (size_read = read(file_d, fbuffer, sizeof fbuffer))
        {    
            size_send = send(new_sock, fbuffer, sizeof fbuffer, 0);
        }
        close(file_d);
        close(new_sock);
    }
    
}

