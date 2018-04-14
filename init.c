#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define NEW(p) if(!(p=(char *)malloc(128*sizeof(char)))) printf("Molloc ERROR!");
#define PIPE_FILE1 "pipe1.txt"
#define PIPE_FILE2 "pipe2.txt"

typedef struct commandline{
    char cmd[256];
    char *args[128];
    int argnum;
}CMDL;

char rawcmd[256];/* ����������� */
CMDL cmd[10];/* �����в��ɵĸ����֣��Կ�ָ���β */
int count;
int RunCmd()
{
    int i,j;
    int fd[2];
    int fdfrom,fdto,fdadd;
    for(i=0; i<count; i++)
    {
        if(i!=0)
        {
            fd[0] = open ( PIPE_FILE1 , O_CREAT|O_WRONLY|O_TRUNC , 0666 );
            fd[1] = open ( PIPE_FILE2 , O_CREAT|O_RDONLY , 0666 );
            char buf[4096];
            int bytenum;
            while ((bytenum = read(fd[1], buf, 1024)) > 0)
            {
                write(fd[0], buf, bytenum);
            }
            close(fd[0]);
            close(fd[1]);
        }
        fd[0] = open ( PIPE_FILE1 , O_CREAT|O_RDONLY , 0666 );//��Ϊ�ӹܵ��ж������ݵĶ˿�
        fd[1] = open ( PIPE_FILE2 , O_CREAT|O_WRONLY|O_TRUNC , 0666 );//��Ϊ�ӹܵ���д�����ݵĶ˿�

        pid_t pid=fork();
        fflush(stdout);
        fflush(stdin);
        int lastargs = cmd[i].argnum + 1;
        if(pid<0)
        {
            printf("Set pid ERROR!");
            continue;
        }else
        if(pid==0)
        {
            if(i==0)
            {
                for(j=0; j<=cmd[i].argnum; j++)
                {
                    if(strcmp(cmd[i].args[j], "<")==0)
                    {
                        if(j+1>cmd[i].argnum)
                        {
                            printf("<ERROR!");
                            exit(11);
                        }
                        int fdfrom = open(cmd[i].args[j+1], O_CREAT|O_RDONLY, 0666);
                        dup2(fdfrom, fileno(stdin));                                            //���ӱ�׼����
                        if(j<lastargs) lastargs = j;
                    }
                }
            }
            if(i==count-1)
            {
                for(j=0; j<=cmd[i].argnum; j++)
                {
                    if(strcmp(cmd[i].args[j], ">")==0)
                    {
                        if(j+1>cmd[i].argnum)
                        {
                            printf(">ERROR!");
                            exit(12);
                        }
                        int fdto = open(cmd[i].args[j+1], O_CREAT|O_WRONLY|O_TRUNC, 0666);
                        dup2(fdto, fileno(stdout));                                            //���ӱ�׼���
                        if(j<lastargs) lastargs = j;
                    }
                    if(strcmp(cmd[i].args[j], ">>")==0)
                    {
                        if(j+1>cmd[i].argnum)
                        {
                            printf(">>ERROR!");
                            exit(13);
                        }
                        int fdadd = open(cmd[i].args[j+1], O_CREAT|O_WRONLY|O_APPEND, 0666);
                        dup2(fdadd, fileno(stdout));                                            //���ӱ�׼���
                        if(j<lastargs) lastargs = j;
                    }
                }
            }
            cmd[i].args[lastargs] = NULL;
            /*��ʼ����ܵ�*/
            if(i!=0) dup2(fd[0], fileno(stdin));
            if(i!=count-1) dup2(fd[1], fileno(stdout));
            /* �ڽ����� */
            if (strcmp(cmd[i].args[0], "pwd") == 0) {
                char wd[4096];
                puts(getcwd(wd, 4096));
                exit(0);
            }
            execvp(cmd[i].args[0], cmd[i].args);
            return 255;                            //ִ��ʧ��
        }else                           //������
        {
            waitpid(0, NULL, 0);		//�����ȴ��ӽ���ִ�н���
        }
    }
}

