#ifndef _POSIX_C_SOURCE
    #define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>  
#define LSH_RL_BUFSIZE 1024
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

void lsh_loop(void);
char *lsh_read_line(void);
char *lsh_read_line(void);
char **lsh_split_line(char *line);
int lsh_execute(char **args);
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_echo(char **args);
int lsh_exit(char **args);
int lsh_record(char **args);
int lsh_replay(char **args);
int my_pid(char **args);
int background(char **args);
int mypipe(char** args1, char** args2);

int record_num = 0;
char record_str[16][150];

int main()
{
    printf("---------------------------------------\n");
    printf("| Hi! Welcome to my shell !           |\n");
    printf("| Wish you have fun here !!           |\n");
    printf("---------------------------------------\n");
    lsh_loop();
    return EXIT_SUCCESS;
}

void lsh_loop(void)
{
    char *line;
    char **args;
    int status;

    do{
        printf("->>> ");
        line = lsh_read_line();
        record_num++;
        args = lsh_split_line(line);
        status = lsh_execute(args);

        free(line);
        free(args);

    }while(status);
        
    
}

char *lsh_read_line(void)
{
#ifdef LSH_USE_STD_GETLINE
  char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us
  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);  // We received an EOF
    } else  {
      perror("lsh: getline\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
#else
#define LSH_RL_BUFSIZE 1024
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();
    
    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buffer[position] = '\0';
      record_str[record_num][position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
      record_str[record_num][position] = c;
    }
    position++;

    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
#endif
}

char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(BUFSIZ * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

char *builtin_str[] = {
  "cd",
  "help",
  "echo",
  "record",
  "replay",
  "mypid",
  "exit"
};

char *info_str[] = {
  "hange directory",
  "show all build-in function info",
  "echo the strings to standard output",
  "show last-16 cmds you typed in",
  "re-execute the cmd showed in record",
  "find and print process-ids",
  "exit shell"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_echo,
  &lsh_record,
  &lsh_replay,
  &my_pid,
  &lsh_exit
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}


int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

int lsh_help(char **args)
{
  int i;
  printf("=================================================\n\n");
  printf("The following are built in:\n\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("%d. %s:   %s\n", i+1, builtin_str[i], info_str[i]);
  }

  printf("\n================================================\n");
  return 1;
}

int lsh_echo(char **args)
{
  int i = 1;

  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if(strcmp(args[1], "-n") == 0)
    {
      i = 2;
      do{
        printf("%s ", args[i]);
      }while(args[++i] != NULL);
        
      lsh_loop();
    }
    else{

      do{
        printf("%s ", args[i]);
      }while(args[++i] != NULL);

      printf("\n");
    }
  }

  return 1;
}

int lsh_record(char **args)
{
  if (args[1] != NULL) {
    fprintf(stderr, "unrequired argument to \"history\"\n");return 3;
  } 
  printf("\nhistory commands:\n\n");
  int i, j=1;
  if(record_num/16>0){
    for (i = record_num-16; i < record_num; i++) 
      {
        if(record_str[i] != NULL && record_str[i] != "" && record_str[i] != " ")
        printf("- %d. %s\n",j++, record_str[i]);
      }
  }
  else
  for (i = 0; i < record_num; i++) 
  {
    if(record_str[i]!=NULL)
      printf("- %d. %s\n", i+1, record_str[i]);
  }

  printf("\n");
  return 1;

}

int lsh_replay(char **args)
{
  if(args[1] == NULL)
  {
    fprintf(stderr, "required argument to \"replay\"\n");return 3;
  }
  char *line;
  char **arg;
  int status;
  record_num--;
  int num = atoi(args[1]);
  if(record_num>15)
  {
    strncpy(record_str[record_num], record_str[record_num+num-17], 150);
    line = record_str[record_num+num-17];
    arg = lsh_split_line(line);
    status = lsh_execute(arg);
    strncpy(record_str[record_num+num-17], record_str[record_num], 150);

    free(arg);
  }
  else
  {
    strncpy(record_str[record_num], record_str[num-1], 150);
    line = record_str[num-1];
    arg = lsh_split_line(line);
    status = lsh_execute(arg);
    strncpy(record_str[num-1], record_str[record_num], 150);
    
    free(arg);
  }

  record_num++;
  return 1;
}

int my_pid(char **args)
{
  if(strcmp(args[1], "-i") == 0)
  {
    printf("process %d executed \"mypid\"\n", getpid());
  }
  else if(strcmp(args[1], "-p") == 0)
  {
    int pid;
    char buf[BUFSIZ*2];
    char procname[32]; 
    FILE *fp;
    pid = atoi(args[2]);

    snprintf(procname, sizeof(procname), "/proc/%u/status", pid);
    fp = fopen(procname, "r");
    if (fp != NULL) {
        size_t ret = fread(buf, sizeof(char), BUFSIZ*2-1, fp);
        if (!ret) {
            return 1;
        } else {
            buf[ret++] = '\0'; 
        }
    }
    else{
      printf("process id not exist\n");
      return 1;
    }
    fclose(fp);
    char *ppid_loc = strstr(buf, "PPid:");
    if (ppid_loc) {
        int k = 6;
        while (ppid_loc[k] != '\n')
        {
          printf("%c", ppid_loc[k]);
          k++;
        }
        printf("\n");
        return 1;
    } else {
        return 1;
    }

  }
  else if(strcmp(args[1], "-c") == 0)
  {
    int pid;
    char buf[BUFSIZ*2];
    char procname[32]; 
    FILE *fp;
    pid = atoi(args[2]);

    snprintf(procname, sizeof(procname), "/proc/%u/task/%u/children", pid, pid);
    fp = fopen(procname, "r");
    if (fp != NULL) {
        size_t ret = fread(buf, sizeof(char), BUFSIZ*2-1, fp);
        if (!ret) {
            return 1;
        } else {
            buf[ret++] = '\0'; 
        }
    }
    else{
      printf("process id not exist\n");
      return 1;
    }
    fclose(fp);
    char *ppid_loc = strstr(buf, "");
    if (ppid_loc) {
        int k = 0;
        while ((ppid_loc[k]>47&&ppid_loc[k]<58) || (ppid_loc[k] == ' '))
        {
          if(ppid_loc[k] == ' ')
            printf("\n");
          else
            printf("%c", ppid_loc[k]);
          k++;
        }
        printf("\n");
        return 1;
    } else {
        return 1;
    }
  }
  else{
    fprintf(stderr, "wrong argument to \"mypid\"\n");return 3;
  }

  return 1;
}

int lsh_exit(char **args)
{
  printf("***********************\n");
  printf("Bye~~See you next time!\n");
  printf("***********************\n");
  return 0;
}

int background(char **args)
{
  int i = 0; 
  while(args[i] != NULL)
  {
    if(strcmp(args[i], "&") == 0)
    {
      args[i] = '\0';
      break;
    }
    i++;
  }

  pid_t pid;
  pid = fork();
  if (pid < 0) {
      perror("fork()");
      return 3;
  }
  if (pid == 0) {
      freopen( "/dev/null", "w", stdout );
      freopen( "/dev/null", "r", stdin ); 
      signal(SIGCHLD,SIG_IGN);
      execvp(args[0], args);
      printf("%s: Wrong command.\n", args[0]);
      return 3;
  }else {
      printf("[Pid]: %d\n", getpid());
      return 1;
  }
  return 1;
}

int mypipe(char** args1, char** args2)
{
  pid_t pid1, pid2;
  int fd[2];
  pipe(fd);

  pid1 = fork();
  if(pid1 < 0) {
    printf("Error forking.\n");
  }
  else if(pid1 == 0) {
    dup2(fd[1], STDOUT_FILENO);
    close(fd[0]);
    lsh_execute(args1);
    exit(0);
  }
  else {
    pid2 = fork();
    if(pid2 < 0) {
      printf("Error forking.\n");
    }
    else if(pid2 == 0) {
      dup2(fd[0], STDIN_FILENO);
      close(fd[1]);
      lsh_execute(args2);
      exit(0);
    }
    else {
      close(fd[0]);
      close(fd[1]);
      waitpid(pid1, NULL, 0);
      waitpid(pid2, NULL, 0);
    }
  }
  return 1;
}

int lsh_execute(char **args)
{
  int i; 
  int stdinDup, stdoutDup, infile=0, outfile=0, check = 0;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }
  
  i=0;

  while(args[i] != NULL)
  {
    if(strcmp(args[i], "<") == 0) {
      infile = open(args[i+1], O_RDONLY);
      args[i] = NULL;
    }
    else if(strcmp(args[i], ">") == 0) {
      outfile = open(args[i+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR | O_CLOEXEC);
      args[i] = NULL;
    } 
    else if(strcmp(args[i], "|") == 0)
    {
      args[i] = NULL;
      mypipe(&args[0], &args[i+1]);
      return 1;
    }
    else if(strcmp(args[i], "&") == 0)
    {
      background(args);
    }
    
    i++;
  }
  

  for (i = 0; i < lsh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      stdinDup = dup(STDIN_FILENO);
      stdoutDup = dup(STDOUT_FILENO);
      if(outfile)
        dup2(outfile, STDOUT_FILENO);
      if(infile)
        dup2(infile, STDIN_FILENO);

      dup2(stdoutDup, STDOUT_FILENO);
      dup2(stdinDup, STDIN_FILENO);


      return (*builtin_func[i])(args);
    }
  }

  pid_t pid;
  pid = fork();
  if(pid < 0) {
    printf("Error forking\n");
  }
  else if(pid == 0) {

    if(outfile)
      dup2(outfile, STDOUT_FILENO);
    if(infile)
      dup2(infile, STDIN_FILENO);

    execvp(args[0], args);
    printf("Command or executable not recognized.\n");
  }
  else {
   waitpid(pid, NULL, 0);
  }

  return 1;
}
