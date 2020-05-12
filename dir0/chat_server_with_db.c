#include "chat_server_with_db.h"

#include <iostream>


result *chat_server_with_db::send_2_svc (send_params *argp, struct svc_req *rqstp)
{
    static result  result;
    PGresult *pg_res;
    std::string req;

    std::cout << "sending message, id = " << argp->cookie << '\n';
    req = "SELECT id FROM users WHERE login = '" + std::string (argp->to) + "';";
    result.descr = new char[RESULT_DESCR_LEN];

    if (!complete_request (req, pg_res)) 
	{
        result.code = FAIL;
        strcpy (result.descr, "FAIL: sending message, database error");
        return &result;
    }
    if (PQntuples (pg_res) != 1) 
	{
        result.code = FAIL;
        strcpy (result.descr, "FAIL: sending message, user not found");
        return &result;
    }

    std::string id = std::string (PQgetvalue (pg_res, 0, 0));
    PQclear (pg_res);
    
    req = "INSERT INTO messages VALUES (" + std::to_string (argp->cookie) + ", " + id + ", '" + std::string (argp->message) + "');";
    if (!complete_request (req, pg_res)) 
	{
        result.code = FAIL;
        strcpy (result.descr, "FAIL: sending message, database error");
        return &result;
    }
    result.code = OK;
    strcpy (result.descr, "sending message, success");

    return &result;
}

login_result *chat_server_with_db::login_2_svc (login_params *argp, struct svc_req *rqstp)
{
    static login_result  login_res;
	PGresult *pg_res;
    std::string req;

	std::cout << "logging in: " << argp->login << " on server\n";
	req = "SELECT id FROM users WHERE login ='" + std::string (argp->login) +
		  "' AND password = '" + std::string (argp->password) + "';";
	login_res.res.descr = new char[RESULT_DESCR_LEN];
	if (complete_request (req, pg_res)) 
	{
        if (PQntuples (pg_res) == 1) 
		{
			login_res.cookie = atoi (PQgetvalue (pg_res, 0, 0));
            PQclear (pg_res);
            req = "UPDATE users SET online = TRUE WHERE login = '" + std::string (argp->login) + "';";
			if (!complete_request (req, pg_res)) 
			{
                login_res.res.code = FAIL;
                strcpy (login_res.res.descr, "FAIL: logging in, database error");
                login_res.cookie = -1;
                return &login_res;
            }
            login_res.res.code = OK;
			strcpy (login_res.res.descr, "logging in, success");

		} 
		else 
		{
			login_res.res.code = FAIL;
			strcpy (login_res.res.descr, "FAIL: logging in, login or password is incorrect");
			login_res.cookie = -1;
		}
	} 
	else 
	{
		login_res.res.code = FAIL;
		strcpy (login_res.res.descr, "FAIL: logging in, database error");
		login_res.cookie = -1; 
	}
	PQclear (pg_res);

	return &login_res;
}

result* chat_server_with_db::logout_2_svc (logout_params *argp, struct svc_req *rqstp)
{
    static result result;
    PGresult* pg_res;
    std::string req;

    std::cout << "logging out, id = " << argp->cookie << '\n';
    req = "UPDATE users SET online = FALSE WHERE id = " + std::to_string (argp->cookie) + ";";
    result.descr = new char[RESULT_DESCR_LEN];
    if (!complete_request (req, pg_res)) 
	{
        result.code = FAIL;
        strcpy (result.descr, "FAIL: logging out, database error");
    } 
	else 
	{
        result.code = OK;
        strcpy (result.descr, "logging out, success");
    }
    PQclear (pg_res);

    return &result;
}

