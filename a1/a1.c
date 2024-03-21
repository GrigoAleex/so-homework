#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <limits.h>

#define MAX_FILE_NAME 255
#define MAX_PATH_SIZE 512
typedef enum
{
    false,
    true
} bool;

void NFSC_List(char **options, int optionsCount);
void NFSC_DisplayFolderContents(char *path, bool reccursive, char permissions[9], int maxiumumSize);
int NFSC_ThrowError(char *message);
int NSFC_PermissionsToOctal(const char permString[9]);
void NFSC_ProcessListOptoins(const char *option, bool *recursive, char **path, char *permissions, int *maximumSize);

int main(int argc, char **argv)
{
    if (argc >= 2)
    {
        if (strcmp(argv[1], "variant") == 0)
        {
            printf("33866\n");
        }
        if (strcmp(argv[1], "list") == 0)
        {
            NFSC_List(argv, argc);
        }
    }
    return 0;
}

void NFSC_List(char **options, int optionsCount)
{
    bool recursive = false;
    char *path = NULL;
    char permissions[9] = "uninit";
    size_t maximumSize = INT_MAX;

    for (size_t i = 2; i < optionsCount; ++i)
    {
        // NFSC_ProcessListOptoins(options[i], &recursive, &path, permissions, &maximumSize);
        if (strcmp(options[i], "recursive") == 0)
            recursive = true;
        else if (strstr(options[i], "path") != NULL)
        {
            path = (char *)realloc(path, strlen(options[i]));
            strcpy(path, options[i] + 5);
        }
        else if (strstr(options[i], "permissions"))
        {
            strcpy(permissions, options[i] + 12);
        }
        else if (strstr(options[i], "size_smaller"))
        {
            maximumSize = atoi(options[i] + 13);
        }
    }

    puts("SUCCESS");
    NFSC_DisplayFolderContents(path, recursive, permissions, maximumSize);

    free(path);
    path = NULL;
}

void NFSC_DisplayFolderContents(char *path, bool recursive, char permissions[9], int maximumSize)
{
    DIR *dir = NULL;
    if ((dir = opendir(path)) == NULL)
    {
        NFSC_ThrowError("invalid directory path");
        exit(-1);
    }

    struct dirent *entry = NULL;
    struct stat statbuf;
    bool filterByPermissons = strcmp(permissions, "uninit") != 0;
    while ((entry = readdir(dir)) != NULL)
    {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        char filePath[MAX_PATH_SIZE];
        snprintf(filePath, MAX_PATH_SIZE, "%s/%s", path, entry->d_name);
        stat(filePath, &statbuf);

        if (filterByPermissons && NSFC_PermissionsToOctal(permissions) != (statbuf.st_mode & 0xFFF))
            continue;

        if (statbuf.st_size > maximumSize)
            continue;

        if (maximumSize == INT_MAX || S_ISREG(statbuf.st_mode))
            printf("%s\n", filePath);
        if (recursive && S_ISDIR(statbuf.st_mode))
            NFSC_DisplayFolderContents(filePath, recursive, permissions, maximumSize);
    }
    closedir(dir);
}

int NFSC_ThrowError(char *string)
{
    printf("ERROR\n%s", string);
    return -1;
}

int NSFC_PermissionsToOctal(const char permString[9])
{
    int perms = 0;

    for (int i = 0; i < 9; i++)
    {
        perms <<= 1;
        if (permString[i] != '-')
            perms |= 1;
    }

    return perms;
}

void NFSC_ProcessListOptoins(const char *option, bool *recursive, char **path, char *permissions, int *maximumSize)
{
    if (strcmp(option, "recursive") == 0)
    {
        *recursive = true;
    }
    else if (strstr(option, "path") == option)
    {
        *path = realloc(*path, strlen(option + 5) + 1);
        strcpy(*path, option + 5);
    }
    else if (strstr(option, "permissions") == option)
    {
        strcpy(permissions, option + 12);
    }
    else if (strstr(option, "size_smaller") == option)
    {
        *maximumSize = atoi(option + 13);
    }
}