void SplitCmd()
{
    /* ��������У��������� */
    int l=0;
    count = 0;
    int i,j,k;
    for (i = 0; rawcmd[i]; i++)
    {
        if(rawcmd[i]=='|')
        {
            for(j=0; j<i-l; j++)
                cmd[count].cmd[j] = rawcmd[l+j];
            cmd[count].cmd[j] = '\0';
            l = i + 1;
            count++;
        }
    }
    for(j=0; j<i-l; j++)                                             //���һ�����û�йܵ�����
        cmd[count].cmd[j] = rawcmd[l+j];
    cmd[count].cmd[j] = '\0';
    count++;
    /*��ⵥ������Ĳ���*/
    for(i=0; i<count; i++)
    {
        cmd[i].argnum=-1;
        int flag=0;
        for(j=0; cmd[i].cmd[j]; j++)
        {
            if(cmd[i].cmd[j]==' '||cmd[i].cmd[j-1]=='\0') flag = 0;                              //0:wait for the next
            for(k=j; cmd[i].cmd[k]==' '; cmd[i].cmd[k]='\0', k++);
            /*�зֺţ�ȥ�ո��Ҽ���һ�������Ľ�����,�ÿ�ʼ��һ��������flag*/
            j = k;
            if((cmd[i].cmd[j]=='<')||(cmd[i].cmd[j]=='>'&&cmd[i].cmd[j-1]!='>')) flag = 2;
            if(cmd[i].cmd[j]!=' '&&flag==0) flag = 1;                     //1:find next
            if(cmd[i].cmd[j])
            {
                if(flag==1)
                {
                    cmd[i].argnum++;
                    cmd[i].args[cmd[i].argnum] = &cmd[i].cmd[j];                                             //argָ����cmd�ϴ���
                    flag = -1;                                              //-1:wait for the end
                }
                if(flag==2)
                {
                    cmd[i].argnum++;
                    if(cmd[i].cmd[j]=='<')
                    {
                        cmd[i].cmd[j] = '\0';
                        NEW(cmd[i].args[cmd[i].argnum]);
                        strcpy(cmd[i].args[cmd[i].argnum], "<\0");
                    }
                    if(cmd[i].cmd[j]=='>')
                    {
                        if(cmd[i].cmd[j+1]=='>')
                        {
                            NEW(cmd[i].args[cmd[i].argnum]);
                            strcpy(cmd[i].args[cmd[i].argnum], ">>\0");
                            cmd[i].cmd[j] = '\0';
                            cmd[i].cmd[j+1] = ' ';
                            continue;
                        }
                        if(cmd[i].cmd[j+1]!='>')
                        {
                            NEW(cmd[i].args[cmd[i].argnum]);
                            strcpy(cmd[i].args[cmd[i].argnum], ">\0");
                            cmd[i].cmd[j] = '\0';
                        }
                    }
                }
            }else
                break;
        }
        cmd[i].args[cmd[i].argnum+1] = NULL;					//�����������û����һ������
    }
}                                                                 //split succeed
int main()
{
    while (1) {
        /* ��ʾ�� */
        printf("# ");
        fflush(stdin);
        fgets(rawcmd, 256, stdin);
        /* �����β�Ļ��з� */
        int i,j,k;
        for (i = 0; rawcmd[i] != '\n'; i++)
            ;
        rawcmd[i] = '\0';
        SplitCmd();                                              //�������
        /* û���������� */
        if (!cmd[0].args[0])
            continue;
        if (strcmp(cmd[0].args[0], "cd") == 0) {
            if (cmd[0].args[1])
                chdir(cmd[0].args[1]);
            continue;
        }
		if (strcmp(cmd[0].args[0], "export") == 0){
			char *name,*value;
			for(j=0; cmd[0].args[1][j]; j++)
			{
				if(cmd[0].args[1][j]=='='){
					cmd[0].args[1][j] = '\0';
					NEW(name);
					strcpy(name, cmd[0].args[1]);
					NEW(value);
					strcpy(value, &cmd[0].args[1][j+1]);
					if(setenv(name, value, 1)==0){
						break;
					}else{
						printf("Path set ERROR!");
						break;
					}
				}
			}
            continue;
		}
		if (strcmp(cmd[0].args[0], "exit") == 0)
            return 0;
        /*��ʼ����ܵ����ض����ѽ��ܵ�֮�������ָ���ض����ڵ���������д���*/
        RunCmd();
    }
}
