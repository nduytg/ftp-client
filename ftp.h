/*
 * ftp.h
 *
 *  Created on: May 4, 2016
 *      Author: nguyenngocduy
 */

#ifndef FTP_H_
#define FTP_H_

//Kieu bool
typedef int bool;
enum {false, true };

void command_handler(int sock, char* cmd);
void parseCode(int command_code);
void pwd();
int disconnect(int sockfd);
int put(int sockfd, char filename[], char localPath[]);


void help();
void bye(int sock);		//QUIT

//recv remote-file [local-file ]
int ftp_recv(int sock, char *arg1, char *arg2);	//RETR

//ls [remote-directory ] [local-file ]
int ls(int sockfd, char *arg1, char *arg2);		//LIST

void cd (int sock_fd, char* s1);
void cdup (int sock_fd);
void MKDIR (int sock_fd, char* s1);
void delete (int sock_fd, char* s1);
void RMDIR (int sock_fd, char* s1);


int Send_cmd (char* s1, char* s2, int sock_fd);
int Reply_cmd(int sock_fd);
void Pasre (char s1[256], int sock_fd);  //s1 la lenh nhap tu nguoi dung

#endif /* FTP_H_ */
