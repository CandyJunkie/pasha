#include <iostream>
#include <mutex>
#include <thread>
#include <string>
#include <vector>

#include <unistd.h>
#include "chat.h"

bool startswith(std::string str, char *substr)
{
	return (str.find(std::string (substr), 0) == 0);
}

template <typename Res, typename Params> // Result, Param
Res* get_user_data (CLIENT* cl, Res* (*pfunc) (Params *argp, CLIENT *clnt))
{
	Params params;
	params.login = new char[LOGIN_LEN];
	params.password = new char[PASSWORD_LEN];

	std::cout << "Username: ";
	std::cin >> params.login;
	std::cout << "Password: ";
	std::cin >> params.password;

	Result *result = pfunc (&param, cl);
	if (result == NULL) {
		clnt_perror (cl, "The call has been failed\n");
	}

	return result;
}

void try_login (CLIENT *cl, int &cookie)
{
	std::string str;

	std::cout << "Possble commands: \n";
	std::cout << "  - login\n";
	std::cout << "  - register\n";
	std::cout << "  - quit | exit\n";
	getline(std::cin, str);

	if (startswith(str, "login")) 
	{
		login_result *login_res = get_user_data<login_result, login_params> (cl, &login_2);
		std::cout << "\n" << login_res->res.descr << "\n";
		if (login_res->res.code == OK) 
		{
			cookie = login_res->cookie;
		} 
		else 
		{
			cookie = -1;
		}
	} 
	else if (startswith(str, "register")) 
	{
		cookie = -1;
		result* register_res = get_user_data <result, register_params> (cl, &register_2);
		if (register_res->code == OK) 
		{
			std::cout << "You have successfully registered\n";
		} 
		else 
		{
			std::cout << "FAIL: " << register_res->descr << std::endl;
		}
	} 
	else if (startswith(str, "quit") || startswith(str, "exit")) 
	{
		exit (0);
	} 
	else 
	{
		std::cout << "FAIL: Invalid command.\n";
	}
}


void print_user_list (int &cookie, CLIENT *cl)
{
	users_result *users_res;
	users_param params;

	params.cookie = cookie;
	users_res = users_2 (&params, cl);
	if (users_res == NULL) {
		clnt_perror (cl, "FAIL: call failed");
	}
	if (users_res->res.code == FAIL) {
		std::cout << "FAIL: " << users_res->res.descr << "\n";
		return;
	}

	std::cout << '\n';
	for (int i = 0; i < users_res->data.data_len; ++i) 
	{
		std::cout << users_res->data.data_val[i].login << '\t';
		if (users_res->data.data_val[i].online == ONLINE) 
		{
			std::cout << "ONLINE\n";
		} 
		else 
		{
			std::cout << "OFFLINE\n";
		}
	}
}

void send_message (int &cookie, CLIENT *cl)
{
	send_params params;
	result *result;
	std::string recipient;
	std::string message;

	std::cout << "Write to: ";
	std::cin >> recipient;
	std::cout << "Message: ";
	getline (std::cin, message);
	getline (std::cin, message);

	params.to = new char[LOGIN_LEN];
	params.message = new char[MESSAGE_LEN];
	strcpy (params.to, recipient.data());
	strcpy (params.message, message.data());
	params.cookie = cookie;

	result = send_2 (&params, cl);
	if (result == NULL) {
		clnt_perror (cl, "FAIL: call failed");
	}

	if (result->code == FAIL) {
		std::cout << "FAIL: " << result->descr << '\n';
	}
}

void logout (int &cookie, CLIENT *cl)
{
	logout_params params;
	params.cookie = cookie;

	result *res = logout_2 (&lp, cl);

	cookie = -1;
	while (cookie < 0) 
	{
		try_login (cl, cookie);
	}
}

class message_receiver
{
public:
	static void check_new_messages (int &cookie, CLIENT *cl);
	static void read_messages ();

private:
	static std::mutex lock;
	static std::vector <std::pair <std::string, std::string>> messages;
};

std::vector <std::pair <std::string, std::string>> message_receiver::messages;
std::mutex message_receiver::lock; 

void message_receiver::check_new_messages (int &cookie, CLIENT *cl)
{
	while (1) 
	{
		receive_params params;
		receive_result *recieve_res;

		params.cookie = cookie;
		if (cookie < 0) 
		{
			sleep (5);
			continue;
		}
		recieve_res = receive_2 (&params, cl);
		if (recieve_res->res.code == FAIL) 
		{
			std::cout << "FAIL: " << recieve_res->res.descr << '\n';
			return;
 		}

		lock.lock();
		for (int i = 0; i < recieve_res->data.data_len; ++i) 
		{
			messages.push_back (std::pair <std::string, std::string> (
									std::string (recieve_res->data.data_val[i].from),
									std::string (recieve_res->data.data_val[i].message)));
		}

		lock.unlock();
		if (recieve_res->data.data_len != 0) 
		{
			std::cout << "------ New message ------\n"; 
		}
		sleep (5);
	}
}

void message_receiver::read_messages ()
{
	if (messages.size() == 0) 
	{
		std::cout << "You have no new messages\n";
		return;
	}

	lock.lock();
	for (auto &msg : messages)
		std::cout << "[" << msg->first << "]: " << msg->second << "\n";
	}

	lock.unlock();
	messages.clear();
}

void user_functionality (int &cookie, CLIENT *cl)
{
	std::string str;
	while (true) 
	{
		std::cout << "Possble commands: \n";
		std::cout << "  - userlist\n";
		std::cout << "  - send\n";
		std::cout << "  - receive\n";
		std::cout << "  - logout\n";
		getline(std::cin, str);
		if (startswith(str, "userlist"))
		{
			print_user_list (cookie, cl);
		}
		else if (startswith(str, "send"))
		{
			send_message (cookie, cl);
		}
		else if (startswith(str, "receive"))
		{
			message_receiver::read_messages();
		}
		else if (startswith(str, "logout"))
		{
			logout (cookie, cl);
		}
		else
		{
			std::cout << "FAIL: invalid command\n";
		}
	}
}


int main (int argc, char *argv[])
{
	int cookie = -1;
	CLIENT* cl;
	cl = clnt_create("localhost", RPC_CHAT, RPC_CHAT_VERSION_2, "udp");

	while (cookie < 0) 
	{
		try_login (cl, cookie);
	}

	std::thread message_thread (message_receiver::check_new_messages, std::ref (cookie), cl);
	std::thread user_thread	  (user_functionality, std::ref (cookie), cl);

	if (message_thread.joinable())
	{
		message_thread.join();
	}
	if (user_thread.joinable())
	{
		user_thread.join();
	}

	return 0;
}
