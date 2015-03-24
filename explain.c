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

/*
 * Most man pages are formatted such that the description of the argument
 * is on the line AFTER the argument is stated.  For example, in the man
 * page for ls:
 * 
 *      -a, --all
 *              do not ignore entries starting with .
 *
 * Some man pages, however, start the description on the same line as the
 * argument.  For example, in the man page for Vim:
 *
 *      -A           If Vim has been compiled with Arabic support...
 *
 * 
 */
#define LIMIT 30
#define MORE 1

int argCount(int, char **);
int buildArgsList(char *const, int, char **);
int same(char *, char*);
FILE *createTempManPage(char *, char *);
void parseManPage(char *, FILE *);
char *copyString(char *);
int isEmptySpace(char *);
int stringHasArg(char *const string, char *list);



int main(int argc, char *argv[])
{
    const int count = argCount(argc, argv);
    char args[count + 1];
    
    const int countNoDupes = buildArgsList(args, argc, argv);
    args[countNoDupes] = '\0';
    
    FILE *manPage = createTempManPage(args, argv[1]);
    
    if(manPage)
        parseManPage(args, manPage);
}

/**
 * Creates a temporary file for holding the man page
 * output if no existing temp file for that man
 * page is found. Then it outputs the result
 * of calling "man [command]" to the temp file.
 *
 * Returns an (unclosed) file pointer to the
 * created temp file.
 */
FILE *createTempManPage(char *args, char *command)
{
    //Determine temp file name
    char tmpFile[strlen("/tmp/exp") + strlen(command) + 1];
    strcpy(tmpFile, "/tmp/exp");
    strcat(tmpFile, command);
    printf("temp file: %s\n", tmpFile);
    
    pid_t pID = fork();
    
    //child executes man
    if(pID == 0) {
        if(access(tmpFile, F_OK) != 0) { //make temp file if necessary
            const int file = open(tmpFile, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
            if(file < 0) {
                printf("Error creating temp file. Aborting.\n");
                return NULL;
            }
            dup2(file, 1);  //redirect output to file
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
        }
        
        return fp;
    }
}

void parseManPage(char *args, FILE *fp)
{
    int inDescription = FALSE;
    char *lastLine = NULL;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    
    int nameNext = FALSE;
    int optionsNext = FALSE;
    int optionsDescripNext = FALSE;
    while((read = getline(&line, &len, fp)) != -1) {
        if(lastLine && same(lastLine, "NAME\n")) {
            printf("%s\n", line);
        }
        else if(lastLine){
            if(inDescription) { // still in a multi-line description
                if(isEmptySpace(line)) {
                    inDescription = FALSE;
                    printf("\n");
                }
                else {
                    //some man pages have the description on the same
                    //line as the argument (eg: -c    will do a copy)
                    //
                    printf("%s", line);
                }
            }
            else {
                char linecp[strlen(lastLine) + 1];
                strcpy(linecp, lastLine);
                char *token = strtok(linecp, " \t"); //first token of lastLine
                if(token && token[1] && token[0] == '-') {
                    if(token[2] && token[1] == '-') {
                      //is a double, eg --color
                    }
                    else {
                        if(strchr(args, token[1]) || (MORE && read < LIMIT && stringHasArg(lastLine, args))) {
                            printf("%s", lastLine);
                            if(isEmptySpace(line) == FALSE) {    
                                inDescription = TRUE;
                                printf("%s", line);
                            }
                        }
                    }
                }
            }
        }
        
        if(lastLine)
            free(lastLine);
        lastLine = copyString(line);
    }
    
    if(line)
        free(line);
    if(lastLine)
        free(lastLine);
}

int isEmptySpace(char *line) {
    while(line && *line != '\0') {
        if(*line != '\n' && *line != '\t' && *line != ' ')
            return FALSE;
        line++;
    }
    return TRUE;
}

int same(char *strA, char *strB)
{
    return !strcmp(strA, strB);
}

char *copyString(char *original)
{
    char *copy = (char *)malloc(strlen(original) + 1);
    strcpy(copy, original);
    return copy;
}

/*
 * Return TRUE iff string contains any of
 * the chars in list, preceded by -
 * Ex: if string is "-R, -r, --recursive"
 * and list is "qrs", returns true.
 */
int stringHasArg(char *const string, char *list)
{
    const int len = strlen(list);
    char buf[2];
    for(int i = 0; i < len; i++) {
        sprintf(buf, "-%c", list[i]);
        if(strstr(string, buf)) {
            printf("[%s] has [%s]\n", string, buf);
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * Places each argument in args
 * without duplicates. Returns
 * number of unique args
 */
int buildArgsList(char *const args, int argc, char *argv[]) {
    int count = 0;
    for(int i = 1; i < argc; i++) {
        char *const curArg = argv[i];
        const int argLen = strlen(argv[i]);
        if(curArg[0] == '-') {
            for(int j = 1; j < argLen; j++) {
                if(!strchr(args, curArg[j])) {
                    args[count] = curArg[j];
                    count++;     
                }

            }
        }
    }
    return count;
}

/**
 * Returns the amount of arguments
 * found in argv, including "combined"
 * arguments.
 * Example: -a returns 1
 *          -abc returns 3
 *          -a -b -c returns 3
 */
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