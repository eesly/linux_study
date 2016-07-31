/*
 * Daemon
 * author: sly
 * time  : 2016 05.15
 */

#ifndef CDAEMON_H
#define CDAEMON_H

#include <sys/wait.h>
//#include <sys/types.h>
#include <sys/param.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

class CDaemon
{
	public:
		CDaemon(){}
		~CDaemon(){}

	public:
		static void BecomeDaemon(){
			pid_t pid = fork();
			if (pid < 0){
				perror("fork error!");
				exit(1);
			}

			if (pid > 0) 		//父进程
				exit(0); 	//这里之所以不需要wait是因为，父进程立即退出，子进程退出时会被当作孤儿进程被init回收

			//子进程1
			setsid(); 		//为脱离控制终端，登录会话和进程组，变成新的会话组长

			pid = fork();
			if (pid < 0){
				perror("fork error!");
				exit(1);
			}

			if (pid > 0)		//子进程1退出
				exit(0);

			//子进程2 不再是会话组长
			//关闭文件描述符
			for (int i = 0; i < NOFILE; ++i)
				close(i);

			//更改工作目录
			chdir("/");

			//set mask
			umask(0);

			//重定向标准输入输出
			int fp = open("/dev/null", O_RDWR);
			dup2(fp, STDIN_FILENO);
			dup2(fp, STDOUT_FILENO);
			dup2(fp, STDERR_FILENO);

			//处理SIGCHLD信号
			signal(SIGCHLD, SIG_IGN);

			return;
		}

};

#endif
