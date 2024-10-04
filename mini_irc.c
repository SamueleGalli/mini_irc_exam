#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct client
{
    int id;
    char msg[4242];
} t_client;

typedef struct server
{
    int max;
    int len;
    int sockfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in cli;
    struct client c[1024];
    socklen_t sock_len;
    fd_set fds, fd_read, fd_write;
    char write_b[424242];
    char read_b[424242];
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

int    accept_new_client(t_server *s, int next_id)
{
    //create a new socket
    s->sock_len = sizeof(s->servaddr);
    int c_socket = accept(s->sockfd, (struct sockaddr *)&s->servaddr, &s->sock_len);
    //used to track the higest fd
    if (c_socket > s->max)
        s->max = c_socket;
    //assigninf id to client
    s->c[c_socket].id = next_id++;
    //clearing msg
    bzero(s->c[c_socket].msg, strlen(s->c[c_socket].msg));
    //the socket is added to active socket
    FD_SET(c_socket, &s->fds);
    //sending welcome message to client
    sprintf(s->write_b, "server: client %d just arrived\n", s->c[c_socket].id);
    send_msg(c_socket, s);
    return (next_id);
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

void    handling_host(t_server *s, int next_id)
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
                next_id = accept_new_client(s, next_id);
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
    int next_id = 0;
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
    bzero(&s->servaddr, sizeof(s->servaddr));
    bzero(s->c, sizeof(s->c));
    s->servaddr.sin_family = AF_INET;
    s->servaddr.sin_addr.s_addr = htonl(2130706433);
    s->servaddr.sin_port = htons(atoi(v[1]));
    if ((bind(s->sockfd, (const struct sockaddr *)&s->servaddr, sizeof(s->servaddr))) != 0)
        print_error("Fatal error");
    if (listen(s->sockfd, 10) != 0)
        print_error("Fatal error");
    s->len = sizeof(s->cli);
    handling_host(s, next_id);
}