receive_result* chat_server_with_db::receive_2_svc (receive_params *argp, struct svc_req *rqstp)
{
	static receive_result  result;
    PGresult* pg_res;
    std::string req;

    std::cout << "receiving messages, id = " << argp->cookie << '\n';
    req = "SELECT users.login, messages.message FROM users INNER JOIN messages ON users.id = messages.sender "
          "WHERE messages.recipient = " + std::to_string (argp->cookie) + ";";
    result.res.descr = new char[RESULT_DESCR_LEN];
    if (!complete_request (req, pg_res)) 
	{
        result.res.code = FAIL;
        strcpy (result.res.descr, "FAIL: receiving messages, database error");
        result.data.data_len = 0;
        return &result;
    }
    int len = PQntuples (pg_res);
    result.data.data_len = len;
    result.data.data_val = new receive_message[len];
    for (int i = 0; i < len; ++i) 
	{
        result.data.data_val[i].from = new char[LOGIN_LEN];
        strcpy (result.data.data_val[i].from, PQgetvalue (pg_res, i, 0));
        result.data.data_val[i].message = new char[MESSAGE_LEN];
        strcpy (result.data.data_val[i].message, PQgetvalue (pg_res, i, 1));
    }
    result.res.code = OK;
    strcpy (result.res.descr, "receiving messages, success");
    PQclear (pg_res);

    req = "DELETE FROM messages WHERE recipient = " + std::to_string (argp->cookie) + ";";
    if (!complete_request (req, pg_res)) 
	{
        result.res.code = FAIL;
        strcpy (result.res.descr, "FAIL: receiving messages, database error");
        result.data.data_len = 0;
    }
    PQclear (pg_res);

	return &result;
}

users_result* chat_server_with_db::users_2_svc (users_param *argp, struct svc_req *rqstp)
{
	static users_result  ur;
    PGresult* pg_res;
	std::string req;

    std::cout << "getting users list, id = " << argp->cookie << '\n';
    req = "SELECT login, online FROM users WHERE id != " + std::to_string (argp->cookie) + ";";
    ur.res.descr = new char[RESULT_DESCR_LEN];

    if (!complete_request (req, pg_res)) {
        ur.res.code = FAIL;
        strcpy (ur.res.descr, "FAIL: getting users list, database error");
        ur.data.data_len = 0;
        return &ur;
    }

    int len = PQntuples (pg_res);
    ur.data.data_len = len;
    ur.data.data_val = new users_message[len];
    for (int i = 0; i < len; ++i) 
	{
        ur.data.data_val[i].login = new char[LOGIN_LEN];
        strcpy (ur.data.data_val[i].login, PQgetvalue (pg_res, i, 0));

        if (strcmp (PQgetvalue (pg_res, i, 1), "t") == 0) 
		{
            ur.data.data_val[i].online = ONLINE;
        } 
		else 
		{
            ur.data.data_val[i].online = OFFLINE;
        }
    }
    ur.res.code = OK;
    strcpy (ur.res.descr, "getting users list, success");    
    PQclear (pg_res);

	return &ur;
}

result* chat_server_with_db::register_2_svc(register_params *argp, struct svc_req *rqstp)
{
    static result  result;
	PGresult* pg_res;
	std::string req;

	std::cout << "Registerring: " << argp->login << " on server\n";
	req = "SELECT id FROM users WHERE login ='" + std::string (argp->login) + "';";
	if (!complete_request (req, pg_res)) {
		result.code = FAIL;
		strcpy (result.descr, "FAIL: registerring, database error");
		PQclear (pg_res);
		return &result;
	}
    result.descr = new char[RESULT_DESCR_LEN];

	if (PQntuples (pg_res) == 1) 
	{
		result.code = FAIL;
		strcpy (result.descr, "FAIL: registerring, user with such login is exists");
		PQclear (pg_res);
		return &result;
	}

	PQclear (pg_res);
	req = "SELECT id FROM users";

	if (!complete_request (req, pg_res)) 
	{
		result.code = FAIL;
		strcpy (result.descr, "FAIL: registerring, database error");
		PQclear (pg_res);
		return &result;
	}

	int id = PQntuples (pg_res) + 1;
	PQclear (pg_res);
	req = "INSERT INTO users VALUES (" +
	      std::to_string (id) + ", '" + std::string (argp->login) + "', '" + std::string (argp->password) + "', FALSE);";

	if (complete_request (req, pg_res)) 
	{
	    result.code = OK;
	    strcpy (result.descr, "Registerring,  success");
	} 
	else 
	{
        result.code = FAIL;
        strcpy (result.descr, "FAIL: registerring, database error");
        PQclear (pg_res);
    }

	return &result;
}


bool chat_server_with_db::complete_request (std::string req, PGresult*& pg_res)
{
	pg_res = PQexec (conn, req.data());
	if (PQresultStatus (pg_res) != PGRES_TUPLES_OK && PQresultStatus (pg_res) != PGRES_COMMAND_OK) {
		std::cout << "FAIL: db request, " << PQerrorMessage (conn) << "\n";
		PQclear (pg_res);
		return false;
	}
	return true;
}