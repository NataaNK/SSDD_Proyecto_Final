#ifndef MENSAJE_H
#define MENSAJE_H

struct peticion  {
    int op; /* Operación, 0 (register), 1 (unregister), 2 (connect), 3 (publish), 4 (delete),
			 5 (list_users), 6 (list_content), 7 (disconn), 8 (get_file)*/
	char time[20];
	char user_name[256];
	char listen_port[6];
	char file_name[256];
	char description[256];
	char remote_user_name[256];
	char remote_file_name[256];
	char local_file_name[256];
	char err_msg[50];
};

#endif // MENSAJE_H