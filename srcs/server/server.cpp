/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ybensell <ybensell@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/16 10:53:11 by mbabela           #+#    #+#             */
/*   Updated: 2022/08/31 12:55:41 by ybensell         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "server.hpp"

/*************************[ Constructors/Destructors ]*************************/

Server::Server(int _port, std::string _password)
{
	std::cout << "Creating Server . . . " << std::endl;
	std::cout << "updating parameter . . . " << std::endl;
	this->port = _port;
	this->password = _password;
	this->nfds = 0;
	std::cout << "Server created, password : " << this->password << std::endl;
}

Server::~Server(void)
{
	std::map<int, User *>::iterator guest;
	for (guest = this->guests.begin(); guest != this->guests.end(); ++guest) {
		delete guest->second;
	}
	this->guests.clear();

	std::map<std::string, User *>::iterator user;
	for (user = this->users.begin(); user != this->users.end(); ++user) {
		delete user->second;
	}
	this->users.clear();

	std::map<std::string, Channel *>::iterator channel;
	for (channel = this->channels.begin(); channel != this->channels.end(); ++channel) {
		delete channel->second;
	}
	this->channels.clear();
	
	std::cout << "Server deleted." << std::endl;
}

/******************************[ Getters/Setters ]*****************************/

int		Server::getSocketFd(void) const {
	return (this->socket_fd);
}

struct pollfd *	Server::getFds(void) {
	return (this->fds);
}

int		Server::getNfds(void) const {
	return (this->nfds);
}

void	Server::setNfds(int nfds) {
	this->nfds = nfds;
}

int		Server::getPort(void) const {
	return (this->port);
}

std::string const &	Server::getPass(void) const {
	return (this->password);
}

std::map<int, User *> &	Server::getGuests(void) {
	return (this->guests);
}

std::map<std::string, User *> &	Server::getUsers(void) {
	return (this->users);
}

std::map<std::string, Channel *> &	Server::getChannels(void) {
	return (this->channels);
}

User *	Server::getUser(int fd) {
	std::map<int, User *>::iterator 		guest;
	std::map<std::string, User *>::iterator	user;
	
	guest = this->guests.find(fd);
	if (guest != this->guests.end()) {
		return (guest->second);
	}

	for (user = this->users.begin(); user != this->users.end(); ++user) {
		if (user->second->getFd() == fd) {
			return (user->second);
		}
	}
	return (NULL);
}

Channel *	Server::getChannel(std::string name) {
	std::map<std::string, Channel *>::iterator	channel;
	
	channel = this->channels.find(name);
	if (channel != this->channels.end()) {
		return (channel->second);
	}
	return (NULL);
}

/*****************************[ Users Management ]*****************************/

void	Server::addGuest(int fd) {
	User *	guest;

	guest = new User(fd);
	this->guests.insert(std::pair<int, User *>(fd, guest));
}

void	Server::registerUser(int fd) {
	std::map<int, User *>::iterator	guest;
	User *							user;
	
	guest = this->guests.find(fd);
	if (guest == this->guests.end())
		return ;
	user = guest->second;
	this->users.insert(std::pair<std::string, User *>(user->getUsername(), user));
	this->guests.erase(guest);
}

void	Server::clientDisconnect(int fd) {
	std::map<int, User *>::iterator				guest;
	std::map<std::string, User *>::iterator		user;
	std::map<std::string, Channel *>			channels;
	std::map<std::string, Channel *>::iterator	channel;

	guest = this->guests.find(fd);
	if (guest != this->guests.end()) {
		delete guest->second;
		this->guests.erase(guest);
		return ;
	}

	for (user = this->users.begin(); user != this->users.end(); ++user) {
		if (user->second->getFd() == fd) {
			user->second->setFd(-1);
			channels = user->second->getChannels();
			while (!channels.empty()) {
				channel = channels.begin();
				user->second->leaveChannel(channel->second->getName());
			}
			return ;
		}
	}
}

/****************************[ Channels Management ]***************************/

