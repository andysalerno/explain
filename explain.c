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

#define SUCCESS 0
#define FAILURE 1

int buildOptLists(int, char **, char **, char ***);
bool same(char *, char*);
FILE *createTempManPage(char *);
void printOptions(FILE *, char *, char **);
char *copyString(char *);
bool isEmptySpace(char *);
bool lineHasShortOpt(char *const string, char *list);
int optType(char *opt);
bool stringHasLongOpt(char *const string, char *opt);
bool listContains(char **list, char *string);
void freeList(char **list);
int countSmallOpts(int argc, char *argv[]);
bool lineHasLongOpt(char *line, char **list);


int main(int argc, char *argv[])
{
    if(argc < 2) {
        printf("Usage: explain command opt1 opt2 opt3...\n");
        exit(1);
    }
    
    char *smallOpts = NULL;  // -a, -abc, etc
    char **longOpts = NULL;  // --std=c99, --silent, etc
    if(buildOptLists(argc, argv, &smallOpts, &longOpts) > 0) {
        return FAILURE;
    }
    
    FILE *manPage = createTempManPage(argv[1]);
    
    if(manPage) {
        printOptions(manPage, smallOpts, longOpts);
        fclose(manPage);
    }
    
    free(smallOpts);
    freeList(longOpts);
}

/**
 * Populates smallOpts, a string, and longOpts, a list of strings, based upon
 * the options found in argv.
 *
 * Example: if argv is: {explain, ls, -a, -bc, --author, --recursive}
 * Then smallOpts will be string "abc," and longOpts will hold two pointers:
 * one to a string "author," another to a string "recursive."
 *
 * Both smallOpts, longOpts, and the strings pointed to by longOpts will
 * need to be freed after calling this method.
 */
int buildOptLists(int argc, char *argv[], char **smallOpts, char **longOpts[])
{
    /**
     * Allocate string for "small" options (-a, -b, -cdef)
     */
    *smallOpts = (char *)malloc(countSmallOpts(argc, argv) + 1);
    
    char *const smallPtr = *smallOpts; // convenience pointer
    if(!smallPtr) {
        printf("Allocation error, quitting.\n");
        return FAILURE;
    }
    smallPtr[0] = '\0';
    
    /**
     * Allocate an array of string pointers for "long" options (--quiet, --color=blue, etc)
     * of size argc
     */
    *longOpts = (char **)malloc(sizeof(char *) * argc); // glimpse of pointer hell
    
    char **longPtr = *longOpts; // convenience pointer
    if(!longPtr) {
        printf("Malloc error, quitting.\n");
        return FAILURE;
    }
    longPtr[0] = NULL; // pointer list is terminated by NULL
        
    /**
     * Populate our opt lists
     */
    int longCount = 0;
    for(int i = 2; i < argc; i++) {
        char *const curArg = argv[i];
        const int type = optType(curArg);
        if(type == LONG) {
            char *const longOpt =
                (char *)malloc(strlen(curArg) - 1); // +1 (for '\0'), -2 (for --) = -1
            
            strcpy(longOpt, &curArg[2]); // copy Opt from --Opt
            longPtr[longCount] = longOpt;
            longPtr[longCount+1] = NULL;
            longCount++;
        }
        else if(type == SHORT) {
            strcat(smallPtr, &curArg[1]);
        }
    }
    
    return SUCCESS;
}

/**
 * Creates a temporary file for holding the man page
 * output if no existing temp file for that man
 * page is found. Then it outputs the result
 * of calling "man [command]" to the temp file.
 *
 * Returns a file pointer to the
 * created temp file (must be closed later)
 */
FILE *createTempManPage(char *command)
{
    //Determine temp file name
    const char const* tempLocation = "/tmp/exp";
    char tmpFile[strlen(tempLocation) + strlen(command) + 1];
    strcpy(tmpFile, tempLocation);
    strcat(tmpFile, command);
    
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
        return NULL;
    }
    
    //parent waits and operates on result
    else {
        int status;
        waitpid(pID, &status, 0);
        
        FILE *fp;
        fp = fopen(tmpFile, "r");
        if(fp == NULL) {
            printf("failure opening tmp file for reading.\n");
        }
        
        return fp;
    }
}

/**
 * Parses through the temp man page file in search for
 * any options matching those in string smallOpts or
 * those in list longOpts.
 *
 * Will print out the line containing the option as well as
 * all following lines making up the description.
 */
