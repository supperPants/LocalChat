/**
 * @file localchatserve.c
 * @brief 本地聊天室的服务端
 *
 * 监视用户的登录，退出情况。并且转发用户的信息。
 *
 * @version 0.1.3
 * @author 史双雷
 * @data 2019年12月14日
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define ONLINE 1
#define CHAT 2
#define OFFLINE 3

typedef struct Node{
	char name[255];
	int fifofd;
	struct Node *next;
}Node, *Nodep;

void sys_error(char *);
int readline(int, char *, size_t);
int parse_packet(char *, size_t);
void online(char *, size_t, Nodep);
void offline(char *, size_t, Nodep);
int main(){
	//创建并打开serve fifo
	if(mkfifo("./.servefifo", 0766) < 0){
		sys_error("mkfifo .servefifo");
	}
	int serve_fifofd;
	if((serve_fifofd = open("./.servefifo", O_RDONLY)) < 0){
		sys_error("open ./.servefifo");
	}
	//阻塞读取serve fifo
	int nread;
	char buff[4096];
	int packet;
	Node load_list;
	load_list.next = NULL;
	while(1){
		//解析读取的请求
		if((nread = readline(serve_fifofd, buff, sizeof(buff))) < 0){
			sys_error("read .servefifo");
		}
		if(nread == 0)
			continue;
		packet = parse_packet(buff, nread);
		//登录请求
		if(packet == ONLINE){
			online(buff, nread, &load_list);
		}
		//退出请求
		else if(packet == OFFLINE){
			offline(buff, nread, &load_list);
		}
		//发送信息请求
		else{
			char sender_name[255];
			size_t i = 5, j = 0;
			while(i < nread && buff[i] != ' '){
				sender_name[j++] = buff[i++];
			}
			sender_name[j] = '\0';
			
			if((nread = readline(serve_fifofd, buff, sizeof(buff))) < 0){
				sys_error("read .servefifo");
			}
			
			char receiver_name[255];
			i = 0, j = 0;
			while(i < nread && buff[i] != '\r'){
				receiver_name[j++] = buff[i++];
			}
			receiver_name[j] = '\0';

			Nodep p = load_list.next;
			char data[4096];
			while(p){
				if(strcmp(p->name, receiver_name) == 0){
					sprintf(data, "info MCP/0.1\r\n%s\r\n", sender_name);

					if((nread = readline(serve_fifofd, buff, sizeof(buff))) < 0){
						sys_error("read .servefifo");
					}
					
					j = strlen(data);
					i = 0;
					while(i < nread){
						data[j++] = buff[i++];
					}
					data[j] = '\0';
					
					write(p->fifofd, data, strlen(data));
					break;
				}
				p = p->next;
			}
			if(!p){
				sprintf(data, "notonline MCP/0.1\r\n%s\r\n", receiver_name);
				p = load_list.next;
				while(p){
					if(strcmp(sender_name, p->name) == 0){
						write(p->fifofd, data, strlen(data));
						break;
					}
					p = p->next;
				}

			}
		}
	}
}

/**
 * @brief 系统错误提示，并退出
 */
void sys_error(char *info){
	perror(info);
	exit(1);
}

/**
 * @brief 从fd中读取一行信息
 *
 * @param fd 文件描述符
 * @param buf 缓冲区
 * @param size 缓冲区大小
 * @return 返回读取到信息的大小。如果返回-1,发生错误，设置errno
 */
int readline(int fd, char *buf, size_t size){
	char c;
	size_t i = 0;
	int nread;
	while((nread = read(fd, &c, 1)) > 0){
		if(c == '\n'){
			buf[i++] = '\n';
			buf[i] = '\0';
			break;
		}
		else
			buf[i++] = c;
	}
	if(nread < 0){
		return -1;
	}
	return i;

}

/**
 * @brief 解析请求包
 *
 * @param packet 请求包
 * @param size 请求包的大小
 * @return 返回请求包的类型
 */
int parse_packet(char *packet, size_t size){
	if(strncmp(packet, "online", 6) == 0)
		return ONLINE;
	else if(strncmp(packet, "chat", 4) == 0)
		return CHAT;
	return OFFLINE;
}
/**
 * @brief 处理登录请求包
 *
 * @param packet 请求包
 * @param size 请求包大小
 * @param load_list 在线用户链表
 */
void online(char *packet, size_t size, Nodep load_list){
	//创建用户结点
	Nodep pnode = (Nodep)malloc(sizeof(Node));
	pnode->next = NULL;

	size_t i = 7, j = 0;
	while(i < size && packet[i] != ' '){
		pnode->name[j++] = packet[i++]; 
	}
	pnode->name[j] = '\0';

	//创建用户fifo文件	
	char fifo[512];
	sprintf(fifo, "./.%sfifo", pnode->name);
	if(mkfifo(fifo, 0766) < 0){
		sys_error("mkfifo .clientfifo");
	}
	
	//打开用户fifo文件，并保存文件描述符
	int client_fifofd;
	if((client_fifofd = open(fifo, O_WRONLY)) < 0){
		sys_error("open .clientfifo");
	}
	pnode->fifofd = client_fifofd;

	Nodep temp = load_list->next;
	load_list->next = pnode;
	pnode->next = temp;
}
/**
 * @brief 处理离线请求包
 *
 * @param packet 请求包
 * @param size 请求包大小
 * @param load_list 在线用户链表
 */
void offline(char *packet, size_t size, Nodep load_list){
	//在load_list中找到退出用户的结点
	char name[255];
	size_t i = 8, j = 0;
	while(i < size && packet[i] != ' '){
		name[j++] = packet[i++];
	}
	name[j] = '\0';
	
	Nodep p = load_list->next;
	Nodep pre = load_list;
	while(p){
		if(strcmp(name, p->name) == 0){		
			//关闭用户fifo
			close(p->fifofd);
			//删除用户节点
			pre->next = p->next;
			free(p);
		}
		pre = p;
		p = p->next;
	}
}