void	Server::createChannel(std::string name, User & op) {
	Channel *	channel;

	channel = new Channel(name);
	if (this->channels.insert(std::pair<std::string, Channel *>(name, channel)).second) {
		op.joinChannel(*channel);
		return ;
	}
	delete channel;
}

void	Server::deleteChannel(std::string name) {
	std::map<std::string, Channel *>::iterator	channel;
	std::map<std::string, User *>::iterator 	member;
	std::map<std::string, User *>				channelMembers;
	std::map<std::string, Channel *>			memberChannels;

	channel = this->channels.find(name);
	if (channel == this->channels.end())
		return ;
	channelMembers = channel->second->getMembers();
	for (member = channelMembers.begin(); member != channelMembers.end(); ++member) {
		memberChannels = member->second->getChannels();
		if (memberChannels.find(name) != memberChannels.end()) {
			memberChannels.erase(memberChannels.find(name));
		}
	}
	delete channel->second;
	this->channels.erase(channel);
}

/*****************************[ Server Management ]****************************/

int		Server::Create_socket(void)
{
	std::cout << "Creating socket . . . ";
	this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->socket_fd < 0)
	{
		std::cout << "[FAILED: " << errno << "]" << std::endl;
		return (0);
	}
	this->fds[0].fd = this->socket_fd;
	this->nfds++;
	std::cout << "[DONE]" << std::endl;
	return (1);
}

int		Server::reusable_socket(void)
{
	int	rc;
	
	std::cout << "Setting up socket options . . . ";
	rc = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&this->on, sizeof(this->on));
	if (rc < 0)
	{
		std::cout << "[FAILED: " << errno << "]" << std::endl;
		return (0); 
	}
	std::cout << "[DONE]" << std::endl;
	return (1);
}

int		Server::nonblocking_socket(void)
{
	int	rc;
	
	std::cout << "Making socket non blocking . . . ";
	rc = fcntl(this->socket_fd, F_SETFL, O_NONBLOCK);
	if (rc < 0)
	{
		std::cout << "[FAILED: " << errno << "]" << std::endl;
		return (0);
	}
	std::cout << "[DONE]" << std::endl;
	return (1);
}

int		Server::bind_socket(void)
{
	struct sockaddr_in	addr;
	int					rc;
	
	std::cout << "Binding Socket . . . ";
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(this->port);
	rc = bind(this->socket_fd, (struct sockaddr *)&addr, sizeof(addr));
	if (rc < 0)
	{
		std::cout << "[FAILED: " << errno << "]" << std::endl;
		return (0);
	}
	std::cout << "[DONE]" << std::endl;
	return (1);
}

int		Server::listen_from_socket(void)
{
	int	rc;
	
	std::cout << "Listening . . ." << std::endl;
	rc = listen(this->socket_fd, MAX_CONN);
	if (rc < 0)
	{
		std::cout << "[FAILED: " << errno << "]" << std::endl;
		return (0);
	}
	return (1);
}

void	Server::poll_trait(void)
{
	std::cout << "Setting up poll structure . . . ";
	memset(this->fds, 0, sizeof(this->fds));
	this->fds[0].fd = this->socket_fd;
	this->fds[0].events = POLLIN;
	std::cout << "[DONE]" << std::endl;
}

bool	Server::accept_connections(void)
{
	int	new_fd = -1;
	
	std::cout << "Waiting for incoming connections . . . " << std::endl;
	do
	{
		new_fd = accept(this->socket_fd, NULL, NULL);
		if (new_fd == -1)
		{
			if (errno != EWOULDBLOCK)
			{
				std::cout << "FAILED at accepting connection ! errno : " << errno << std::endl;
				return (false);
			}
			break;
		}
		std::cout << "NEW Connection detected " << new_fd << std::endl;
		this->addGuest(new_fd);
		this->fds[this->nfds].fd = new_fd;
		this->fds[this->nfds].events = POLLIN;
		this->nfds++;
	} while (new_fd != -1);
	return (true);
}		

