#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define TRUE 1
#define FALSE 0

int argCount(int, char **);
int buildArgsList(char *const, int, char **);
int contains(char *const, char);
int same(char *, char*);
void testManPage(char *, char *);
void parseManPage(char *, FILE *);

int main(int argc, char *argv[])
{
    const int count = argCount(argc, argv);
    char args[count + 1];
    
    const int countNoDupes = buildArgsList(args, argc, argv);
    args[countNoDupes] = '\0';
    
    testManPage(args, argv[1]);
}

void testManPage(char *args, char *command)
{
    //Determine temp file name
    char tmpFile[strlen("/tmp/exp") + strlen(command) + 1];
    strcpy(tmpFile, "/tmp/exp");
    strcat(tmpFile, command);
    printf("temp file: %s\n", tmpFile);
    
    pid_t pID = fork();
    
    //child executes man
    if(pID == 0) {
        if(access(tmpFile, F_OK) != 0) { //does tmp file exist?
            const int file = open(tmpFile, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
            if(file < 0) {
                printf("Error creating temp file. Aborting.\n");
                return;
            }
            dup2(file, 1);
            close(file);
            execlp("man", "man", command, (char *) NULL);
        }
        printf("tmp file already existed!\n");
        exit(0);
    }
    
    //parent waits and operates on result
    else {
        int status;
        waitpid(pID, &status, 0);
        printf("finished. status: %d\n", status);
        
        FILE *fp;
        fp = fopen(tmpFile, "r");
        if(fp == NULL) {
            printf("failure opening tmp file for reading.\n");
            return;
        }
        
        printf("parsing man page.\n");
        parseManPage(args, fp);
        
        if(fp) {
            fclose(fp);
            fp = NULL;
        }
    }
}

void parseManPage(char *args, FILE *fp)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    
    int nameNext = FALSE;
    int optionsNext = FALSE;
    int optionsDescripNext = FALSE;
    while((read = getline(&line, &len, fp)) != -1) {
        if(same(line, "NAME\n")) {
            nameNext = TRUE;
            optionsNext = FALSE;
        }
        else if(nameNext) {
            printf("%s", line);
            nameNext = FALSE;
        }
        else if(same(line, "OPTIONS\n") || same(line, "DESCRIPTION\n")) {
            printf("found options (or description)!\n");
            optionsNext = TRUE;
            nameNext = FALSE;
        }
        else if(optionsNext && !optionsDescripNext) {
            char *token = NULL;
            char linecp[strlen(line) + 1];
            strcpy(linecp, line);
            token = strtok(linecp, " ");
            if(token[0] == '-' && contains(args, token[1])) {
                optionsDescripNext = TRUE;
                printf("%s", line);
            }
        }
        else if(optionsNext && optionsDescripNext) {
            printf("%s", line);
            optionsDescripNext = FALSE;
        }
        
        ////Others
        //else if(same(line, "DESCRIPTION\n")) {
        //    optionsNext = FALSE;
        //    nameNext = FALSE;
        //}
        //else if(same(line, "FILES\n")) {
        //    optionsNext = FALSE;
        //    nameNext = FALSE;
        //}
        //else if(same(line, "EXPRESSIONS\n")) {
        //    optionsNext = FALSE;
        //    nameNext = FALSE;
        //}
        //else if(same(line, "BUGS\n")) {
        //    optionsNext = FALSE;
        //    nameNext = FALSE;
        //}
        //else if(same(line, "SEE ALSO\n")) {
        //    optionsNext = FALSE;
        //    nameNext = FALSE;
        //}
        //else if(same(line, "HISTORY\n")) {
        //    optionsNext = FALSE;
        //    nameNext = FALSE;
        //}
        //else if(same(line, "EXAMPLES\n")) {
        //    optionsNext = TRUE;
        //    nameNext = FALSE;
        //}
    }
    if(line) {
        free(line);
    }
}

int same(char *strA, char *strB)
{
    return !strcmp(strA, strB);
}

int contains(char *const args, char theChar)
{
    for(int i = 0; args[i] != '\0'; i++) {
        if(args[i] == theChar) {
            return TRUE;
        }
    }
    return FALSE;
}

int buildArgsList(char *const args, int argc, char *argv[]) {
    int count = 0;
    for(int i = 1; i < argc; i++) {
        char *const curArg = argv[i];
        const int argLen = strlen(argv[i]);
        if(curArg[0] == '-') {
            for(int j = 1; j < argLen; j++) {
                if(!contains(args, curArg[j])) {
                    args[count] = curArg[j];
                    count++;     
                }

            }
        }
    }
    return count;
}

int argCount(int argc, char *argv[])
{
    int count = 0;
    for(int i = 1; i < argc; i++) {
        char *const curArg = argv[i];
        if(curArg[0] == '-') {
            count += strlen(curArg) - 1;
        }
    }
    return count;
}