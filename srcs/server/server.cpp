/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: hel-makh <hel-makh@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/16 10:53:11 by mbabela           #+#    #+#             */
/*   Updated: 2022/09/27 10:20:05 by hel-makh         ###   ########.fr       */
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
	this->operators.insert(std::pair<std::string,std::string>("penguin","messi123"));
	this->operators.insert(std::pair<std::string,std::string>("darkspiper","maroc2001"));
	this->operators.insert(std::pair<std::string,std::string>("naahio","azerty12"));
	this->name = ":IRC-1337 ";
	this->version = "1.0 ";

	time_t now = time(0);
   	this->creationTime = ctime(&now);
	std::cout << "Server created, password : " << this->password << std::endl;
}

Server::~Server(void)
{
	std::map<int, User *>::iterator user;
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

std::map<int, User *> &	Server::getUsers(void) {
	return (this->users);
}

std::map <int, User *> & Server::getGuests(void)
{
	return (this->guests);
}

std::map<std::string, Channel *> &	Server::getChannels(void) {
	return (this->channels);
}

std::map <std::string, std::string> &  Server::getOperators(void){
	return (this->operators);
}

User *	Server::getUser(int fd) {
	std::map<int, User *>::iterator	user;

	user = this->users.find(fd);
	if (user != this->users.end()) {
		return (user->second);
	}
	return (NULL);
}

User *	Server::getUser(std::string nickname) {
	std::map<int, User *>::iterator	user;

	for (user = this->users.begin(); user != this->users.end(); ++user) {
		if (user->second->getNickname() == nickname) {
			return (user->second);
		}
	}
	return (NULL);
}

User	*	Server::getGuest(int fd)
{
	std::map<int, User *>::iterator	user;

	user = this->guests.find(fd);
	if (user != this->guests.end()) {
		return (user->second);
	}
	return (NULL);
}

User	*	Server::getGuest(std::string nickname)
{
	std::map<int, User *>::iterator	user;

	for (user = this->guests.begin(); user != this->guests.end(); ++user) {
		if (user->second->getNickname() == nickname) {
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

std::string const &	Server::getName(void) const {
	return (this->name);
}


std::string const & Server::getVersion(void) const
{
	return (this->version);
}

/*****************************[ Users Management ]*****************************/

void	Server::addUser(int fd,User *user) {
	
	this->users.insert(std::pair<int, User *>(fd, user));
}

void	Server::addGuest(int fd,char *ip, char *postname)
{
	User *	user;

	user = new User(fd, ip, postname);
	this->guests.insert(std::pair<int, User *>(fd, user));
}

void	Server::clientDisconnect(int fd) {
	try {
		std::map<int, User *>::iterator		user;
		std::map<std::string, Channel *>	channels;
		std::map<std::string, Channel *>::iterator	it;

		user = this->users.find(fd);
		if (user != this->users.end()) {
			channels = user->second->getChannels();
			while (!channels.empty()) {
				channels.begin()->second->removeMember(fd);
				channels = user->second->getChannels();
			}
			delete user->second;
			this->users.erase(user);
		}
		else
		{
			user = this->guests.find(fd);
			if (user != this->guests.end()) {
				delete user->second;
				this->guests.erase(user);
			}
		}
	} catch (myException & e) {}
}

void	Server::listUserModes(User * user, int fd) {
	std::string	reply;
	std::string	reply2;

	reply = this->name + ft_tostring(RPL_UMODEIS) + " " + user->getNickname() + " +";
	if (user->isVisible())
		reply += "i";
	reply2 += "\n";
	sendReply(fd, reply + reply2);
}

/****************************[ Channels Management ]***************************/

void	Server::createChannel(std::string name, User & op) {
	try {
		Channel *	channel;

		channel = new Channel(name);
		if (this->channels.insert(std::pair<std::string, Channel *>(name, channel)).second) {
			channel->addMember(&op);
			return ;
		}
		delete channel;
	} catch (myException & e) {
		throw myException(e.getERROR_NO());
	}
}

void	Server::deleteChannel(std::string name) {
	std::map<std::string, Channel *>::iterator	channel;
	std::map<int, User *>::iterator 			member;
	std::map<int, User *>						channelMembers;
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

void	Server::listChannelModes(Channel * channel, int fd) {
	User *		user;
	std::string	reply;
	std::string	reply2;

	user = this->getUser(fd);
	if (!user)
		return ;
	reply = this->name + ft_tostring(RPL_CHANNELMODEIS) + " " + user->getNickname() + " " + channel->getName() + " +";
	if (channel->isPrivate())
		reply += "p";
	if (channel->isSecret())
		reply += "s";
	if (channel->isInviteOnly())
		reply += "i";
	if (!channel->isTopicSettable())
		reply += "t";
	if (channel->isMemberChatOnly())
		reply += "n";
	if (channel->isModerated())
		reply += "m";
	if (channel->getMembersLimit() != 0) {
		reply += "l";
		reply2 += " " + ft_tostring(channel->getMembersLimit());
	}
	if (channel->getKey() != "") {
		reply += "k";
		reply2 += " " + channel->getKey();
	}
	reply2 += "\n";
	sendReply(fd, reply + reply2);
	sendReply(fd, this->name
		+ ft_tostring(RPL_CREATIONTIME) + " "
		+ user->getNickname() + " "
		+ channel->getName() + " "
		+ ft_tostring(channel->getCreationTimestamp()) + "\n");
}

void	Server::listChannelBans(Channel * channel, int fd) {
	User *							user;
	std::string						replyMessage;
	std::vector<t_bans>				bans;
	std::vector<t_bans>::iterator	it;

	user = this->getUser(fd);
	if (!user)
		return ;
	bans = channel->getBans();
	for (it = bans.begin(); it != bans.end(); ++it) {
		replyMessage += this->name
			+ ft_tostring(RPL_BANLIST) + " "
			+ user->getNickname() + " "
			+ channel->getName() + " "
			+ it->banMask + " "
			+ it->banMod + " "
			+ ft_tostring(it->banTimestamp) + "\n";
	}
	replyMessage += this->name
		+ ft_tostring(RPL_ENDOFBANLIST) + " "
		+ user->getNickname() + " "
		+ channel->getName() + " "
		+ reply(RPL_ENDOFBANLIST) + "\n";
	sendReply(fd, replyMessage);
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
	rc = setsockopt(this->socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&this->on, sizeof(this->on));
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
	rc = listen(this->socket_fd, MAX_CONN + 1);
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
	struct sockaddr_in	addr;
	int addrlen = sizeof(addr);
	int	new_fd = -1;
	std::cout << "Waiting for incoming connections . . . " << std::endl;
	do
	{
		new_fd = accept(this->socket_fd,(struct sockaddr*)&addr,(socklen_t*)&addrlen);
		if (new_fd == -1)
		{
			if (errno != EWOULDBLOCK)
			{
				std::cout << "FAILED at accepting connection ! errno : " << errno << std::endl;
				return (false);
			}
			break;
		}
		
		struct in_addr ipAddr = addr.sin_addr; 
		char str[INET_ADDRSTRLEN];
		inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN);
        struct hostent *hp;


  		hp = gethostbyaddr((const void *)&ipAddr, sizeof ipAddr, AF_INET);
		
		std::cout << "NEW Connection detected " << new_fd << std::endl;
		if (this->nfds <= MAX_CONN)
		{
			this->addGuest(new_fd,str, hp->h_name);
			this->fds[this->nfds].fd = new_fd;
			this->fds[this->nfds].events = POLLIN;
			this->nfds++;
			sendReply(new_fd, this->name + "NOTICE AUTH :*** Looking up your hostname...\n");
			sendReply(new_fd, this->name + "NOTICE AUTH :*** Found your hostname\n");
		}
		else
		{
			sendReply(new_fd, this->name + "ERROR ERROR :*** SORRY ! NO SPACE LEFT ON SERVER\n");
			std::cout << "Connection rejected : no space left ! " << new_fd << std::endl;
			close(new_fd);
		}
	} while (new_fd != -1);
	return (true);
}	
/********************************[ Parsing ]**********************************/

bool	Server::ctcpMessage(std::string &cmd,
					std::vector<std::string> &vec)
{
	split(cmd,' ',vec);
	if (vec.size() >= 5 && vec[0][0] == 0x01 && 
		vec[0].find("DCC") != std::string::npos 
		&& !vec[1].compare("SEND"))
	{
		return true;
	}
	return false;
}

int		Server::splitCmd(std::string &cmd,std::vector<std::string> &oneCmdParsed)
{
	std::vector<std::string> collonSplit;  

	split(cmd,':',collonSplit);
	if (!collonSplit.size())
		return 0;
	split(collonSplit[0],' ',oneCmdParsed);
	for (size_t i = 1 ; i < collonSplit.size(); i++)
		oneCmdParsed.push_back(collonSplit[i]);
	return 1;
}

void	Server::parsExecCommands(Msg &msg)
{
	std::vector<std::string> allCmds;
	std::vector<std::string> oneCmdParsed;

	allCmds = msg.getCommands();
	for (size_t i = 0 ; i < allCmds.size() ;i++)
	{
		if (!splitCmd(allCmds[i],oneCmdParsed))
			return ;	
		for (size_t i = 0 ; i < oneCmdParsed.size(); i++)
		{
			std::cout << oneCmdParsed[i] << std::endl;
		}
		cmdExec(msg,oneCmdParsed);
		oneCmdParsed.clear();
	}
}

void	Server::cmdExec(Msg &msg,std::vector<std::string> &cmd)
{
	User	*user;

	user = this->getUser(msg.getSender());
	if (!user)
	{
		user = this->getGuest(msg.getSender());
		if (!user)
			return;
	}
	try {
		for (int i = 0 ; cmd[0][i] ; i++)
			cmd[0][i] = toupper(cmd[0][i]);
			//+ c,]mdpoint
		if (!cmd[0].compare("HELP"))
			helps(msg.getSender());
		else if (!cmd[0].compare("USER"))
			USERcmd(msg.getSender(), cmd);
		else if (!cmd[0].compare("NICK"))
			NICKcmd(msg.getSender(), cmd);
		else if (!cmd[0].compare("PASS"))
			PASScmd(msg.getSender(), cmd);
		else if (!cmd[0].compare("QUIT"))
			QUITcmd(msg.getSender(), cmd);
		else if (!cmd[0].compare("VERSION"))
			VERSIONcmd(msg.getSender());
		else if (!cmd[0].compare("TIME"))
			TIMEcmd(msg.getSender());
		else if (!cmd[0].compare("ADMIN"))
			ADMINcmd(msg.getSender());
		else if (user && user->isAuth())
		{
			if (!cmd[0].compare("NOTICE"))
				PRIVMSGcmd(msg.getSender(), cmd, true);
			else if (!cmd[0].compare("PRIVMSG"))
				PRIVMSGcmd(msg.getSender(), cmd);
			else if (!cmd[0].compare("JOIN"))
				JOINcmd(msg.getSender(), cmd);
			else if (!cmd[0].compare("KICK"))
				kick(msg.getSender(), cmd);
			else if (!cmd[0].compare("PART"))
				part(msg.getSender(), cmd);
			else if (!cmd[0].compare("MODE"))
				mode(msg.getSender(), cmd);
			else if (!cmd[0].compare("LIST"))
				list(msg.getSender(), cmd);
			else if (!cmd[0].compare("NAMES"))
				names(msg.getSender(), cmd);
			else if (!cmd[0].compare("INVITE"))
				INVITEcmd(msg.getSender(), cmd);
			else if (!cmd[0].compare("OPER"))
				OPERcmd(msg.getSender(), cmd);
			else if (!cmd[0].compare("KILL"))
				KILLcmd(msg.getSender(), cmd);
			else if (!cmd[0].compare("TOPIC"))
				topic(msg.getSender(), cmd);
			else if (!cmd[0].compare("SEND"))
				SENDcmd(msg.getSender(),cmd);
			else if (!cmd[0].compare("ACCEPT")||
					!cmd[0].compare("DECLINE"))
				RESPONDcmd(msg.getSender(),cmd);
			// else if (!cmd[0].compare("PONG"))
			//  	sendReply(msg.getSender(), stringBuilder(3, this->getName().c_str(), "PING ", this->getName().c_str()));
			else
				throw myException(ERR_UNKNOWNCOMMAND);

		}
	} catch(myException & e) {
		sendReply(msg.getSender(), this->getName()
			+ ft_tostring(e.getERROR_NO()) + " "
			+ user->getNickname() + " "
			+ cmd[0] + " "
			+ e.what() + "\n");
	}
}

/*****************************[ Receive Message ]****************************/

bool	Server::recv_send_msg(int fd)
{
	int	rc = 1;
	std::string buff;
	std::string remain;
	char		buffer[BUFF_SIZE];
	User *		user;

	user = this->getUser(fd);
	if (!user)
	{
		if (!(user = this->getGuest(fd)))
			return false;
	}
	std::cout <<  "Receiving message . . ." << std::endl;
	buff += user->getMsgRemainder();
	memset(buffer,0,BUFF_SIZE);
	do 
	{
		while (buff.find_first_of("\r\n") == std::string::npos)
		{
			rc = recv(fd,buffer,510, 0);
			if (rc == -1)
			{
				if (errno != EWOULDBLOCK)
				{
					std::cout << "FAILED at receiving a msg ! errno : " << errno << std::endl;
					return (false);
				}
				continue;
			}
			if (rc == 0)
			{
				std::cout << "Connection closed . . . " << std::endl;
				return (false);
			}
			buffer[rc] = '\0';
			buff += buffer;
			std::cout << "rc : " <<  rc  << std::endl;
		}
		std::cout << " >>>>> "<< buffer << std::endl;
		size_t pos = buff.find_last_of("\r\n");
		buff = buff.substr(0, pos);
		user->setMsgRemainder(remain);
		Msg msg = Msg(buff, fd);
		parsExecCommands(msg);
		return (true);
	} while (true);
	return (true);
}
