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
#define SERV_ADDR 	"96.47.72.72"
#define BUFSIZE 			1024

char current_dir[PATH_MAX + 1];

//Kieu bool
typedef int bool;
enum {false, true };

void help()
{
	printf("\n");
	printf("####### FTP Client Help ########\n\n");
	printf(" Cac lenh duoc ho tro \n");
	printf("help|?\t\t\tmdir\t\t\tuser\n");
	printf("bye|quit\t\tcd\t\t\tclose|disconnect\n");
	printf("recv|get\t\tdelete|mdelete\t\tsend|put\n");
	printf("pwd\t\t\tcdup\t\t\tls\n");
}

bool command_handler(int sock, char* cmd)
{
	help();
	int file_handler;	
	char filename[256];
	char buf[256];
	struct stat obj;
	int size, status;
	
	if (strcmp("get",cmd) == 0 || strcmp("recv",cmd))
	{
		printf("Enter filename to put to server: ");
		scanf("%s", filename);
		file_handler = open(filename, O_RDONLY);
		if(file_handler == -1)
		{
			printf("No such file on the local directory\n\n");
			exit(EXIT_FAILURE);
		}
		
		strcpy(buf, "put ");
		strcat(buf, filename);
		send(sock, buf, 100, 0);
		stat(filename, &obj);
		size = obj.st_size;
		send(sock, &size, sizeof(int), 0);
		sendfile(sock, file_handler, NULL, size);
		recv(sock, &status, sizeof(int), 0);
		
		if(status)
			printf("File stored successfully\n");
		else
			printf("File failed to be stored to remote machine\n");
	}
}

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
    char *servAddr = "96.47.72.72";
    printf("Server information: \n\tIP address: %s\n\tPort: %d\n\n", servAddr, PORT_FTP);

    // Get "Hello?"
    bzero(buf, BUFSIZE);
    recv(sockfd, buf, sizeof(buf), 0);
    sscanf(buf, "%d", &command_code);
    if(command_code != 220)
    {
    	parseCode(command_code);
    	exit (1);
    }
    printf("FTP Hello: %s", buf);

    // User type username and send it to server
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
    if(strcmp(argv[1], "-p") == 0 || argc == 1) // passive mode
    {
    	printf ("%s@%s>>", username, current_dir); // Example: vsftp@/home/Downloads>>

    }
    else
    {
    	if(strcmp(argv[1], "-a") == 0) // active mode
    	{
        	printf ("%s@%s>>", username, current_dir); // Example: vsftp@/home/Downloads>>

    	}
    	else
    	{
    		printf("Wrong parameters!\nPlease make sure you follow this:\n./ftp  OR  ./ftp [-p or -a]\n");
    	}
    }
    
    //----handle command here----
    char tmp[256];
    strcpy(tmp,"get");
    printf("Test get\n");
    command_handler(sockfd, tmp);
    //--------------------

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
		case 120:
			printf("Service ready in nnn minutes.");
			break;
		case 220:
			printf("Service ready for new user.");
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

