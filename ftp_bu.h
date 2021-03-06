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

bool command_handler(int sock, char* cmd);
void parseCode(int command_code);
void pwd();
int disconnect(int sockfd);
int put(int sockfd, char filename[], char localPath[], int mode);


void help();
void bye(int sock);		//QUIT

//recv remote-file [local-file ]
int ftp_recv(int sock, char *arg1, char *arg2);	//RETR

//ls [remote-directory ] [local-file ]
int ls(int sock, char *arg1, char *arg2);		//LIST

#endif /* FTP_H_ */
