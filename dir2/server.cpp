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

    // Надо как-то хранить очередь из неполученных сообщений для каждого пользователя. Желательно еще с логином отправителя, чтобы было понятно, кто отправил.
    // И этой функцией как раз в эту очередь добавлять сообщение.
    // сетапить эту очередь можно при регистрации например.
    // А хранить все эти очереди можно например в мапе (лень как-то делать хэш-функцию)
    // свой логин мы в очередь вставляем через cookie
    // Каждый cookie хранится на сервере и через (двухсторонний?) map связан с логином и паролем пользователя.

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
 * По параметру to добавляем в очередь на сервере новое сообщение + свой логин (получаем его из cookie)
 */
result *
send_2_svc(send_params *argp, struct svc_req *rqstp)
{
    result *res = new result;
	send_params params = *argp;
    // Получаем логин по cookie
    string login = global_login_list.find_login(params.cookie);
    // Создаем сообщение
    receive_message message;
	message.from = new char[LOGIN_LEN];
	message.message = new char[MESSAGE_LEN];
	strcpy(message.from, login.data());
	strcpy(message.message, params.message);
    // Получаем очередь сообщений для получателя и пишем туда сообщение
    *res = global_message_map.pushq(params.to, message); // receive_message
    return res;
}

/**
 * [in] params.login (string)
 * [in] params.password (string)
 * [out] login_result.res (result) #code + description
 * [out] login_result.cookie (int)
 * Залезаем в ячейку с нашим логином на сервере. Берем хэш от нашего пароля, сверяем с хэшом, который хранится в ячейке. Ожидаем, что они совпадают.
 * Сохраняем на сервере в нашу ячейку с логином и паролем новый cookie. Добавляем cookie в список cookie, чтобы они не повторялись. 
 * Их можно генерить рандомно, и пробегать по всем ячейкам с логинами, чтобы проверять, что они не повторяются.
 */
login_result *
login_2_svc(login_params *argp, struct svc_req *rqstp)
{
    result res;
	login_params params = *argp;
    login_result *ret = new login_result;
    int client_hash = hash_fun (params.password);
    int server_hash = global_login_list.get_pass_hash(params.login);
    if ((server_hash > client_hash) || (server_hash < client_hash)) // Если хэш пароля не совпал с записанным
    {
        res.code = FAIL;
        strcpy(res.descr, "Invalid login + password combination");
        ret->res = res;
        ret->cookie = -1;

        return ret;
    }
    // Если хэш пароля совпадает с записанным
    // Генерируем cookie
    // Добавляем cookie в список на сервере, чтобы обеспечить уникальность
    // и сохраняем его на сервере для нашего логина
    int cookie = global_login_list.generate_cookie();
    res = global_login_list.set_cookie(params.login, cookie);

    // Возвращаем cookie
    ret->res = res;
    ret->cookie = cookie;

    return ret;
}


/** 
 * [in] params.cookie (int)
 * [out] (result) #code + description
 * На сервере ищем по всем ячейкам с логинами наш cookie и удаляем его оттуда
 * Также удаляем наш cookie из списка cookie на сервере
 */
result *
logout_2_svc(logout_params *argp, struct svc_req *rqstp)
{
    result *res = new result;
	logout_params params = *argp;
	res->code = OK;
    // Ищем логин по cookie
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
 * Берем из очереди на сервере по логину (получаем его с cookie) все сообщения + кто какое отправил, пакуем их в структурку (или храним прямо в них)
 */
receive_result *
receive_2_svc(receive_params *argp, struct svc_req *rqstp)
{
    receive_result *ret = new receive_result;
    // Получаем логин по cookie
	receive_params params = *argp;
    string login = global_login_list.find_login(params.cookie);
    // Получаем очередь сообщений для логина
    message_queue &que = global_message_map.get_queue(login);
	ret->data.data_len = que.size();
    // Аллоцируем массив, достаточный для того, чтобы вместить все сообщения.
	ret->data.data_val = new receive_message[ret->data.data_len];
    // Берем сообщения из очереди и пишем их в массив
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
 * Ходим по списку логинов, проверяем, есть ли там сейчас cookie. Если да - выдаем ONLINE, иначе - OFFLINE
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
 * Создаем новую структурку с логином и паролем на сервере (мб в файле, чтобы сохранялось нормально), чтобы там можно было хранить cookie
 * Раз уж в файле, то при запуске сервера надо будет его подгружать, чтобы старые пользователи сохранялись
 */
result *
register_2_svc(register_params *argp, struct svc_req *rqstp)
{
	result *res = new result;
	register_params params = *argp;
    *res = global_login_list.reg(params.login, params.password);
	return res;
}