#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "../include/http.h"

#define QLEN 5
#define BUFFERSIZE 4096

int passivesockTCP(char *service, int qlen);

int main()
{
    struct sockaddr_in fsin;
    unsigned int alen;
    char rbuffer[BUFFERSIZE];
    int size_read, passive_sock, new_sock;
    char *service = "http";

    passive_sock = passivesockTCP(service, QLEN);

    while (1)
    {
        alen = sizeof(fsin);
        new_sock = accept(passive_sock, (struct sockaddr *)&fsin, &alen);

        while (1)
        {
            size_read = read(new_sock, rbuffer, BUFFERSIZE - 1);

            if (size_read > 0)
            {
                rbuffer[size_read] = '\0';
            }
            else
            {
                close(new_sock);
                break;
            }

            if (parseoSolicitud(rbuffer, new_sock) == 1)
            {
                close(new_sock);
                break;
            }
        }
    }
}