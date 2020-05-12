#include "chat.h"

// Подключаем истинное зло
#include <postgresql/libpq-fe.h>
#include <string>


class chat_server_with_db
{
public:

    static result *send_2_svc (send_params *argp, struct svc_req *rqstp);
    static login_result *login_2_svc(login_params *argp, struct svc_req *rqstp);
    static result *logout_2_svc(logout_params *argp, struct svc_req *rqstp);
    static receive_result *receive_2_svc (receive_params *argp, struct svc_req *rqstp);
    static users_result *users_2_svc(users_param *argp, struct svc_req *rqstp);
    static result *register_2_svc(register_params *argp, struct svc_req *rqstp);

    static void set_conn (PGconn* _conn) { conn = _conn; }
    static void exit_nicely () {
    	PQfinish(conn);
    	exit(0);
	}

private:

	static PGconn* conn;
    static bool complete_request (std::string req, PGresult*& pg_res);
};
