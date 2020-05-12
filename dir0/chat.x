#define RESULT_DESCR_LEN 128
#define LOGIN_LEN 32
#define PASSWORD_LEN 32
#define MESSAGE_LEN 128

enum result_code {
  FAIL,
  OK
};

struct result {
  result_code code;
  string descr<RESULT_DESCR_LEN>;  
};

struct send_params{
  string to<LOGIN_LEN>;
  int cookie;
  string message<MESSAGE_LEN>;
};

struct login_params{
  string login<LOGIN_LEN>;
  string password<PASSWORD_LEN>;
};

struct logout_params{
  int cookie;
};

struct receive_params{
   int cookie;
};

struct users_param{
  int cookie;
};

struct register_params{
  string login<LOGIN_LEN>;
  string password<PASSWORD_LEN>;
};

struct login_result {
  result res;
  int cookie;
};

struct receive_message {
  string from<LOGIN_LEN>;
  string message<MESSAGE_LEN>;
};

struct receive_result {
  result res;
  receive_message data<>;
};

enum online_status {
  OFFLINE,
  ONLINE
};

struct users_message {
  string login<LOGIN_LEN>;
  online_status online;
};

struct users_result {
  result res;
  users_message data<>;
};

program RPC_CHAT {
  version RPC_CHAT_VERSION_2 {
    result SEND(send_params) = 1;
    login_result LOGIN(login_params) = 2;
    result LOGOUT(logout_params) = 3;
    receive_result RECEIVE(receive_params) = 4;
    users_result USERS(users_param) = 5;
    result REGISTER(register_params) = 6;
  } = 2;
} = 0x20000001;
