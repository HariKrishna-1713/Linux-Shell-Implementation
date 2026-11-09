#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<time.h>
#include<dirent.h>
#include<sys/types.h>
#include<pwd.h>
#include<grp.h>
#include <readline/history.h>

#define st_atime  st_atim.tv_sec
#define st_mtime  st_mtim.tv_sec
#define st_ctime  st_ctim.tv_sec

#define BLUE    "\033[01;34m"
#define CYAN    "\033[38;5;51m"
#define GREEN   "\033[38;5;46m"
#define RESET   "\033[0m"

int mycd(char *argv[]);
int myhistory(void);
int mystat(char *);
int mycpy(char* , char*);
int myls_opt(char *, char*);


int custom_command(char *argv[])
{
        int result = 0;

        // Identify and execute built-in commands.
        if(!strcmp(argv[0], "mycd"))
        {
                result = mycd(argv);
                if(result == -1)
                        return -1;

                return 0;
        }
        else if(!strcmp(argv[0],"myhistory"))
        {
                result = myhistory();
                if(result == -1)
                        return -1;

                return 0;
        }
        else if(!strcmp(argv[0],"mystat"))
        {
                if(argv[1] == NULL)
                {
                        printf("Usage: stat filename\n");
                        return -1;
                }
                result = mystat(argv[1]);
                if(result == -1)
                        return -1;

                return 0;
        }
        else if(!strcmp(argv[0],"mycp"))
        {
                if(argv[1] == NULL || argv[2] == NULL)
                {
                        printf("Usage: cp source_filename destination_filename\n");
                        return -1;
                }
                result = mycpy(argv[1],argv[2]);
                if(result == -1)
                        return -1;

                return 0;
        }
        else if(!strcmp(argv[0],"myls"))
        {
                result = myls_opt(argv[0],argv[1]);
                if(result == -1)
                        return -1;
                if(result == 1)
                        return 1;

                return 0;
        }

        // Return 1 when the command is not a built-in command.
        return 1;
}


int mycd(char *argv[])
{
        char *path,cwd[1024];//cwd - current working directory

        // If no directory is specified, navigate to HOME.
        if(argv[1] == NULL)
                path = getenv("HOME");
        else
                path = argv[1];

        if(path == NULL)
        {
                printf("HOME not set\n");
                return -1;
        }

        if(chdir(path)==-1)
        {
                perror("cd");
                return -1;
        }
        return 0;
}


int myhistory(void)
{
        HIST_ENTRY **list;
        int i = 0;

        // Retrieve the command history maintained by Readline.
        list = history_list();
        if(list == NULL)
        {
                printf("No commnads in history\n");
                return -1;
        }

        // Display history entries along with their history numbers.
        while(list[i] != NULL)
        {
                printf(" %d   %s\n", history_base + i, list[i]->line);
                i++;
        }
        return 0;
}


int mystat(char *filename)
{
        struct stat s;

        // Retrieve file metadata using stat().
        if(stat(filename,&s)==-1)
        {
                perror("stat");
                return -1;
        }

        printf("File: %s\n",filename);
        printf("size: %ld       ",s.st_size);
        printf("        Blocks: %ld     ",s.st_blocks);
        printf("IO Block:%ld    ",s.st_blksize);
        printf((S_ISDIR(s.st_mode))? "directory": (S_ISREG(s.st_mode))? "regular file":
                (S_ISCHR(s.st_mode))? "Character file": (S_ISBLK(s.st_mode))? "Block file":
                (S_ISFIFO(s.st_mode))? "FIFO": (S_ISLNK(s.st_mode))? "Symbolic Link":"Socket");//(S_ISSOCK(s.st_mode))

        printf("\nDevice: %ld     ",s.st_rdev);
        printf("Inode: %ld      ",s.st_ino);
        printf("Links: %ld\n",s.st_nlink);

        // Display file type and user, group and other permissions.
        printf("Access: ");

        printf("%c",S_ISDIR(s.st_mode)? 'd': S_ISLNK(s.st_mode)? 'l': (S_ISCHR(s.st_mode))? 'c':'-' );

        printf("%c",s.st_mode & S_IRUSR ? 'r':'-');
        printf("%c",s.st_mode & S_IWUSR ? 'w':'-');
        printf("%c",s.st_mode & S_IXUSR ? 'x':'-');

        printf("%c",s.st_mode & S_IRGRP ? 'r':'-');
        printf("%c",s.st_mode & S_IWGRP ? 'w':'-');
        printf("%c",s.st_mode & S_IXGRP ? 'x':'-');

        printf("%c",s.st_mode & S_IROTH ? 'r':'-');
        printf("%c",s.st_mode & S_IWOTH ? 'w':'-');
        printf("%c",s.st_mode & S_IXOTH ? 'x':'-');

        printf("        ");

        printf("Uid : %d        ",s.st_uid);
        printf("Gid : %d\n",s.st_gid);
        printf("Access: %s",ctime(&s.st_atime));
        printf("Modify: %s",ctime(&s.st_mtime));
        printf("Change: %s",ctime(&s.st_ctime));

        return 0;
}


int mycpy(char *srcfilename,char *destfilename)
{
        // Open the source file in read-only mode.
        int fd1=open(srcfilename,O_RDONLY);
        if(fd1==-1)
        {
                perror("open");
                return -1;
        }

        char ch,arr[100000];
        int i=0,ret;

        // Read source file contents into the buffer.
        while((ret=read(fd1,&ch,1))==1)
                arr[i++]=ch;

        if(ret==-1)
        {
                perror("read");
                close(fd1);
                return -1;
        }

        // Create the destination file or truncate it if it already exists.
        int fd2=open(destfilename,O_WRONLY|O_CREAT|O_TRUNC,0660);
        if(fd2==-1)
        {
                perror("open");
                close(fd1);
                return -1;
        }

        // Write the buffered contents into the destination file.
        if(write(fd2,arr,i)==i)
                printf("sucessfully written\n");
        else
        {
                printf("written failed\n");
                close(fd1);
                close(fd2);
                return -1;
        }

        close(fd1);
        close(fd2);

        return 0;
}