void Server::splitCmd(std::string &cmd,std::vector<std::string> &oneCmdParsed)
{
	std::vector<std::string> collonSplit;  
	//I split first the command with ':'
   		// then I split with spaces		

	split(cmd,':',collonSplit);
	split(collonSplit[0],' ',oneCmdParsed);
	for (size_t i = 1 ; i < collonSplit.size();i++)
		oneCmdParsed.push_back(collonSplit[i]);
};

void	Server::parsExecCommands(Msg &msg)
{
	std::vector<std::string> allCmds;
	std::vector<std::string> oneCmdParsed;

	allCmds = msg.getCommands();
	std::cout << "******************************************" << std::endl;
	for (size_t i = 0 ; i < allCmds.size() ;i++)
	{
		splitCmd(allCmds[i],oneCmdParsed);
		std::cout << "            Command " << i << ":" << std::endl;
		
		for (size_t i = 0 ; i < oneCmdParsed.size(); i++)
		{
			std::cout << oneCmdParsed[i] << std::endl;
		}
		std::cout << "---------------------------------" << std::endl;
		oneCmdParsed.clear();
	};
	std::cout << "******************************************" << std::endl;

};




void Server::checkMsg(Msg &msg)
{
	// Might Put command on a list and check them instead of If else
	std::vector<std::string> parsedMsg;
	std::map <int, User *>::iterator it;
	
	parsedMsg = msg.getParsedMsg();
	if (!msg.get_cmd().compare("USER"))
	{
		if (parsedMsg.size() < 5)
			send(msg.get_sender(), "Error need more parameters\n", 
				sizeof("Error need more parameters\n"), 0);
		else
		{
			it = this->guests.find(msg.get_sender());
			it->second->setUsername(parsedMsg[1]);
			it->second->setHostName(parsedMsg[2]);
			it->second->setServerName(parsedMsg[3]);
			it->second->setFullName(parsedMsg[4]);
		}
	}
	if (!msg.get_cmd().compare("NICK"))
	{
		if (parsedMsg.size() < 2)
			send(msg.get_sender(), "Error need more parameters\n", 
				sizeof("Error need more parameters\n"), 0);
		else
		{
			it = this->guests.find(msg.get_sender());
			it->second->setNickname(parsedMsg[1]);
		}
	}
}

bool	Server::recv_send_msg(int fd)
{
	int	rc = 1;
	std::string buff;
	std::cout <<  "Receiving message . . ." << std::endl;
	do
	{
		// we should read until we get (CR or LF OR both IN THE BUFFER)
		while (buff.find_first_of("\r\n") == std::string::npos)
		{
			rc = recv(fd, this->buffer, sizeof(this->buffer), 0);
			if (rc > 0)
			{
				this->buffer[rc] = '\0';
				buff +=  this->buffer;
			}
			if (rc == -1)
			{
				if (errno != EWOULDBLOCK)
				{
					std::cout << "FAILED at receiving a msg ! errno : " << errno << std::endl;
					return (false);
				}
			}
			if (rc == 0)
			{
				std::cout << "Connection closed . . . " << std::endl;
				return (false);
			}
			
		}
		// for (size_t i = 0 ; i < buff.length() ; i++)
		// {
		// 	std::cout << std::hex << (int)buff[i] << "-"; 
		// }
		Msg msg = Msg(buff, fd);
		parsExecCommands(msg);
		//checkMsg(msg);
		// this for testing 
		// std::map <int, User *>::iterator it;
		// it = this->guests.find(fd);
		// std::cout << it->second->getUsername() << std::endl;
		// std::cout << it->second->getNickname() << std::endl;
		// std::cout << it->second->getHostName() << std::endl;
		// std::cout << it->second->getServerName() << std::endl;
		// std::cout << it->second->getFullName() << std::endl;
		send(fd, "received succ >.<\n", sizeof("received succ >.<\n"), 0);
		return (true);
	} while (true);
	return (true);
}
