#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define QLEN 5
#define BUFFERSIZE 500

int passivesockTCP(char *service, int qlen);

int main()
{
    struct sockaddr_in fsin;
    unsigned int alen;
    char rbuffer[BUFFERSIZE];
    char fbuffer[BUFFERSIZE];
    int size_read, passive_sock, new_sock, file_d;
    char url[1024];
    char metodo[1024];
    char *service = "http";

    passive_sock = passivesockTCP(service, QLEN);

    while (1)
    {
        new_sock = accept(passive_sock, (struct sockaddr *)&fsin, &alen);
        size_read = read(new_sock, rbuffer, sizeof rbuffer);

        sscanf(rbuffer, "%9s %49s", metodo, url);

        printf("URL solicitada: %s", url);

        file_d = open(url, O_RDONLY);

        while (size_read = read(file_d, fbuffer, sizeof fbuffer))
        {
            send(new_sock, fbuffer, sizeof fbuffer, 0);
        }
        close(file_d);
        close(new_sock);
    }
}