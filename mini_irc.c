#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

typedef struct client
{
    int id; //identificator of client
    char msg[4242]; //size of message
} t_client;

typedef struct server
{
    int max, sockfd; //max fd, socket of the fd of the server itself
    struct sockaddr_in server, client; //hold server adress information, hold the client adress information
    struct client c[1024]; //the number of client
    socklen_t sock_len; //lenght of sockadress_in used when calling accept
    fd_set fds, fd_read, fd_write; //fds (fds current fd) (fds_read fds for reading) (fd_write fd used to write)
    char write_b[424242] ,read_b[424242]; //buffer for writing ,buffer for reading
} t_server;

void    print_error(char *s)
{
    for (int i = 0; s[i] != 0; i++)
    {
        write(2, &s[i], 1);
    }
    write(2, "\n", 1);
    exit (1);
}

void    send_msg(int s_fd, t_server *s)
{
    for (int fd = 0; fd <= s->max; fd++)
    {
        if (FD_ISSET(fd, &s->fd_write) && fd != s_fd)
            write(fd, s->write_b, strlen(s->write_b));
    }
}

int    accept_new_client(t_server *s, int ids)
{
    //create a new socket
    s->sock_len = sizeof(s->server);
    int c_socket = accept(s->sockfd, (struct sockaddr *)&s->server, &s->sock_len);

    //used to track the higest fd
    if (c_socket > s->max)
        s->max = c_socket;

    //assigning id to client
    s->c[c_socket].id = ids++;

    //clearing msg
    bzero(s->c[c_socket].msg, sizeof(s->c[c_socket].msg));

    //the socket is added to active socket
    FD_SET(c_socket, &s->fds);

    //sending welcome message to client
    sprintf(s->write_b, "server: client %d just arrived\n", s->c[c_socket].id);
    send_msg(c_socket, s);

    return (ids);
}

void    client_disconnect(t_server *s, int fd)
{
    sprintf(s->write_b, "server: client %d just left\n", s->c[fd].id);
    send_msg(fd, s);
    FD_CLR(fd, &s->fds);
    close(fd);
}

void    client_message(t_server *s, int fd, int read)
{
    for (int i = 0, j = strlen(s->c[fd].msg); i < read; i++, j++)
    {
        //adding data from buff to msg
        s->c[fd].msg[j] = s->read_b[i];

        //if i added \n then i replace it with the end of string
        if (s->c[fd].msg[j] == '\n')
        {
            s->c[fd].msg[j] = '\0';
            sprintf(s->write_b, "client %d: %s\n", s->c[fd].id, s->c[fd].msg);
            send_msg(fd, s);
            bzero(s->c[fd].msg, strlen(s->c[fd].msg));
            j = -1;
        }
    }
}

void    handling_host(t_server *s, int ids)
{
    while (1)
    {
        //s->fds the file descriptor the server is tracking
        //read and write fds to track if the client need to rad or write
        s->fd_read = s->fds;
        s->fd_write = s->fds;
        //function to monitor fds to see if they need to write or read
        //oterwhise continue the loop indicating problem
        if (select(s->max + 1, &(s->fd_read), &(s->fd_write), NULL, NULL) < 0)
            continue;
        //check if fd is read for reading
        for (int fd = 0; fd <= s->max; fd++)
        {
            if (FD_ISSET(fd, &s->fd_read) && fd == s->sockfd)
                ids = accept_new_client(s, ids);
            //if a fd of a client try reading
            if (FD_ISSET(fd, &s->fd_read) && fd != s->sockfd)
            {
                //read data
                int read = recv(fd, s->read_b, sizeof(s->read_b), 0);

                //if a client dissconect the message is sent to all other users
                if (read <= 0)
                    client_disconnect(s, fd);
                else
                    client_message(s, fd, read);
            }
        }
    }
}

int main(int i, char **v)
{
    int ids = 0; //update the id of client
    if (i != 2)
        print_error("Wrong number of arguments");
    t_server *s = malloc(sizeof(t_server));
    if (!s)
        print_error("Fatal error");
    s->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->sockfd == -1)
        print_error("Fatal error");
    s->max = s->sockfd;
    //reset fd clearing reaming memory
    FD_ZERO(&s->fds);
    //set the server socket to (s->fds)
    FD_SET(s->sockfd, &s->fds);
    bzero(&s->server, sizeof(s->server));
    bzero(s->c, sizeof(s->c));
    s->server.sin_family = AF_INET;
    s->server.sin_addr.s_addr = htonl(2130706433);
    s->server.sin_port = htons(atoi(v[1]));
    if ((bind(s->sockfd, (const struct sockaddr *)&s->server, sizeof(s->server))) != 0)
        print_error("Fatal error");
    if (listen(s->sockfd, 10) != 0)
        print_error("Fatal error");
    s->sock_len = sizeof(s->client);
    handling_host(s, ids);
}