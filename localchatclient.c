/**
 *
 * @file localchatclient.c
 * @brief 本地聊天室的客户端
 *
 * 用户可以通过本地聊天室的客户端，与其他用户进行聊天。
 *
 * @version 0.1.5
 * @author 史双雷
 * @data 2019年12月14日
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define SEND 1
#define QUIT 0
#define ILLEAGLE_INPUT -1

void sys_error(char *);
void online(int , char *);
int parse_command(char *, size_t);
void parse_data(char *, size_t);
void send(int , char *, size_t,  char *);
void quit(int, char *);

int main(int argc, char *argv[]){

	if(argc < 2){
		printf("./localchatclient name\n");
		exit(1);
	}
	char buff[4096];

	//向localchatserve发送登录包
	int serve_fifofd;
	if((serve_fifofd = open("./.servefifo", O_WRONLY)) < 0){
		sys_error("open ./.servefifo");
	}

	online(serve_fifofd, argv[1]);

	//打开client  fifo
	int client_fifofd;
	char client_fifo[255];
	sprintf(client_fifo, "./.%sfifo", argv[1]);
	while((client_fifofd = open(client_fifo, O_RDONLY|O_NONBLOCK)) < 0){
		if(errno != 2)
			sys_error("open client fifo");
	}
	
	//将标准输入变为非阻塞打开方式
	int flags = fcntl(STDIN_FILENO, F_GETFL);
	flags |= O_NONBLOCK;
	if(fcntl(STDIN_FILENO, F_SETFL, flags) == -1){
		sys_error("fcntl STDIN_FILENO");
	}

	int nread;
	while(1){
		//从键盘非阻塞读取命令
		if((nread = read(STDIN_FILENO, buff, sizeof(buff))) < 0)
		{
			if(errno != EAGAIN)
				sys_error("read STDIN");
		}
		//解析命令
		else if(nread > 0){
			int command = parse_command(buff, nread);
			if(command == SEND)
				send(serve_fifofd, buff, nread, argv[1]);
			else if(command == QUIT){
				quit(serve_fifofd, argv[1]);
				close(serve_fifofd);
				unlink(client_fifo);
				close(client_fifofd);
				break;
			}
		}
		//非阻塞接受localchatserve发送的包
		if((nread = read(client_fifofd, buff, sizeof(buff))) < 0){
			if(errno != EAGAIN)
				sys_error("read data");
		}
		//解析包
		else if(nread > 0){
			parse_data(buff, nread);
		}
	}

	return 0;
}

/**
 * @brief 向localchatserve发送登陆包
 *
 * @param fd serve fifofd
 * @param name 登录人的名字
 */
void online(int fd, char *name){
	char data[4096];
	sprintf(data, "online %s MCP/0.1\r\n", name);
	write(fd, data, strlen(data));
}

/**
 * @brief 输出系统错误信息，并异常退出
 */
void sys_error(char *info){
	perror(info);
	exit(1);
}

/**
 * @brief 解析命令，并处理命令
 *
 * @param command 命令行
 * @param size 命令行的长度
 */
int parse_command(char *command, size_t size){
	if(strncmp(command, "to", 2) == 0){
		return SEND;
	}
	else if(strncmp(command, "quit", 4) == 0){
		return QUIT;
	}
	return ILLEAGLE_INPUT;

}
/**
 * @brief 处理to命令
 *
 * @param fd serve fifofd
 * @param command 命令行
 * @param size 命令行的长度
 * @param name 发送方的名字
 */
void send(int fd, char *command, size_t size, char *name){
	char data[4096];
	sprintf(data, "chat %s MCP/0.1\r\n", name);

	size_t i = 3, j = strlen(data);
	while(i < size && command[i] != ':'){
		data[j++] = command[i++];
	}
	data[j++] = '\r';
	data[j++] = '\n';

	i++;
	while(i < size){
		data[j++] = command[i++];
	}
	data[j++] = '\r';
	data[j++] = '\n';
	data[j] = '\0';

	write(fd, data, strlen(data));
}

/**
 * @brief 退出登录
 *
 * @param fd serve fifofd
 * @param name 名字
 */
void quit(int fd, char *name){
	char data[4096];
	sprintf(data, "offline %s MCP/0.1\r\n", name);

	write(fd, data, strlen(data));
}
/**
 * @brief 解析收到的数据
 *
 * @param info 收到的数据
 * @param size 数据的大小
 */
void parse_data(char *info, size_t size){
	char buff[4096];
	size_t i, j;
	if(strncmp(info, "info ", 5) == 0)
	{
		sprintf(buff, "from ");

		i = 5;
		while(i < size && !(info[i++] == '\r' && info[i] == '\n'));

		i++;
		j = strlen(buff);
		while(i < size && !(info[i] == '\r' && info[i+1] == '\n')){
			buff[j++] = info[i++];
		}
		buff[j++] = ':';

		i += 2;
		while(i < size){
			buff[j++] = info[i++];
		}
		buff[j] = '\0';
	}
	else{
		i = 0;
		while(i < size && info[i++] != '\n');

		j = 0;
		char name[255];
		while(i < size && info[i] != '\r'){
			name[j++] = info[i++];
		}
		name[j] = '\0';

		sprintf(buff, "%s isn't online.\n", name);
	}
	write(STDOUT_FILENO, buff, strlen(buff));
}
