#include "chat.h"
#include <malloc.h>
#include <string>
#include <map>
#include <queue>
#include <list>
#include <string.h>

using namespace std;

#define RESULT_DESCR_LEN 128
#define LOGIN_LEN 32
#define PASSWORD_LEN 32
#define MESSAGE_LEN 128

    // ���� ���-�� ������� ������� �� ������������ ��������� ��� ������� ������������. ���������� ��� � ������� �����������, ����� ���� �������, ��� ��������.
    // � ���� �������� ��� ��� � ��� ������� ��������� ���������.
    // �������� ��� ������� ����� ��� ����������� ��������.
    // � ������� ��� ��� ������� ����� �������� � ���� (���� ���-�� ������ ���-�������)
    // ���� ����� �� � ������� ��������� ����� cookie
    // ������ cookie �������� �� ������� � ����� (�������������?) map ������ � ������� � ������� ������������.

/**
 *
 */
class login_list
{
private:
    struct login_list_elem
    {
    public:
        string login;
        string password;
        int cookie;

        login_list_elem *next;
        login_list_elem *previous;
    };

    login_list_elem *head;
    login_list_elem *tail;
    int size;
public:
	login_list () { head = NULL; tail = NULL; size = NULL; }
	~login_list() {
		login_list_elem *iter;
		for (iter = tail->previous; iter != NULL; iter = iter->previous)
		{
			delete iter->next;
		}
		delete head;
	}
    result set_cookie(string login, int cookie);
    void   remove_cookie(string login);
    string find_login(int cookie);
    result reg(string login, string password);
    result get_users_list(users_message *data);
    int    get_size();
    void   setup_from_file(string filename);
	int	   get_pass_hash(string login);
	int    generate_cookie();
};

result login_list::set_cookie(string login, int cookie)
{
	result res;
	login_list_elem *iter = head;
	for (iter = head; iter != NULL; iter = iter->next)
	{
		if (iter->login == login)
		{
			iter->cookie = cookie;
			res.code = OK;
			return res;
		}
	}
	res.code = FAIL;
	strcpy(res.descr, "FAIL: trying to set up cookie for unregistered user\n");
	return res;
}

void login_list::remove_cookie(string login)
{
	login_list_elem *iter = head;
	for (iter = head; iter != NULL; iter = iter->next)
	{
		if (iter->login == login)
		{
			iter->cookie = -1;
			return;
		}
	}
}

string login_list::find_login (int cookie)
{
	login_list_elem *iter = head;
	for (iter = head; iter != NULL; iter = iter->next)
	{
		if (iter->cookie == cookie)
		{
			return iter->login;
		}
	}
	return string("");
}

result login_list::reg(string login, string password)
{
	result res;
	login_list_elem *new_elem = new login_list_elem;
	login_list_elem *iter;
	for (iter = head; iter != NULL; iter = iter->next)
	{
		if (iter->login == login)
		{
			res.code = FAIL;
			strcpy(res.descr, "FAIL: this username is already used.\n");
			return res;
		}
	}

	new_elem->login = login;
	new_elem->password = password;
	new_elem->previous = tail;
	new_elem->next = NULL;

	if (head == NULL)
	{
		head = new_elem;
	}
	else
	{
		tail->next = new_elem;
	}
	tail = new_elem;
	size += 1;

	res.code = OK;
	return res;
}

result login_list::get_users_list(users_message *data)
{
	result res;
	int i = 0;
	login_list_elem *iter;
	for (iter = head; iter != NULL; iter = iter->next)
	{
		strcpy(data[i].login, iter->login.data());
		if (iter->cookie > 0) // if user is online
		{
			data[i].online = ONLINE;
		}
		else
		{
			data[i].online = OFFLINE;
		}
		++i;
	}
	res.code = OK;
	return res;
}

int login_list::get_size()
{
	return size;
}

void login_list::setup_from_file(string filename)
{
	// TODO
}

int	login_list::get_pass_hash(string login)
{
	login_list_elem *iter;
	for (iter = head; iter != NULL; iter = iter->next)
	{
		if (iter->login == login)
		{
			return strlen(iter->password.data());
		}
	}
	return 0;
}

int login_list::generate_cookie()
{
	login_list_elem *iter;
	int my_little_cookie = 1;
	for (iter = head; iter != NULL; iter = iter->next)
	{
		if (iter->cookie > my_little_cookie)
		{
			my_little_cookie = iter->cookie;
		}
	}
	my_little_cookie += 1; // grow to be a big stong cookie, my little cookie;
	return my_little_cookie;
}

/**
 *
 */
typedef queue<receive_message> message_queue;

/**
 *
 */
class message_map : map<string, message_queue>
{
public:
    message_queue &get_queue(string login);
    result pushq(string to_login, receive_message message);
};

message_queue &message_map::get_queue(string login)
{
	return (*this)[login];
}

result message_map::pushq(string to_login, receive_message message)
{
	result res;
	message_queue &que = (*this)[to_login];
	que.push(message);
	res.code = OK;
	return res;
}

/**
 *
 */
static login_list global_login_list;

/**
 *
 */
static message_map global_message_map;

//------ TODO --
int hash_fun(char *pass)
{
	return strlen(pass);
}

//--------------

/**
 * [in] params.to (string)
 * [in] params.cookie (int)
 * [in] params.message (string)
 * �� ��������� to ��������� � ������� �� ������� ����� ��������� + ���� ����� (�������� ��� �� cookie)
 */
