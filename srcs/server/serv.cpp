/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serv.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbabela <mbabela@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/16 10:53:11 by mbabela           #+#    #+#             */
/*   Updated: 2022/08/18 13:36:42 by mbabela          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "serv.hpp"


Serv::Serv(/* args */)
{
}

Serv::~Serv()
{
}


int main(int argc, char **argv)
{
    int len              = TRUE;
    int rc               = TRUE;
    int on               = TRUE;
    int socket_fd        = F_INIT;
    int new_fd           = F_INIT;
    int desc_ready;
    int end_server       = FALSE;
    int compress_array   = FALSE;
    int close_conn;
    char buffer[BUFF_SIZE];
    struct sockaddr_in6 addr;
    int timeout;
    struct pollfd       fds[POLL_SIZE];
    int     nfds = TRUE;
    int     current_size = FALSE;
    int     i;
    int     j;

                    Serv ser = Serv();
    // creating socket v6

    socket_fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        std::cout << "FAILED to create socket, errno : " << errno << std::endl;
        exit (EXIT_FAILURE);
    }
    
    // make the socket fd reuseable : 

    rc = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
    if (rc < 0)
    {
        std::cout << "FAILED at set_sock_opt, errno : " << errno << std::endl;
        close (socket_fd);
        exit (EXIT_FAILURE);
    }
    
    // hadle the part of makingthe spocket nonblocking :
    
    rc = ioctl(socket_fd, FIONBIO, (char *)&on);
    if (rc < 0)
    {
        std::cout << "FAILED at making socket nonblocking, errno : " <<  errno << std::endl;
        close (socket_fd);
        exit (EXIT_FAILURE);
    }
    
    // assign a PORT and an IP to the socket : 

    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    memcpy(&addr.sin6_addr, &in6addr_any, sizeof(in6addr_any));
    addr.sin6_port = htons(SERVER_PORT);
    rc = bind(socket_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rc < 0)
    {
        std::cout << "FAILED at binding the socket, errno : " <<  errno << std::endl;
        close (socket_fd);
        exit (EXIT_FAILURE);
    }

    // set the socket to listen : 
    
    rc = listen(socket_fd, MAX_CONN);
    if (rc < 0)
    {
        std::cout << "FAILED at listing, errno : " << errno << std::endl;
        close (socket_fd);
        exit (EXIT_FAILURE);
    }

    memset(fds, 0, sizeof(fds)); // init the poll struct

    fds[0].fd = socket_fd;
    fds[0].events = POLLIN;

    timeout = (3 * 60 * 1000); // set the timeout  to 3 min
    do
    {
        std::cout << "Waiting for a poll . . . " << std::endl;
        rc = poll(fds, nfds, timeout);
        if (rc < 0)
        {
            std::cout << "FAILED to poll . . . " << std::endl;
            break;
        }
        if (rc == 0)
        {
            std::cout << "POLL : time out !" << std::endl;
            break;
        }
        current_size = nfds;
        // i = 0;
        for (i=0; i < current_size; i++)
        {
            if (fds[i].revents == 0) // get the fd that listen and check if connected
                continue;
            if (fds[i].revents != POLLIN) // unexpected result
            {
                std::cout << "Error ! revents = : " << fds[i].revents << std::endl;
                end_server = TRUE;
                break; 
            }
            if (fds[i].fd == socket_fd)
            {
                std::cout << "Socket readable and redy to get msgs . . . " << std::endl;
                do
                {
                    // check if we accept all the connection or not, and check the errno (all connection accepted)
                    new_fd = accept(socket_fd, NULL, NULL);
                    if (new_fd < 0)
                    {
                        if (errno != EWOULDBLOCK)
                        {
                            std::cout << "FAILED at accepting connection ! " << std::endl;
                            end_server = TRUE; 
                        }
                        break;
                    }
                    std::cout << "NEW Connection detected "<< new_fd << std::endl;
                    User new_user = User(new_fd);
                    std::cout << " ---- " << std::endl;
                    ser.users.insert(std::pair<int, User>(new_fd, new_user));

                    std::cout <<  ser.users.size() << std::endl;
                    fds[nfds].fd = new_fd;
                    fds[nfds].events = POLLIN;
                    nfds++;
                }while (new_fd != -1);
            }
            else
            {
                std::cout << "A new fd (" << fds[i].fd << ") is readdable . . . " <<std::endl;
                close_conn = FALSE;
                do
                {
                    rc = recv(fds[i].fd, buffer, sizeof(buffer), 0);
                    if (rc < 0)
                    {
                        if (errno != EWOULDBLOCK)
                        {
                            std::cout << "FAILED to receive a msg !" << std::endl;
                            close_conn = TRUE;
                        }
                        break;
                    }
                    if (rc == 0)
                    {
                        std::cout << "Connection Closed . . . " << std::endl;
                        close_conn = TRUE;
                        break; 
                    }
                    len = rc;
                    std::cout << len << "bytes received !" << std::endl;
                    rc = send(fds[i].fd, buffer, len, 0);
                    if (rc < 0)
                    {
                        std::cout << "FAILED to send and answer to the client " << std::endl;
                        close_conn = TRUE;
                        break;
                    }
                } while (TRUE);
                if (close_conn)
                {
                    close (fds[i].fd);
                    fds[i].fd = -1;
                    compress_array = TRUE;
                }
            }
            // i++;
        }
        if (compress_array)
        {
            compress_array = FALSE;
            // i = 0;
            for (i = 0; i < nfds; i++)
            {
                if (fds[i].fd == -1)
                {
                    // j = i;
                    for (j = i; j < nfds-1; j++)
                    {
                        fds[j].fd = fds[j+1].fd;
                        // j++;
                    }
                    i--;
                    nfds--;
                }
                // i++;
            }
        }
    } while (end_server == FALSE);
    
    // i = 0;
    for (i = 0; i < nfds; i++)
    {
        if (fds[i].fd >= 0)
            close (fds[i].fd);
        // i++;
    }
    return (0);
}