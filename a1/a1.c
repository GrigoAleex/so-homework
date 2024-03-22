#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
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
void NFSC_Parse(char *path);
void NFSC_DisplayFolderContents(char *path, bool reccursive, char permissions[9], int maxiumumSize);
int NFSC_ThrowError(char *message);
int NSFC_PermissionsToOctal(const char permString[9]);
char *NFSC_ReverseString(char *str);
int validateMagicWord(int fd);
short NFSC_GetHeaderSize(int fd);
int validateVersion(int fd);
int validateSectionNumber(int fd);
int validateSectionTypes(int fd);

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
        if (strcmp(argv[1], "parse") == 0)
        {
            NFSC_Parse(argv[2] + 5);
        }
    }
    return 0;
}

char *NFSC_ReverseString(char *str)
{
    int length = strlen(str);

    for (int i = 0; i < length / 2; i++)
    {
        char temp = str[i];
        str[i] = str[length - i - 1];
        str[length - i - 1] = temp;
    }

    return str;
}

void NFSC_Parse(char *path)
{
    int fileDescription = open(path, O_RDONLY);
    int version, sectionsNumber;

    if (validateMagicWord(fileDescription) || (version = validateVersion(fileDescription)) == 0 ||
        (sectionsNumber = validateSectionNumber(fileDescription)) == 0 || validateSectionTypes(fileDescription))
        return;

    puts("SUCCESS");
    printf("version=%d\nnr_sections=%d\n", version, sectionsNumber);
    short headerSize = NFSC_GetHeaderSize(fileDescription);
    lseek(fileDescription, -headerSize + 5, SEEK_END);
    for (size_t i = 0; i < sectionsNumber; i++)
    {
        short type;
        char name[8];
        int size;

        read(fileDescription, name, 8);
        read(fileDescription, &type, 2);
        read(fileDescription, &size, 4); // acesta este offsetul
        read(fileDescription, &size, 4);
        printf("section%ld: %s %d %d\n", i + 1, name, type, size);
    }
}

int validateMagicWord(int fd)
{
    char magicWord[4];
    lseek(fd, -4, SEEK_END);
    read(fd, magicWord, 4);
    if (strcmp(magicWord, "Wa49") != 0)
    {
        printf("ERROR\nwrong magic");
        return 1;
    }

    return 0;
}

short NFSC_GetHeaderSize(int fd)
{
    short headerSize;

    lseek(fd, -6, SEEK_END);
    read(fd, &headerSize, 2);

    return headerSize;
}

int validateVersion(int fd)
{
    short headerSize = NFSC_GetHeaderSize(fd);
    int version;
    lseek(fd, -headerSize, SEEK_END);
    read(fd, &version, 4);

    if ((version < 98) || (version > 127))
    {
        printf("ERROR\nwrong version");
        return 0;
    }
    return version;
}

int validateSectionNumber(int fd)
{
    short headerSize = NFSC_GetHeaderSize(fd);
    char sectionsNumber;
    lseek(fd, -headerSize + 4, SEEK_END);
    read(fd, &sectionsNumber, 1);
    if (sectionsNumber != 2 && (sectionsNumber < 4 || sectionsNumber > 17))
    {
        printf("ERROR\nwrong sect_nr");
        return 0;
    }
    return sectionsNumber;
}
int validateSectionTypes(int fd)
{
    short headerSize = NFSC_GetHeaderSize(fd);

    char sectionsNumber;
    lseek(fd, -headerSize + 4, SEEK_END);
    read(fd, &sectionsNumber, 1);

    while (sectionsNumber)
    {
        sectionsNumber--;
        short type;
        lseek(fd, -headerSize + 5 + 8 + sectionsNumber * 18, SEEK_END);
        read(fd, &type, 2);
        if (type != 93 && type != 91 && type != 60)
        {
            printf("ERROR\nwrong sect_types");
            return 1;
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
