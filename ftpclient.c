

/*FTP Client*/
 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h> 
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
 
/*for getting file size using stat()*/
#include<sys/stat.h>
 
/*for sendfile()*/
#include<sys/sendfile.h>
 
/*for O_RDONLY*/
#include<fcntl.h>


#define PORT 21

int getch() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

void replylogcode(int code)
{
	switch(code){
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



char* sendCommand(char str[100])
{
	//sprintf(buf,"USER %s\r\n",info);
	return NULL;
}
 
int main(int argc,char *argv[])
{
	int mode;	//0 = active, 1 = passive
	struct sockaddr_in *remote;
	struct stat obj;
	int sock;
	int choice;
	int tmpres, size;
	int filehandle;
	int status;
	char buf[BUFSIZ+1], filename[20], *f;
	char path[50];
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1)
	{
		printf("socket creation failed");
		exit(1);
	}
	char *ip = "10.0.0.6";
	remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
	remote->sin_family = AF_INET;
	tmpres = inet_pton(AF_INET, ip, (void *)(&(remote->sin_addr.s_addr)));
	if( tmpres < 0)  
	{
		perror("Can't set remote->sin_addr.s_addr");
		exit(1);
	}else if(tmpres == 0)
	{
		fprintf(stderr, "%s is not a valid IP address\n", ip);
		exit(1);
	}
	remote->sin_port = htons(PORT);

	tmpres = connect(sock,(struct sockaddr*)remote, sizeof(struct sockaddr));
	if(tmpres == -1)
	{
		printf("Connect Error");
		exit(1);
	}

	/*
	Connection Establishment
	   120
		  220
	   220
	   421
	Login
	   USER
		  230
		  530
		  500, 501, 421
		  331, 332
	   PASS
		  230
		  202
		  530
		  500, 501, 503, 421
		  332
	*/
	char * str;
	int codeftp;
	printf("Connection established, waiting for welcome message...\n");
	//How to know the end of welcome message: http://stackoverflow.com/questions/13082538/how-to-know-the-end-of-ftp-welcome-message
	while((tmpres = recv(sock, buf, BUFSIZ, 0)) > 0){
		sscanf(buf,"%d", &codeftp);
		printf("%s", buf);
		if(codeftp != 220) //120, 240, 421: something wrong
		{
			replylogcode(codeftp);
			exit(1);
		}

		str = strstr(buf, "220 \r\n");//Why ???
		if(str != NULL){
			break;
		}
		memset(buf, 0, tmpres);
	}
	
	//Send Username
	char info[50];
	printf("Name (%s): ", ip);
	memset(buf, 0, sizeof buf);
	scanf("%s", info);

	sprintf(buf,"USER %s\r\n",info);
	tmpres = send(sock, buf, strlen(buf), 0);

	memset(buf, 0, sizeof buf);
	tmpres = recv(sock, buf, BUFSIZ, 0);

	sscanf(buf,"%d", &codeftp);
	if(codeftp != 331)
	{
		replylogcode(codeftp);
		exit(1);
	}
	printf("%s", buf);

	//Send Password
	memset(info, 0, sizeof info);
	//printf("Password: ");
	//memset(buf, 0, sizeof buf);
	//scanf("%s", info);
	char *password;
	password = getpass("Enter Password: "); // get a password
    printf("%s\n",password);

	sprintf(buf,"PASS %s\r\n",password);
	tmpres = send(sock, buf, strlen(buf), 0);

	memset(buf, 0, sizeof buf);
	tmpres = recv(sock, buf, BUFSIZ, 0);

	sscanf(buf,"%d", &codeftp);
	if(codeftp != 230)
	{
		replylogcode(codeftp);
		exit(1);
	}
	printf("%s", buf);
	
	printf("Choose passive mode or active mode: \n");
	printf("  1 - Passvive mode\n");
	printf("  0 - Active mode\n");
	printf("Please enter your choice: ");
	scanf("%d", &mode);
	
	if (mode == 1) //Passive mode
	{
		memset(buf, 0, sizeof buf);
		sprintf(buf,"PASS \r\n");
		tmpres = send(sock, buf, strlen(buf), 0);
		
		memset(buf, 0, sizeof buf);
		tmpres = recv(sock, buf, BUFSIZ, 0);
		char passbuf[BUFSIZ+1];
		
		strcpy(passbuf, buf);
		sscanf(buf,"%d", &codeftp);
		if(codeftp == 227)
		{
			int a1, a2, a3, a4, p1, p2, dataPort;       //PASV Information
			sscanf(passbuf, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &a1,&a2,&a3,&a4,&p1,&p2);
			dataPort = (p1 * 256) + p2;
			
			sock = socket(AF_INET, SOCK_STREAM, 0);
			if(sock == -1)
			{
				printf("socket creation failed");
				exit(1);
			}
			remote->sin_port = htons(dataPort);
			tmpres = connect(sock,(struct sockaddr*)remote, sizeof(struct sockaddr));
			if(tmpres == -1)
			{
				printf("Connect Error");
				exit(1);
			}
			//vòng lặp thực hiện các lệnh
			int i = 1;
			while(1)
			{
				printf("Enter a choice:\n1- get\n2- put\n3- pwd\n4- ls\n5- cd\n6- delete\n7- quit\n");
				scanf("%d", &choice);
				switch(choice)
				{
					case 1:
						printf("Enter filename to get: ");
						scanf("%s", filename);
						sprintf(buf,"GET %s\r\n",filename);
						send(sock, buf, 100, 0);
						recv(sock, &size, sizeof(int), 0);
						if(!size)
						{
							printf("No such file on the remote directory\n\n");
							break;
						}
						f = malloc(size);
						recv(sock, f, size, 0);
						while(1)
						{
							filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666);
							if(filehandle == -1)
							{
								sprintf(filename + strlen(filename), "%d", i);//needed only if same directory is used for both server and client
							}
							else 
							{
								break;
							}
						}
						write(filehandle, f, size);
						close(filehandle);
						strcpy(buf, "cat ");
						strcat(buf, filename);
						system(buf);
						break;
					case 2:
						printf("Enter filename to put to server: ");
						scanf("%s", filename);
						filehandle = open(filename, O_RDONLY);
						if(filehandle == -1)
						{
							printf("No such file on the local directory\n\n");
							break;
						}
						sprintf(buf,"PUT %s\r\n", filename);
						send(sock, buf, 100, 0);
						stat(filename, &obj);
						size = obj.st_size;
						send(sock, &size, sizeof(int), 0);
						sendfile(sock, filehandle, NULL, size);
						recv(sock, &status, sizeof(int), 0);
						if(status)
						{
							printf("File stored successfully\n");
						}
						else
						{
							printf("File failed to be stored to remote machine\n");
						}
						break;
					case 3:
						sprintf(buf,"PWD \r\n");
						send(sock, buf, 100, 0);
						recv(sock, buf, 100, 0);
						printf("The path of the remote directory is: %s\n", buf);
						break;
					case 4:
						sprintf(buf,"PWD \r\n");
						send(sock, buf, 100, 0);
						recv(sock, buf, BUFSIZ, 0);
						//recv(sock, &size, sizeof(int), 0);
						//f = malloc(size);
						//recv(sock, f, size, 0);
						//filehandle = creat("temp.txt", O_WRONLY);
						//write(filehandle, f, size, 0);
						//close(filehandle);
						//printf("The remote directory listing is as follows:\n");
						//system("cat temp.txt");
						break;
					case 5:
						printf("Enter the path to change the remote directory: ");;
						scanf("%s", path);
						sprintf(buf,"CD %s\r\n",path);
						send(sock, buf, 100, 0);
						recv(sock, &status, sizeof(int), 0);
						if(status)
						{
							printf("Remote directory successfully changed\n");
						}
						else
						{
							printf("Remote directory failed to change\n");
						}
						break;
					case 6:
						printf("Enter the path to change the remote directory: ");;
						scanf("%s", path);
						sprintf(buf,"DELETE %s\r\n",path);
						send(sock, buf, 100, 0);
						recv(sock, &status, sizeof(int), 0);
						if(status)
						{
							printf("Remote directory successfully deleted\n");
						}
						else
						{
							printf("Remote directory failed to delete\n");
						}
						break;
					case 7:
						strcpy(buf, "QUIT \r\n");
						send(sock, buf, 100, 0);
						recv(sock, &status, 100, 0);
						if(status)
						{
							printf("Server closed\nQuitting..\n");
							exit(0);
						}
						printf("Server failed to close connection\n");
				}
			}	
		}
	}
	else //Active mode
	{
		
	}
}
