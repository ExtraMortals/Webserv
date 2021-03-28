/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParse.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jinkim <jinkim@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/03/15 02:05:47 by jinkim            #+#    #+#             */
/*   Updated: 2021/03/25 00:01:14 by jinkim           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGPARSE_HPP
# define CONFIGPARSE_HPP

# include <iostream>
# include <string>
# include <map>
# include <vector>
# include <fcntl.h>
# include <unistd.h>

# define CONFIG_PATH "/Users/jinkim/webserv/config/config2.ini"

class ConfigParse{
private:
	void sectionParse(std::string str);
	void serverParse(std::string *section);
	void locationParse(std::string *section);

public:
	ConfigParse();
	virtual ~ConfigParse();

	class FileNotOpenException: public std::exception {
		virtual const char *what() const throw();
	};

	class InvalidConfigException: public std::exception {
		virtual const char *what() const throw();
	};

	typedef struct s_location
	{
		std::string	root;
		std::vector<std::string> index;
		std::vector<std::string> method;
		std::string	cgi;
		bool		autoindex;

	}t_location;

	typedef struct s_server
	{
		int			port;
		std::string	host;
		std::string	name;
		std::string	client_max_body_size;
		std::string	error_root;
		std::map<int, std::string> error_page;
		t_location	loca;
	}t_server;

	std::map<std::string ,t_location> loca_map;
	t_server *server;

	std::string *splitString(std::string str, char c);
};

#endif