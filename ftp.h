/*
 * ftp.h
 *
 *  Created on: May 4, 2016
 *      Author: nguyenngocduy
 */

#ifndef FTP_H_
#define FTP_H_

void parseCode(int command_code);
void pwd();
int disconnect(int sockfd);
int put(int sockfd, char filename[], char localPath[], int mode);


#endif /* FTP_H_ */
