/*
 * ftp.c
 *
 *  Created on: May 4, 2016
 *      Author: nguyenngocduy
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <resolv.h>
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<fcntl.h>

#define PORT_FTP 		21
#define PASV_PORT 	40000
#define SERV_ADDR 	"192.168.127.1"
//#define SERV_ADDR 	"96.47.72.72"
#define BUFSIZE 			1024

char current_dir[PATH_MAX + 1];

int main(int argc, char* argv[])
{
	int sockfd;
	struct sockaddr_in server;
	char buf[BUFSIZE];
	int command_code;  // indicate message code, such as: 220, 200, 331

	// Create socket for client
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Socket can't be created");
		exit(errno);
	}

	printf("Client socket was created...\n");

	// Create information for sockaddr_in dest
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT_FTP);
    if ( inet_aton(SERV_ADDR, &server.sin_addr.s_addr) == 0 )
    {
        perror(SERV_ADDR);
        exit(errno);
    }

    // Connect to server
    if ( connect(sockfd, (struct sockaddr*)&server, sizeof(server)) != 0 )
	{
		perror("Connect can't be established");
		exit(errno);
	}
    printf("Connected to FTP server!\n");
    printf("Server information: \n\tIP address: %s\n\tPort: %d\n\n", SERV_ADDR, PORT_FTP);

    // Get "Hello?"
	bzero(buf, BUFSIZE);
	recv(sockfd, buf, sizeof(buf), 0);
	printf("FTP Hello: %s", buf);
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
		//printf("%d\n", buf[3]=='-');
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
    //memset(username, 0, sizeof(username));

    // User type password and send it to server
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "Password [%s]: ", username);
    sprintf(pass, "%s", getpass(buf));
    //scanf("%s", pass);

    memset(buf, 0, sizeof(buf));
    sprintf(buf, "PASS %s\r\n", pass);
    //printf("Buffer: %s\n", buf);
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
    printf ("FTP user command start...\n");
    pwd(sockfd);
    printf("Current Directory: %s\n", current_dir);
    if(argc == 2)
		if(strcmp(argv[1], "-p") == 0) // passive mode
		{
			pasv:
			printf ("%s@%s: %s >> ", username, username, current_dir); // Example: vsftp@vsftp: /home/Downloads >>

		}
		else
		{
			if(strcmp(argv[1], "-a") == 0) // active mode
			{
				printf ("%s@%s: %s >> ", username, username, current_dir); // Example: vsftp@vsftp: /home/Downloads >>
				if(!put(sockfd, "Lee_lap.txt", "/media/nguyenngocduy/Data", 0))
				{
					printf("Transmission failed\n");
				}
			}
			else
			{
				printf("Wrong parameters!\nPlease make sure you follow this:\n./ftp  OR  ./ftp [-p or -a]\n");
			}
		}
    else // argc = 1
    	goto pasv;

    disconnect(sockfd);
    memset(buf, 0, sizeof(buf));
    memset(username, 0, sizeof(username));
    memset(pass, 0, sizeof(pass));
    close(sockfd);
	return 0;
}

void parseCode(int command_code)
{
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
	}
	printf("\n");
}

void pwd(int sockfd)
{
	char buff[BUFSIZE];
	int command_code;
	sprintf(buff, "PWD\r\n");
	// printf("%s\n", buff);
	send(sockfd, buff, strlen(buff), 0);
	// printf("Send successful\n");
	memset(buff, 0, sizeof(buff));
	recv(sockfd, buff, BUFSIZE, 0);
	// printf("Server response: %s\n", buff);

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
	// printf("%s\n", current_dir);
	memset(buff, 0, sizeof(buff));
	// return current_dir;
}

 int disconnect(int sockfd)
{
	char buff[6];
	//memset(buff, 0, sizeof(buff));
	//sprintf(buff, "QUIT\n" );
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

 int put(int sockfd, char filename[], char localPath[], int mode) // 0 active, 1 passive
 {
	 char stor[BUFSIZE + 1];
	 int command_code;
	 //memset(stor, 0, sizeof(stor));
	 if(strcmp(current_dir, "/") != 0)
		 sprintf(stor, "STOR %s/%s\n", current_dir, filename);
	 else
		 sprintf(stor, "STOR %s%s\n", current_dir, filename);
	 //printf("Buffer size: %d\nBuffer length: %d\n", sizeof(buff), strlen(buff));

	 /*
	 send(sockfd, stor, strlen(stor), 0);

	 memset(stor, 0, sizeof(stor));
	 recv(sockfd, stor, sizeof(stor), 0);

	 sscanf(stor, "%d", &command_code);
	 if(command_code != 150)
	 {
		 parseCode(command_code);
		 memset(stor, 0, sizeof(stor));
		 return 0;
	 }
	 */

	char local[PATH_MAX + 1];
	sprintf(local, "%s/%s", localPath, filename);
	FILE *f = fopen(local, "rt");
	if(f == NULL)
	{
		printf("Open file error!\n");
		return 0;
	}

	printf("\nOpen file: %s\n", local);
	 if(mode == 0) // active, server use port 20 for data transmission
	 {
		 // create new server address to connect
		 char port[BUFSIZE];
		 char buff[BUFSIZE];
		 int min_port = 1025;
		 int max_port = 65535;
		struct sockaddr_in serv_addr, client_addr;
		int sockfd_client, sockfd_server, len, nb, fd, i;
		struct hostent *host;

		if((sockfd_server = socket(AF_INET, SOCK_STREAM, 0) ) < 0)
		{
			perror("Socket can't be created!\n");
			fclose(f);
			return 0;
		}

		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		//serv_addr.sin_port = htons(20);
		//inet_aton("192.168.127.2", &serv_addr.sin_addr.s_addr);
		memset(&serv_addr.sin_zero, 0, 8);

		len = sizeof(client_addr);
		int port_num=3425;

		for( i = min_port; i <= max_port; i++)
		{
			serv_addr.sin_port = htons(i);

			if(bind(sockfd_server, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr) >= 0) )
			{
				printf("FTP client receive data port: %d\n", i);
				port_num = i;
				break;
			}
		}

		memset(buff, 0, sizeof(buff));
		//memset(port, 0, sizeof(port));
		/*
		struct sockaddr_storage addr;
		socklen_t l;
		getpeername(sockfd_server, (struct sockaddr*) &addr, &l);
		struct sockaddr_in *s = (struct sockaddr_in *) &addr;
		int data_port = ntohs(s->sin_port);
		inet_ntop(AF_INET, &s->sin_addr, buff, sizeof(buff));
		printf("FTP Client IP address: %s\nPort: %d\n", buff, data_port);
		*/
		sprintf(buff, "%s", inet_ntoa(serv_addr.sin_addr));
		//sprintf(buff, "192.168.127.2");
		/*if ( connect(sockfd_server, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != 0 )
		{
			perror("Connect can't be established");
			exit(errno);
		}*/
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
			fclose(f);
			return 0;
		}
		//memset(buff, 0, sizeof(buff));
		if (recv(sockfd, buff, sizeof(buff), 0) < 0)
		{
			perror("Receive error!");
			fclose(f);
			return 0;
		}
		else
			printf("Server Response: %s\n", buff);

		printf("Listen on PORT: %d\n", port_num);
		if(listen(sockfd_server, 5) < 0)
		{
			perror("Can't listen on this socket server");
			fclose(f);
			return 0;
		}

		// send STOR
		if(send(sockfd, stor, strlen(stor), 0) < 0)
		{
			perror("Send STOR error");
			fclose(f);
			return 0;
		}

		memset(buff, 0, sizeof buff);
		recv(sockfd, buff, sizeof buff, 0);
		printf("%s\n", buff);

		printf("Listening\n");
		len = sizeof(client_addr);
		sockfd_client = accept(sockfd_server, (struct sockaddr*) &client_addr, &len);
		if(sockfd_client < 0)
		{
			perror("Accept error");
			exit(1);
		}

		printf("Accepted!");
		// send data
		memset(buff, 0, sizeof(buff));
		while( (nb = read(f, buff, BUFSIZE))  > 0)
		{
			if(send(sockfd_client, buff, nb, 0) < 0 )
			{
				perror("send data error");
				exit(1);
			}
		}
		close(sockfd_server);
		close(sockfd_client);
		sockfd_client =0;
		sockfd_server=0;
		printf("Transmission complete!\n");
	 }
	 else // passive, server use don't use port 20 for data transmission
	 {

	 }
	 fclose(f);
	 return 1;
 }
