#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>

#define SHORT 0
#define LONG 1
#define NA 2

#define DEBUG 1

#define SUCCESS 0
#define FAILURE 1

int buildOptLists(int, char **, char **, char ***);
//int buildArgsList(char *const, int, char **);
int same(char *, char*);
FILE *createTempManPage(char *);
void parseManPage(FILE *, char *, char **);
char *copyString(char *);
int isEmptySpace(char *);
int stringHasArg(char *const string, char *list);
int optType(char *opt);
int stringHasLongOpt(char *const string, char *opt);
int listContains(char **list, char *string);
void freeList(char **list);

void db(char *);
void dbi(char *, int);
void dbs(char *message, char *string);
int countSmallOpts(int argc, char *argv[]);
int lineMatchesList(char *line, char **list);



int main(int argc, char *argv[])
{
    if(argc < 3) {
        printf("Usage: explain command opt1 opt2 opt3...\n");
        exit(1);
    }
    
    char *smallOpts = NULL; // -a, -abc, etc
    char **longOpts = NULL;  // --std=c99, --silent, etc
    if(buildOptLists(argc, argv, &smallOpts, &longOpts) > 0) {
        return FAILURE;
    }
    
    FILE *manPage = createTempManPage(argv[1]);
    
    if(manPage) {
        parseManPage(manPage, smallOpts, longOpts);
        fclose(manPage);
    }
    free(smallOpts);
    freeList(longOpts);
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
FILE *createTempManPage(char *command)
{
    //Determine temp file name
    const char const* tempLocation = "/tmp/exp";
    char tmpFile[strlen(tempLocation) + strlen(command) + 1];
    strcpy(tmpFile, tempLocation);
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
        return NULL;
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

//void parseManPage(char *args, FILE *fp)
void parseManPage(FILE *fp, char *smallOpts, char **longOpts)
{
    bool inDescription = false;
    char *lastLine = NULL;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    
    while((read = getline(&line, &len, fp)) != -1) {
        if(lastLine && same(lastLine, "NAME\n")) {
            printf("%s", line);
        }
        else if(lastLine){
            if(inDescription) { // still in a multi-line description
                if(isEmptySpace(line)) {
                    inDescription = false;
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
                char *token = strtok(linecp, " \t\n"); // first token of lastLine
                const int type = optType(token);
                if(type != NA) {
                    if(type == LONG && listContains(longOpts, &token[2])) {
                        printf(lastLine);
                        if(isEmptySpace(line) == false) {
                            inDescription = true;
                            printf(line);
                        }
                    }
                    else if(type == SHORT && strchr(smallOpts, token[1])) {
                        printf(lastLine);
                        if(isEmptySpace(line) == false) {
                            inDescription = true;
                            printf(line);
                        }
                    }
                    
                    else if(lineMatchesList(lastLine, longOpts) || stringHasArg(lastLine, smallOpts)) {
                        printf(lastLine);
                        if(isEmptySpace(line) == false) {
                            inDescription = true;
                            printf(line);
                        }
                    }
                }
            }
        }
        
        if(lastLine) {
            free(lastLine);
            
        }
        lastLine = copyString(line);
    }
    
    free(line);
    free(lastLine);
    
    

}

bool lineMatchesList(char *line, char **list)
{
    char *copy = copyString(line);
    char *token = strtok(copy, " ,;\n");
    for(int i = 0; token != NULL && token[0] && token[0] == '-'; i++) {
        if(strlen(token) >= 3 && token[0] == '-' && token[1] == '-' && listContains(list, &token[2])) {
            free(copy);
            return true;
        }
        token = strtok(NULL, " ,;\n");
    }
    free(copy);
    return false;
}

bool listContains(char **list, char *string)
{
    for(int i = 0; list[i] != NULL; i++) {
        if(same(list[i], string))
            return true;
    }
    return false;
}

int optType(char *opt)
{
    if(!opt || strlen(opt) < 1) {
        return NA;
    }
    const int len = strlen(opt);
    if (len > 2 && opt[0] == '-' && opt[1] == '-') {
        return LONG;
    }
    else if (len >= 2 && opt[0] == '-') {
        return SHORT;
    }
    
    return NA;
}

bool isEmptySpace(char *line) {
    while(line && *line != '\0') {
        if(*line != '\n' && *line != '\t' && *line != ' ')
            return false;
        line++;
    }
    return true;
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
 * Return true iff string contains any of
 * the chars in list, preceded by -
 * Ex: if string is "-R, -r, --recursive"
 * and list is "qrs", returns true.
 */
bool stringHasArg(char *const string, char *list)
{
    char *const copy = copyString(string);
    char *token = strtok(copy, " ");
    while(token != NULL) {
        if(token[0] != '-') {
            free(copy);
            return false;
        }
        else if(token[1] && strchr(list, token[1])) {
            free(copy);
            return true;
        }
        token = strtok(NULL, " ");
    }
    free(copy);
    return false;
}


int stringHasLongOpt(char *const string, char *opt)
{
    char *const copy = copyString(string);
    char *token = strtok(copy, " ,;\n");
    while(token != NULL) {
        if(token[0] && token[0] == '-' && token[1] && token[1] == '-' && token[2]) { // less pretty than strlen, but simpler
            printf("[%s] v [%s]\n", &token[2], opt);
            if(same(&token[2], opt)) {
                free(copy);
                return true;
            }
        }
        token = strtok(NULL, " ,;");
    }
    free(copy);
    return false;
}

int buildOptLists(int argc, char *argv[], char **smallOpts, char **longOpts[])
{    
    dbi("buildOptLists w argc", argc);
    /**
     * Allocate a string of length argc to hold the "small" options (-r, -A, etc).
     * This one string will be a concatenation of each small opt.
     */
    *smallOpts = (char *)malloc(countSmallOpts(argc, argv) + 1); // max of argc-1 smallOpts + '\0'
    
    char *const smallPtr = *smallOpts; // convenience pointer
    if(!smallPtr) {
        printf("Malloc error, quitting.\n");
        return FAILURE;
    }
    smallPtr[0] = '\0';
    
    /**
     * Allocate an array of string pointers for "long" options (--quiet, --color=blue, etc)
     * of size argc
     */
    *longOpts = (char **)malloc(sizeof(char *) * argc); // welcome to pointer hell
    
    char **longPtr = *longOpts; // convenience pointer
    if(!longPtr) {
        printf("Malloc error, quitting.\n");
        return FAILURE;
    }
    longPtr[0] = NULL;
    
    db("    past mallocs");
    
    /**
     * Populate our opt lists
     */
    int longCount = 0;
    for(int i = 2; i < argc; i++) {
        dbs("    arg: ", argv[i]);
        dbi("        is argv", i);
        char *const curArg = argv[i];
        const int type = optType(curArg);
        if(type == LONG) {
            db("        is long opt");
            char *const longOpt = (char *)malloc(strlen(curArg) - 1); // +1 (for '\0'), -2 (for --) = -1
            
            strcpy(longOpt, &curArg[2]); // copy Opt from --Opt
            longPtr[longCount] = longOpt;
            longPtr[longCount+1] = NULL;
            longCount++;
        }
        else if(type == SHORT) {
            db("        is short opt");
            strcat(smallPtr, &curArg[1]);
        }
    }
    
    db("    returning from buildOptLists");
    return SUCCESS;
}

int countSmallOpts(int argc, char *argv[])
{
    int count = 0;
    for(int i = 2; i < argc; i++) {
        char *const curArg = argv[i];
        if(curArg[0] == '-' && curArg[1] && curArg[1] != '-' && curArg[1] != '\0') {
            count += strlen(&curArg[1]);
        }
    }
    dbi("counted", count);
    return count;
}

void freeList(char **list)
{
    for(int i = 0; list[i] != NULL; i++) {
        free(list[i]);
        
    }
    free(list);
    
}

void db(char *message)
{
    if(DEBUG)
        printf("[debug] %s\n", message);
}

void dbi(char *message, int i)
{
    if(DEBUG)
        printf("[debug] %s: %d\n", message, i);
}

void dbs(char *message, char *string)
{
    if(DEBUG)
        printf("[debug] %s %s\n", message, string);
}