result *
send_2_svc(send_params *argp, struct svc_req *rqstp)
{
    result *res = new result;
	send_params params = *argp;
    // �������� ����� �� cookie
    string login = global_login_list.find_login(params.cookie);
    // ������� ���������
    receive_message message;
	message.from = new char[LOGIN_LEN];
	message.message = new char[MESSAGE_LEN];
	strcpy(message.from, login.data());
	strcpy(message.message, params.message);
    // �������� ������� ��������� ��� ���������� � ����� ���� ���������
    *res = global_message_map.pushq(params.to, message); // receive_message
    return res;
}

/**
 * [in] params.login (string)
 * [in] params.password (string)
 * [out] login_result.res (result) #code + description
 * [out] login_result.cookie (int)
 * �������� � ������ � ����� ������� �� �������. ����� ��� �� ������ ������, ������� � �����, ������� �������� � ������. �������, ��� ��� ���������.
 * ��������� �� ������� � ���� ������ � ������� � ������� ����� cookie. ��������� cookie � ������ cookie, ����� ��� �� �����������. 
 * �� ����� �������� ��������, � ��������� �� ���� ������� � ��������, ����� ���������, ��� ��� �� �����������.
 */
login_result *
login_2_svc(login_params *argp, struct svc_req *rqstp)
{
    result res;
	login_params params = *argp;
    login_result *ret = new login_result;
    int client_hash = hash_fun (params.password);
    int server_hash = global_login_list.get_pass_hash(params.login);
    if ((server_hash > client_hash) || (server_hash < client_hash)) // ���� ��� ������ �� ������ � ����������
    {
        res.code = FAIL;
        strcpy(res.descr, "Invalid login + password combination");
        ret->res = res;
        ret->cookie = -1;

        return ret;
    }
    // ���� ��� ������ ��������� � ����������
    // ���������� cookie
    // ��������� cookie � ������ �� �������, ����� ���������� ������������
    // � ��������� ��� �� ������� ��� ������ ������
    int cookie = global_login_list.generate_cookie();
    res = global_login_list.set_cookie(params.login, cookie);

    // ���������� cookie
    ret->res = res;
    ret->cookie = cookie;

    return ret;
}


/** 
 * [in] params.cookie (int)
 * [out] (result) #code + description
 * �� ������� ���� �� ���� ������� � �������� ��� cookie � ������� ��� ������
 * ����� ������� ��� cookie �� ������ cookie �� �������
 */
result *
logout_2_svc(logout_params *argp, struct svc_req *rqstp)
{
    result *res = new result;
	logout_params params = *argp;
	res->code = OK;
    // ���� ����� �� cookie
    string login = global_login_list.find_login(params.cookie);
    global_login_list.remove_cookie(login);
    return res;
}

/**
 * [in] params.cookie (int)
 * [out] receive_result.res (result) #code + description
 * [out] receive_result.receive_message (array of structs):
 *        -- receive_result.receive_message.from (string) 
 *        -- receive_result.receive_message.message (string)
 * ����� �� ������� �� ������� �� ������ (�������� ��� � cookie) ��� ��������� + ��� ����� ��������, ������ �� � ���������� (��� ������ ����� � ���)
 */
receive_result *
receive_2_svc(receive_params *argp, struct svc_req *rqstp)
{
    receive_result *ret = new receive_result;
    // �������� ����� �� cookie
	receive_params params = *argp;
    string login = global_login_list.find_login(params.cookie);
    // �������� ������� ��������� ��� ������
    message_queue &que = global_message_map.get_queue(login);
	ret->data.data_len = que.size();
    // ���������� ������, ����������� ��� ����, ����� �������� ��� ���������.
	ret->data.data_val = new receive_message[ret->data.data_len];
    // ����� ��������� �� ������� � ����� �� � ������
    for (int i = 0; i < ret->data.data_len; ++i)
    {
		ret->data.data_val[i] = que.front();
		que.pop();
    }
    ret->res.code = OK;
    return ret;
}

/**
 * [in] params.cookie (int)
 * [out] users_result.res (result) #code + description
 * [out] users_result.users_message (array of structs):
 *        -- users_result.users_message.login (string)
 *        -- users_result.users_message.online (online status) #enum: ONLINE, OFFLINE
 * ����� �� ������ �������, ���������, ���� �� ��� ������ cookie. ���� �� - ������ ONLINE, ����� - OFFLINE
 */
users_result *
users_2_svc(users_params *argp, struct svc_req *rqstp)
{
	users_result *ret = new users_result;
	users_params params = *argp;
	ret->data.data_len = global_login_list.get_size();
	ret->data.data_val = new users_message[ret->data.data_len];
	for (int i = 0; i < ret->data.data_len; ++i)
	{
		ret->data.data_val[i].login = new char[LOGIN_LEN];
	}
	ret->res = global_login_list.get_users_list(ret->data.data_val);
    return ret;
}

/**
 * [in] params.login (string)
 * [in] params.password (string)
 * [out] result #code + description
 * ������� ����� ���������� � ������� � ������� �� ������� (�� � �����, ����� ����������� ���������), ����� ��� ����� ���� ������� cookie
 * ��� �� � �����, �� ��� ������� ������� ���� ����� ��� ����������, ����� ������ ������������ �����������
 */
result *
register_2_svc(register_params *argp, struct svc_req *rqstp)
{
	result *res = new result;
	register_params params = *argp;
    *res = global_login_list.reg(params.login, params.password);
	return res;
}