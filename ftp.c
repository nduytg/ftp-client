/*
	FTP Client
	* 1312084 - 1312086 - 1312110
 */

#include "ftp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>


#define PORT_FTP 		21
#define BUFSIZE 			1024

char current_dir[PATH_MAX + 1];
char myIP[16];
char svIP[16];
int svPassivePort;
int mode;		//Mode active: 0, Mode passive: 1
int dataSock;
char SERV_ADDR[16];


int main(int argc, char* argv[])
{
	//Kiem tra mode active hay passive
	if(argc >= 2 && (strcmp(argv[1],"-p") == 0) )
	{
		//Mode passive
		mode = 1;
		printf("Passive mode!\n");
	}
	else if (argc >= 2 && (strcmp(argv[1],"-a") == 0))
	{
		//Mode active
		mode = 0;
		printf("Active mode!\n");
	}
	else
	{
		printf("Usage - Plz type in as: ./ftp -p ip OR ./ftp -a ip\n");
		exit(EXIT_FAILURE);
	}
	
	strcpy(SERV_ADDR,argv[2]);
	printf("Test, server addr: %s\n",SERV_ADDR);
	
	int sockfd;
	struct sockaddr_in server;
	char buf[BUFSIZE];
	int command_code;  // indicate message code, such as: 220, 200, 331


	//---- Create command socket for client -----
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Socket can't be created");
		exit(errno);
	}

	printf("Command socket was created...\n");

	// Create information for sockaddr_in dest
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT_FTP);
    
    if ( inet_aton(SERV_ADDR, &server.sin_addr.s_addr) == 0 )
    {
        perror(EXIT_FAILURE);
        exit(errno);
    }

    // Connect to server
    if ( connect(sockfd, (struct sockaddr*)&server, sizeof(server)) != 0 )
	{
		perror("Connect can't be established");
		exit(EXIT_FAILURE);
	}
    printf("Connected to FTP server!\n");
    printf("Server information: \n\tIP address: %s\n\tPort: %d\n\n", SERV_ADDR, PORT_FTP);
	//-----------------------------

	struct ifaddrs *ifaddr, *ifa;
	int family, s;
	char host[NI_MAXHOST];

	if (getifaddrs(&ifaddr) == -1)
	{
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	// get interface address
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL)
			continue;

		s = getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

		if(ifa->ifa_addr->sa_family==AF_INET)
		{
			if (s != 0)
			{
				printf("getnameinfo() failed: %s\n", gai_strerror(s));
				exit(EXIT_FAILURE);
			}

			int c=0;
			for(int j = 0; j<strlen(host); j++)
			{
				if(host[j] == SERV_ADDR[j])
				{
					if(host[j] == '.')
						c++;
				}
				if(c == 3)
				{
					strcpy(myIP, host);
				}
			}
		}
	}

    // Get "Hello"
	bzero(buf, BUFSIZE);
	recv(sockfd, buf, sizeof(buf), 0);
	printf("FTP Hello: %s", buf);
	if(strstr(buf, "220 ") == 0)
	
	//Vong lap xu ly hello strings
    while(buf[3] == '-')
    {
		bzero(buf, BUFSIZE);
		recv(sockfd, buf, sizeof(buf), 0);
		sscanf(buf, "%d", &command_code);
		if(command_code != 220)
		{
			parseCode(command_code);
			exit (1);
		}
		printf("%s", buf);
		if(buf[3] == ' ')
		{
			memset(buf, 0, sizeof buf);
			break;
		}
    }

    // User type username and send it to server
    printf("Identify user!\n");
    char username[256];
    char pass[256];
    int result;
  reEnter:
    printf("\nUsername: ");
    memset(buf, 0, sizeof(buf));
    scanf ("%s", username);

    sprintf(buf, "USER %s\r\n", username);
    result = send(sockfd, buf, strlen(buf), 0); // send USER username\r\n to server
    memset(buf, 0, sizeof(buf));

    result = recv(sockfd, buf, BUFSIZE, 0);

    sscanf(buf, "%d", &command_code);
    if(command_code != 331) // error
    {
    	parseCode(command_code);
    	exit(1);
    }
    printf("Server response: %s", buf);

    // User type password and send it to server
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "Password [%s]: ", username);
    sprintf(pass, "%s", getpass(buf));

    memset(buf, 0, sizeof(buf));
    sprintf(buf, "PASS %s\r\n", pass);
    result = send(sockfd, buf, strlen(buf), 0);

    memset(buf, 0, sizeof(buf));
    result = recv(sockfd, buf, BUFSIZE, 0);

    sscanf(buf, "%d", &command_code);
    if(command_code != 230)
    {
    	parseCode(command_code);
        memset(buf, 0, sizeof(buf));
        memset(username, 0, sizeof(username));
        memset(pass, 0, sizeof(pass));
        goto reEnter; // re-enter username and password
    }
    
    printf("Server response: %s", buf);
    memset(buf, 0, sizeof(buf));
    
    //Socket o client de nhan data!
    dataSock = 0;
    int svPort = 0;
    
    if ((dataSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Socket can't be created");
		exit(errno);
	}

	printf("Data socket was created...\n");
    
    //Goi lenh PASSIVE || PORT, tuy theo mode passive hay active
    if (mode == 1)
    {
		//Passive

		sprintf(buf, "PASV\r\n", pass);

		result = send(sockfd, buf, strlen(buf), 0);

		memset(buf, 0, sizeof(buf));
		result = recv(sockfd, buf, BUFSIZE, 0);
		printf("Server response: %s", buf);
		
		char *pt = NULL;
		pt = strtok(buf," ");
		for(int i=0; i < 4;i++)
			pt = strtok(NULL,",");
		
		svPort = atoi(pt = (strtok(NULL,","))) * 256;
		svPort = svPort + atoi(pt = strtok(NULL,"\0"));
		
		printf("Port received though PORT command return: %d\n",svPort);
		
		// Create information for sockaddr_in dest
		bzero(&server, sizeof(server));
		server.sin_family = AF_INET;
		server.sin_port = htons(svPort);
		
		if ( inet_aton(SERV_ADDR, &server.sin_addr.s_addr) == 0 )
		{
			perror(SERV_ADDR);
			exit(errno);
		}

		// Connect to server
		if ( connect(dataSock, (struct sockaddr*)&server, sizeof(server)) != 0 )
		{
			perror("Connect can't be established");
			exit(errno);
		}
		
		printf("Connected to Server data port!\n");
		printf("Connection information: \n\tIP address: %s\n\tPort: %d\n\n", SERV_ADDR, svPort);	
	}
	else if (mode == 0)
    {
		//***Chua test cai nay!!!
		// create new server address to connect
		char port[BUFSIZE];
		char buff[BUFSIZE];
		int min_port = 49153;
		int max_port = 65535;
		struct sockaddr_in serv_addr, client_addr;
		int sockfd_client, sockfd_server, len, nb, fd, i;


		sockfd_server = socket(AF_INET, SOCK_STREAM, 0);
		if(sockfd_server < 0)
		{
			perror("Socket can't be created!\n");

			return 0;
		}


		bzero((char*) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = inet_addr(myIP);
		len = sizeof(client_addr);
		
		int port_num=3425;

		for( i = min_port; i <= max_port; i++)
		{
			serv_addr.sin_port = htons(i);

			if(bind(sockfd_server, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0)
			{
				printf("FTP client receive data port: %d\n", i);
				port_num = i;
				break;
			}
		}

		memset(buff, 0, sizeof(buff));
		sprintf(buff, "%s", myIP);

		for (int j = 0; j <strlen(buff); j++)
		{
			if(buff[j] == '.')
			{
				buff[j] = ',';
			}
		}

		int a = port_num/256;
		int b = port_num - a*256;
		sprintf(port, "PORT %s,%d,%d\r\n", buff, a, b);
		printf("Send command: %s\n", port);
		if(send(sockfd, port, strlen(port), 0) < 0)
		{
			perror("Send error!");

			return 0;
		}

		if (recv(sockfd, buff, sizeof(buff), 0) < 0)
		{
			perror("Receive error!");
			return 0;
		}
		else
			printf("Server Response: %s\n", buff);

		sscanf(buff, "%d", &command_code);
		if(command_code != 200)
		{
			parseCode(command_code);
			return 0;
		}

		printf("Listen on PORT: %d\n", port_num);
		int lis = listen(sockfd_server, 5);
		if(lis < 0)
		{
			perror("Can't listen on this socket server");

			return 0;
		}
	}
    
    printf ("FTP user command start...\n");
    pwd(sockfd);
    printf("Current Directory: %s\n", current_dir);
	
	
	//--------- Vong lap xu ly cmd -----------
	//out khi nhan lenh bye/quit!!
	char cmd[100];
	while(1)
	{
		
		printf ("%s@%s: %s >> ", username, username, current_dir);
		printf("Your command: ");
		memset(cmd, 0, sizeof(cmd));
		scanf ("%s", cmd);
		command_handler(sockfd,cmd);
	}

	//put(sockfd, "README.TXT", "/home/nduytg/", 0);

    disconnect(sockfd);
	freeifaddrs(ifaddr);
    memset(buf, 0, sizeof(buf));
    memset(username, 0, sizeof(username));
    memset(pass, 0, sizeof(pass));
    close(sockfd);
	return 0;
}


int put(int sockfd, char filename[], char localPath[])
 {
	char stor[BUFSIZE + 1];
	int command_code;
	char buff[BUFSIZE];
	int len, nb, fd, i;
	
	
	if(strcmp(current_dir, "/") != 0)
		sprintf(stor, "STOR %s/%s\r\n", current_dir, filename);
	else
		sprintf(stor, "STOR %s\r\n", filename);

	char local[PATH_MAX + 1];
	sprintf(local, "%s/%s", localPath, filename);
	FILE *f = fopen(local, "rt");
	
	if(f == NULL)
	{
		printf("Open file error!\n");
		return 0;
	}

	printf("\nOpen file: %s\n", local);
	
	// send STOR
	printf("STOR cmd: %s\n",stor);
	if(send(sockfd, stor, strlen(stor), 0) < 0)
	{
		perror("Send STOR error");
		fclose(f);
		return 0;
	}
	

	if (recv(sockfd, buff, sizeof(buff), 0) < 0)
	{
		perror("Receive error!");
		fclose(f);
		return 0;
	}
	else
		printf("Server Response: %s\n", buff);

	sscanf(buff, "%d", &command_code);
	if(command_code != 200)
	{
		parseCode(command_code);
		return 0;
	}
	
	if(mode == 1) // passive
	{
		// send data
		memset(buff, 0, sizeof(buff));
		while( (nb = read(f, buff, BUFSIZE))  > 0)
		{
			if(send(dataSock, buff, nb, 0) < 0 )
			{
				perror("send data error");
				exit(1);
			}
		}
	}
	else // active
	{

	}

	fclose(f);
	return 1;
 }


void pwd(int sockfd)
{
	char buff[BUFSIZE];
	int command_code;
	
	sprintf(buff, "PWD\r\n");
	send(sockfd, buff, strlen(buff), 0);
	memset(buff, 0, sizeof(buff));
	recv(sockfd, buff, BUFSIZE, 0);

	sscanf(buff, "%d", &command_code);
	if(command_code != 257)
	{
		parseCode(command_code);
		memset(buff, 0, sizeof(buff));
		return;
	}

	for (int i = 5; i < strlen(buff)-2; i++)
	{
		current_dir[i-5] = buff[i];
		if(buff[i+1] == '\"')
			//current_dir[i-4] = '\n';
			break;
	}
	printf("%s\n", current_dir);
	memset(buff, 0, sizeof(buff));
}


void help()
{
	printf("\n\n");
	printf("##################### FTP Client Help ######################\n\n");
	printf("Cac lenh duoc ho tro \n");
	printf("help|?\t\t\tmdir\t\t\tuser\n");
	printf("bye|quit\t\tcd\t\t\tclose|disconnect\n");
	printf("recv|get\t\tdelete|mdelete\t\tsend|put\n");
	printf("pwd\t\t\tcdup\t\t\tls\n");
}

//recv remote-file [local-file ]
int ftp_recv(int sock, char *arg1, char *arg2)	//RETR
{
	char remote_directory[256];
	char local_file[256];
	int file_handler;	
	char filename[256];
	char buf[256];
	struct stat obj;
	int size, status;
	
	if (arg1 == NULL)
		arg1 = "";
	if (arg2 == NULL)
		arg2 = "";
		
	strcpy(remote_directory, arg1);
	strcpy(local_file, arg2);
		
	printf("Which file do you want to get: ");
	scanf("%s", filename);
	
	strcpy(buf, "RETR ");
	strcat(buf, filename);
	strcat(buf,"\r\n");
	send(sock, buf, 100, 0);
	recv(sock, &size, sizeof(int), 0);
	
	if(!size)
	{
		printf("No such file on the remote directory\n\n");
		return false;
	}
	
	char *file_buffer = (char*) malloc(size + 1);
	recv(sock, file_buffer, size, 0);
	
	while(1)
	{
		file_handler = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0777);
		
		if(file_handler == -1)
		{
			sprintf(filename + strlen(filename), "%d", 1);//needed only if same directory is used for both server and client
		}
		else 
			break;
	}
	
	write(file_handler, file_buffer, size);
	close(file_handler);
	strcpy(buf, "cat ");
	strcat(buf, filename);
	system(buf);
	
	return true;
}

void bye(int sock)		//QUIT
{
	char buf[100];
	int size, code;
	strcpy(buf, "QUIT\r\n");
	send(sock, buf, 100, 0);
	
	printf("FTP Client will exit\n");
	exit(EXIT_SUCCESS);
}

int ls(int sockfd, char *arg1, char *arg2)		//LIST
{	
	char *file_buffer;
	char remote_directory[256];
	char local_file[256];
	int file_handler;	
	char filename[256];
	char buf[100];
	int size;

	if (arg1 == NULL)
		arg1 = ".";
	if (arg2 == NULL)
		arg2 = "~ls.txt";
	
	//Send LIST command
	//sprintf(buff, "LIST %s %s\r\n", remote_directory, local_file);
	sprintf(buf, "LIST\r\n");
	
	if(send(sockfd, buf, strlen(buf), 0) < 0)
	{
		perror("Send LIST error");
		return 0;
	}
	
	if(mode == 1)
	{
		recv(sockfd, &size, sizeof(int), 0);
		recv(sockfd, buf, size, 0);
		printf("Server response: %s\n",buf);
	}

	recv(dataSock, &size, sizeof(int), 0);
	file_buffer = (char*)malloc(size + 1);
	memset(file_buffer, 0, size);
	recv(dataSock, file_buffer, size, 0);
	
	
	file_handler = creat(local_file, O_WRONLY);
	write(file_handler, file_buffer, size);
	close(file_handler);
	printf("ls command result:\n\n");
	printf("%s\n",file_buffer);
	//system("cat ~ls.txt");
	
	return true;
}

void command_handler(int sock, char* cmd)
{
	char temp[256];
	char* pch;

	char* path;
	
	strcpy(temp, cmd);
	
	if (strstr(temp, " ") == NULL)
	{
		cmd = (char*) malloc (sizeof(temp));
		strcpy(cmd, temp);
	}
	else
	{
		pch = strtok(temp, " ");
		cmd = (char*) malloc (sizeof(pch));
		strcpy(cmd, pch);
	
		pch = strtok(NULL, " ");
		path = (char*) malloc (sizeof(pch));
		strcpy(path, pch);
	}
	
	printf("\nCmd get: %s\n",cmd);
	
	if (strcmp("get",cmd) == 0 || strcmp("recv",cmd) == 0)
	{
		ftp_recv(sock, NULL, NULL);
		
	}
	else if (strcmp("bye",cmd) == 0 || strcmp("quit",cmd) == 0)
	{
		bye(sock);
	}
	else if (strcmp("pwd",cmd) == 0 )
	{
		pwd(sock);
	}
	else if (strcmp("ls",cmd) == 0)
	{
		ls(sock,NULL,NULL);
	}
	else if (strcmp(cmd, "cd") == 0)
	{
		cd(sock, path);
	}
	else if (strcmp(cmd, "cdup") == 0)
	{
		cdup(sock);
	}
	else if (strcmp(cmd, "mkdir") == 0)
	{
		MKDIR(sock, path);
	}
	else if (strcmp(cmd, "delete") == 0)
	{
		delete(sock, path);
	}
	else if (strcmp(cmd, "rmdir") == 0)
	{
		RMDIR(sock, path);
	}
	else if (strcmp(cmd,"help") == 0)
	{
		printf("Khong ho tro lenh nay: %s!\n",cmd);
		help();
	}
	else
	{
		printf("Khong ho tro lenh nay: %s!\n",cmd);
		help();
	}
}

 int disconnect(int sockfd)
{
	char buff[6];

	sprintf(buff, "QUIT\r\n");
	send(sockfd, buff, strlen(buff) + 1, 0);

	printf("\nTerminate connection to FTP server!\n");
	memset(buff, 0, sizeof(buff));
	recv(sockfd, buff, sizeof(buff), 0);

	int command_code;
	sscanf(buff, "%d", &command_code);
	if(command_code != 221)
	{
		parseCode(command_code);
		memset(buff, 0, sizeof(buff));
		return 0;
	}

	printf("\nConnection closed!\n");
	memset(buff, 0, sizeof(buff));
	return 1;
}


void parseCode(int command_code)
{
	printf("%d",command_code);
	switch(command_code)
	{
		case 200:
			printf("Command okay");
			break;
		case 500:
			printf("Syntax error, command unrecognized.");
			printf("This may include errors such as command line too long.");
			break;
		case 501:
			printf("Syntax error in parameters or arguments.");
			break;
		case 202:
			printf("Command not implemented, superfluous at this site.");
			break;
		case 502:
			printf("Command not implemented.");
			break;
		case 503:
			printf("Bad sequence of commands.");
			break;
		case 530:
			printf("Not logged in.");
			break;
		case 120:
			printf("Service ready in nnn minutes.");
			break;
		case 220:
			printf("Service ready for new user.");
			break;
		case 221:
			printf("Response code: Service closing control connection");
			break;	
		case 421:
			printf("Service not available, closing control connection.");
			printf("This may be a reply to any command if the service knows it must shut down.");
			break;
		case 230:
			printf("User logged in, proceed. Logged out if appropriate.");
			break;
		case 331:
			printf("User name okay, need password.");
			break;
		case 332:
			printf("Need account for login.");
			break;
	}
	printf("\n");
}

void cd (int sock_fd, char* s1)
{
	Send_cmd("CWD", s1, sock_fd);
	Reply_cmd(sock_fd);
}

void cdup (int sock_fd)
{
	Send_cmd("CDUP", NULL, sock_fd);
	Reply_cmd(sock_fd);
}

void MKDIR (int sock_fd, char* s1)
{
	Send_cmd("MKD", s1, sock_fd);
	Reply_cmd(sock_fd);
}

void delete (int sock_fd, char* s1)
{
	Send_cmd("DELE", s1, sock_fd);
	Reply_cmd(sock_fd);
}

void RMDIR (int sock_fd, char* s1)
{
	//Stub??
}

//Ham send cmd
int Send_cmd (char* s1, char* s2, int sock_fd)     //s1 is cmd, s2 i path (or filename)
{
	char buff_to_send[256];
	int send_err;
	
	if (s1 != NULL)
	{
		strcpy(buff_to_send, s1);
		
		if (s2 != NULL)
		{
			strcat(buff_to_send, " ");
			strcat(buff_to_send, s2);
			strcat(buff_to_send, "\r\n");
			send_err = send(sock_fd, buff_to_send, strlen(buff_to_send), 0);
		}
		else
		{
			strcat(buff_to_send, " ");
			strcat(buff_to_send, "\r\n");
			send_err = send(sock_fd, buff_to_send, strlen(buff_to_send), 0);
		}
	}
	else
	{
		strcat(buff_to_send, " ");
		strcat(buff_to_send, "\r\n");
		send_err = send(sock_fd, buff_to_send, strlen(buff_to_send), 0);
	}
	
	if (send_err < 0)
	{
		printf("Send Error!!!\n");
	}
	return send_err;
}

//Ham nhan reply tu server
int Reply_cmd(int sock_fd)
{
	int code = 0, count = 0;
	char buff_to_recv[512];
	
	count = recv(sock_fd, buff_to_recv, 510, 0);
	
	if (count > 0)
	{
		sscanf(buff_to_recv, "%d", &code);
	}
	else
	{
		return 0;
	}
	
	while (1)
	{
		if (count <= 0)
		{
			break;
		}
		buff_to_recv[count] = '\0';
		printf("Server response: %s", buff_to_recv);
		count = recv(sock_fd, buff_to_recv, 510, 0);
	}
	return code;
}

//Ham phan tich cau lenh nguoi dung nhap vao
void Pasre (char s1[256], int sock_fd)  //s1 la lenh nhap tu nguoi dung
{
	char temp[256];
	char* pch;
	char* cmd;
	char* path;
	
	strcpy(temp, s1);
	
	if (strstr(temp, " ") == NULL)
	{
		cmd = (char*) malloc (sizeof(temp));
		strcpy(cmd, temp);
	}
	else
	{
		pch = strtok(temp, " ");
		cmd = (char*) malloc (sizeof(pch));
		strcpy(cmd, pch);
	
		pch = strtok(NULL, " ");
		path = (char*) malloc (sizeof(pch));
		strcpy(path, pch);
	}
	
	if (strcmp(cmd, "cd") == 0)
	{
		cd(sock_fd, path);
	}
	if (strcmp(cmd, "cdup") == 0)
	{
		cdup(sock_fd);
	}
	if (strcmp(cmd, "mkdir") == 0)
	{
		MKDIR(sock_fd, path);
	}
	if (strcmp(cmd, "delete") == 0)
	{
		delete(sock_fd, path);
	}
	if (strcmp(cmd, "rmdir") == 0)
	{
		RMDIR(sock_fd, path);
	}
}