int myls_opt(char *cmd,char *options)
{
        struct stat s; struct passwd *P; struct group *g;
        DIR *ptr;
        char path[258];
        //int flag = 0;
        int found = 0;

        // Open the current working directory for reading entries.
        ptr = opendir("./");

        if(ptr == NULL)
        {
                perror("opendir");
                return -1;
        }

        struct dirent *p;

        // Read directory entries one by one.
        while((p = readdir(ptr)) != NULL)
        {
                if(options != NULL && !strcmp(options, "-a"))
                {
                        // -a: Display all entries, including hidden files.
                        if (lstat(p->d_name, &s) == -1)
                        {
                               perror("stat");
                               continue;
                        }

                        if(S_ISDIR(s.st_mode))
                              printf(""BLUE "%s"RESET"  ",p->d_name);

                        else if(S_ISLNK(s.st_mode))
                              printf(""CYAN "%s"RESET"  ",p->d_name);

                        else if(s.st_mode & S_IXUSR)
                              printf(""GREEN "%s"RESET"  ",p->d_name);

                        else
                              printf("%s  ",p->d_name);
                }

                else if(options != NULL && !strcmp(options, "-i"))
                {
                        // -i: Display inode numbers, excluding hidden files.
                        if(p->d_name[0]!='.')
                        {
                                if (lstat(p->d_name, &s) == -1)
                                {
                                        perror("stat");
                                        continue;
                                }

                                if(S_ISDIR(s.st_mode))
                                        printf(""BLUE "%ld %s"RESET"  ",p->d_ino, p->d_name);

                                else if(S_ISLNK(s.st_mode))
                                        printf(""CYAN "%ld %s"RESET"  ",p->d_ino, p->d_name);

                                else if(s.st_mode & S_IXUSR)
                                        printf(""GREEN "%ld %s"RESET"  ",p->d_ino, p->d_name);

                                else
                                        printf("%ld %s  ",p->d_ino,p->d_name);
                        }
                }

                else if(options != NULL && !strcmp(options, "-l"))
                {
                        // -l: Display detailed file information.
                        if(p->d_name[0]!='.')
                        {
                                found = 1;

                                // lstat() retrieves metadata of the link itself, not its target.
                                //snprintf(path, sizeof(path), "./%s", p->d_name);
                                //if you want to use snprintf use path in if instead of p->d_name
                                if (lstat(p->d_name, &s) == -1)//you can use lstat also for symbolic links
                                {
                                        perror("stat");
                                        continue;
                                }

                                printf("%c", S_ISDIR(s.st_mode) ? 'd' :
                                S_ISLNK(s.st_mode) ? 'l' : '-');

                                printf("%c", s.st_mode & S_IRUSR ? 'r' : '-');
                                printf("%c", s.st_mode & S_IWUSR ? 'w' : '-');
                                printf("%c", s.st_mode & S_IXUSR ? 'x' : '-');

                                printf("%c", s.st_mode & S_IRGRP ? 'r' : '-');
                                printf("%c", s.st_mode & S_IWGRP ? 'w' : '-');
                                printf("%c", s.st_mode & S_IXGRP ? 'x' : '-');

                                printf("%c", s.st_mode & S_IROTH ? 'r' : '-');
                                printf("%c", s.st_mode & S_IWOTH ? 'w' : '-');
                                printf("%c", s.st_mode & S_IXOTH ? 'x' : '-');

                                printf("  %ld ",s.st_nlink);

                                // Convert UID and GID into their corresponding names.
                                P = getpwuid(s.st_uid);
                                if(P != NULL)
                                        printf(" %s ",P->pw_name);

                                g = getgrgid(s.st_gid);
                                if(g != NULL)
                                        printf("%s ",g->gr_name);

                                printf("%5ld ",s.st_size);

                                char *f = ctime(&s.st_atime);
                                if(f!=NULL)
                                {
                                        f[strcspn(f,"\n")]='\0';
                                        printf("%s ",f);
                                }

                                // Apply colors based on file type and permissions.
                                if(S_ISDIR(s.st_mode))
                                {
                                        printf(""BLUE "%s"RESET"\n",p->d_name);
                                        //flag = 0;
                                }
                                else if(S_ISLNK(s.st_mode))
                                        printf(""CYAN "%s"RESET"\n",p->d_name);

                                else if(s.st_mode & S_IXUSR)
                                        printf(""GREEN "%s"RESET"\n",p->d_name);

                                else
                                        printf("%s\n",p->d_name);
                        }
                }
                else if(!strcmp(cmd, "myls") && options == NULL)
                {
                        // Default myls: Display non-hidden entries.
                        if(p->d_name[0] != '.')
                        {
                                if(lstat(p->d_name, &s) == -1)
                                {
                                        perror("stat");
                                        continue;
                                }

                                if(S_ISDIR(s.st_mode))
                                        printf(""BLUE "%s"RESET"   ",p->d_name);

                                else if(S_ISLNK(s.st_mode))
                                        printf(""CYAN "%s"RESET"   ",p->d_name);

                                else if(s.st_mode & S_IXUSR)
                                        printf(""GREEN "%s"RESET"   ",p->d_name);

                                else
                                        printf("%s   ",p->d_name);
                        }
                }
                else
                        return 1;
        }

        closedir(ptr);

        if(!found)
                printf("\n");

        return 0;
}