void printOptions(FILE *fp, char *smallOpts, char **longOpts)
{
    bool inDescription = false;
    char *lastLine = NULL; // the "lookbehind" line. Holds copy of previous line
    char *line = NULL;     // the current line
    size_t len = 0;
    ssize_t read;
    
    while((read = getline(&line, &len, fp)) != -1) {
        if(lastLine && same(lastLine, "NAME\n")) {
            printf("%s", line);
        }
        if(!lastLine) {
            lastLine = copyString(line);
            continue;
        }
        if(inDescription) { // still in a multi-line description
            if(isEmptySpace(line)) {
                inDescription = false;
                printf("\n");
            }
            else {
                printf("%s", line);
            }
        }
        else {
            char linecp[strlen(lastLine) + 1];
            strcpy(linecp, lastLine);
            char *token = strtok(linecp, " \t\n"); // first token of lastLine
            const int type = optType(token);

            if(type != NA && (lineHasLongOpt(lastLine, longOpts)
                              || lineHasShortOpt(lastLine, smallOpts))) {
                printf(lastLine);
                if(!isEmptySpace(line)) {
                    inDescription = true;
                    printf(line);
                }
            }
        }
        free(lastLine);
        lastLine = copyString(line);
    }
    
    free(line);
    free(lastLine);
}

/**
 * Return true iff line contains any of the strings in list
 * This method reads through line in search for long options
 * (--option) and returns true if list contains any matches.
 *
 * This method only reads as long as each token begins with '-'
 * and stops when it reaches one that doesn't.
 * This is because man pages tend to have lists of different
 * synonymous options, such as:
 * -R, -r, --recursive
 */
bool lineHasLongOpt(char *line, char **list)
{
    char *copy = copyString(line);
    char *token = strtok(copy, " ,;\n");
    int len = strlen(token);
    for(int i = 0; token && len > 1 && token[0] == '-'; i++) {
        if(len >= 3 && token[0] == '-' && token[1] == '-' && listContains(list, &token[2])) {
            free(copy);
            return true;
        }
        token = strtok(NULL, " ,;\n");
        if(token)
            len = strlen(token);
        else
            len = 0;
    }
    free(copy);
    return false;
}

/**
 * Return true iff list contains string.
 * List is expected to be terminated with
 * NULL pointer.
 */
bool listContains(char **list, char *string)
{
    for(int i = 0; list[i] != NULL; i++) {
        if(same(list[i], string))
            return true;
    }
    return false;
}

/**
 *  Return macro LONG, SHORT, or NA:
 *
 *  LONG iff opt has length > 2 and begins with '--'
 *  SHORT iff opt has length >=2 and begins with '-'
 *  NA otherwise
 */
int optType(char *opt)
{
    if(!opt)
        return NA;  
    const int len = strlen(opt);
    if(!opt || len < 1) {
        return NA;
    }
    if (len > 2 && opt[0] == '-' && opt[1] == '-') {
        return LONG;
    }
    else if (len >= 2 && opt[0] == '-') {
        return SHORT;
    }
    return NA;
}

/**
 *  Return true iff line contains only empty space
 *  (has only spaces, tabs, or newline chars)
 */
bool isEmptySpace(char *line) {
    while(line && *line != '\0') {
        if(*line != '\n' && *line != '\t' && *line != ' ')
            return false;
        line++;
    }
    return true;
}

/**
 * Convenience method. Same as strcmp but returns bool values.
 */
bool same(char *strA, char *strB)
{
    return !strcmp(strA, strB);
}

/**
 * Return a pointer to a newly allocated
 * copy of the input string. (Must be freed later)
 */
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
bool lineHasShortOpt(char *const string, char *list)
{
    char *const copy = copyString(string);
    char *token = strtok(copy, " ,;");
    while(token && strlen(token) >= 2) {
        if(token[0] != '-') {
            free(copy);
            return false;
        }
        else if(token[1] && strchr(list, token[1])) {
            free(copy);
            return true;
        }
        token = strtok(NULL, " ,;");
    }
    free(copy);
    return false;
}

/**
 * Return true iff string contains the option as a substring.
 * Opt should be formatted --option (the -- will be stripped).
 */
bool stringHasLongOpt(char *const string, char *opt)
{
    char *const copy = copyString(string);
    char *token = strtok(copy, " ,;\n");
    while(token != NULL) {
        if(strlen(token) >= 3 && token[0] == '-' && token[1] == '-') {
            if(same(&token[2], opt)) {
                free(copy);
                return true;
            }
        }
        token = strtok(NULL, " ,;\n");
    }
    free(copy);
    return false;
}


/**
 * Return the amount of "small" options found in argv.
 * Ex: argv of {explain, ls, -a, -b, -c, -def} returns 6.
 */
int countSmallOpts(int argc, char *argv[])
{
    int count = 0;
    for(int i = 2; i < argc; i++) {
        char *const curArg = argv[i];
        if(curArg[0] == '-' && curArg[1] && curArg[1] != '-' && curArg[1] != '\0') {
            count += strlen(&curArg[1]);
        }
    }
    return count;
}

/**
 * Free a list of strings, along with the strings it contains.
 */
void freeList(char **list)
{
    for(int i = 0; list[i] != NULL; i++) {
        free(list[i]);
        
    }
    free(list);
}
