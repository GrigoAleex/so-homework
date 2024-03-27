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
void NFSC_Extract(char **options, int optionsCount);
void NFSC_Parse(char *path);
void NFSC_FindAll(char *path);
void NFSC_DisplayFolderContents(char *path, bool reccursive, char permissions[10], int maxiumumSize, bool checkForSFFormat);
int NFSC_ThrowError(char *message);
int NSFC_PermissionsToOctal(const char permString[9]);
char *NFSC_ReverseString(char *str);
short NFSC_GetHeaderSize(int fd);
int validateMagicWord(int fd, bool printError);
int validateVersion(int fd, bool printError);
int validateSectionNumber(int fd, bool printError);
int validateSectionTypes(int fd, bool printError);

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
        if (strcmp(argv[1], "findall") == 0)
        {
            NFSC_FindAll(argv[2] + 5);
        }
        if (strcmp(argv[1], "extract") == 0)
        {
            NFSC_Extract(argv, argc);
        }
    }

    return 0;
}

void NFSC_FindAll(char *path)
{
    puts("SUCCESS");

    char permissions[10] = {0};
    size_t maximumSize = INT_MAX;
    NFSC_DisplayFolderContents(path, true, permissions, maximumSize, true);
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
    if (fileDescription < 0)
    {
        printf("error!");
    }
    int version, sectionsNumber;

    if (validateMagicWord(fileDescription, true) || (version = validateVersion(fileDescription, true)) == 0 ||
        (sectionsNumber = validateSectionNumber(fileDescription, true)) == 0 || validateSectionTypes(fileDescription, true))
        return;

    puts("SUCCESS");
    printf("version=%d\nnr_sections=%d\n", version, sectionsNumber);
    short headerSize = NFSC_GetHeaderSize(fileDescription);
    lseek(fileDescription, -headerSize + 5, SEEK_END);
    for (size_t i = 0; i < sectionsNumber; i++)
    {
        short type;
        char name[9] = {0};
        int size;

        read(fileDescription, name, 8);
        read(fileDescription, &type, 2);
        read(fileDescription, &size, 4); // acesta este offsetul
        read(fileDescription, &size, 4);
        printf("section%ld: %s %d %d\n", i + 1, name, type, size);
    }
}

int validateMagicWord(int fd, bool printError)
{
    char magicWord[5] = {0};
    lseek(fd, -4, SEEK_END);
    read(fd, magicWord, 4);
    if (strcmp(magicWord, "Wa49") != 0)
    {
        if (printError)
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

int validateVersion(int fd, bool printError)
{
    short headerSize = NFSC_GetHeaderSize(fd);
    int version;
    lseek(fd, -headerSize, SEEK_END);
    read(fd, &version, 4);

    if ((version < 98) || (version > 127))
    {
        if (printError)
            printf("ERROR\nwrong version");
        return 0;
    }
    return version;
}

int validateSectionNumber(int fd, bool printError)
{
    short headerSize = NFSC_GetHeaderSize(fd);
    char sectionsNumber;
    lseek(fd, -headerSize + 4, SEEK_END);
    read(fd, &sectionsNumber, 1);
    if (sectionsNumber != 2 && (sectionsNumber < 4 || sectionsNumber > 17))
    {
        if (printError)
            printf("ERROR\nwrong sect_nr");
        return 0;
    }
    return sectionsNumber;
}

int validateSectionTypes(int fd, bool printError)
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
            if (printError)
                printf("ERROR\nwrong sect_types");
            return 1;
        }
    }

    return 0;
}

void NFSC_Extract(char **options, int optionsCount)
{
    int sectionNumber = 0;
    int wishedLineNumber = 0;
    char *path = NULL;

    for (size_t i = 2; i < optionsCount; ++i)
    {
        if (strstr(options[i], "path") != NULL)
        {
            path = (char *)realloc(path, strlen(options[i]));
            strcpy(path, options[i] + 5);
        }
        else if (strstr(options[i], "section"))
        {
            sectionNumber = atoi(options[i] + 8);
        }
        else if (strstr(options[i], "line"))
        {
            wishedLineNumber = atoi(options[i] + 5);
        }
    }

    puts("SUCCESS");
    int fileDescription = open(path, O_RDWR);
    short headerSize = NFSC_GetHeaderSize(fileDescription);

    int sectionOffset;
    int sectionSize;
    lseek(fileDescription, -headerSize + 15 + (sectionNumber - 1) * 18, SEEK_END);
    read(fileDescription, &sectionOffset, 4);
    read(fileDescription, &sectionSize, 4);

    char section[sectionSize];
    memset(section, 0, sectionSize);
    lseek(fileDescription, sectionOffset, SEEK_SET);
    read(fileDescription, section, sectionSize);

    char newLine[] = "\r\n";
    int lineNumber = 0;
    char *cur = strtok(section, newLine);
    char *result = (char *)calloc(sectionSize, 1);

    while (cur != NULL)
    {
        // printf("%d, %d, %s\n", lineNumber, cur[0], cur);
        // if (cur[0] == '\n')
        // {
        lineNumber++;
        // }
        if (lineNumber == wishedLineNumber)
        {
            result = strcat(result, cur);
        }
        if (lineNumber > wishedLineNumber)
        {
            break;
        }

        cur = strtok(NULL, newLine);
    }

    printf("%s\n", NFSC_ReverseString(result));
    free(result);
    free(path);
    path = NULL;
}

//
// JdY6Kc8eUrCNjXrfxPiU6j7jH6AqpbxsZxsvnWZBJcthDo4w2RnZ7Kmx1n2GfG3jDAnBheiaZWo8j
// j8oWZaiehBnADj3GfG2n1xmK7ZnR2w4oDhtcJBZWnvsxZsxbpqA6Hj7j6UiPxfrXjNCrUe8cK6YdJ
// j8oWZaiehB ADj3GfG2 1xmK7Z R2w4oDhtcJBZW  sxZsxbpqA6Hj7j6Ui xfrXjNCrUe8cK6YdJ
// JdY6Kc8eUrCNjXrfxPiu6j7jH6qpbxszZsvnWZBJcthDo4w2RnZ7Km1n2Gf3jDnBheiaZWo8j
// JdY6Kc8eUrCNjXrfxPiU6j7jH6Aqpbx Zx vnWZBJcthDo4w2RnZ7Kmx1n2GfG3jDAnBheiaZWo8j
// JdY6Kc8e rCNjXrfxPi 6j7jH6AqpbxsZxsvnWZBJcthDo4w2RnZ7Kmx1n2GfG3jDAnBheiaZWo8j
// JdY6Kc8eUrCNjXrfxPiU6jjH6AqpbxsZxsvnWZBJcth Do4w RnZ Kmx1n GfG3jDAnBheiaZWo8j

void NFSC_List(char **options, int optionsCount)
{
    bool recursive = false;
    char *path = NULL;
    char permissions[10] = {0};
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
    NFSC_DisplayFolderContents(path, recursive, permissions, maximumSize, false);

    free(path);
    path = NULL;
}

void NFSC_DisplayFolderContents(char *path, bool recursive, char permissions[10], int maximumSize, bool checkForSFFormat)
{
    DIR *dir = NULL;
    if ((dir = opendir(path)) == NULL)
    {
        NFSC_ThrowError("invalid directory path");
        exit(-1);
    }

    struct dirent *entry = NULL;
    struct stat statbuf;
    char zeros[10] = {0};
    bool filterByPermissons = strcmp(permissions, zeros) != 0;
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
        {
            if (checkForSFFormat && S_ISREG(statbuf.st_mode))
            {
                int fileDescription = open(filePath, O_RDONLY);
                int sectionsNumber;
                if (validateMagicWord(fileDescription, false) || validateVersion(fileDescription, false) == 0 ||
                    (sectionsNumber = validateSectionNumber(fileDescription, false)) < 3 || validateSectionTypes(fileDescription, false))
                    continue;

                short headerSize = NFSC_GetHeaderSize(fileDescription);
                int sectionWithLines = 0;
                for (size_t i = 0; i < sectionsNumber; i++)
                {
                    int sectionOffset;
                    int sectionSize;
                    lseek(fileDescription, -headerSize + 15 + i * 18, SEEK_END);
                    read(fileDescription, &sectionOffset, 4);
                    read(fileDescription, &sectionSize, 4);

                    char section[sectionSize + 1];
                    lseek(fileDescription, sectionOffset, SEEK_SET);
                    read(fileDescription, &section, sectionSize);

                    char newLine[] = {0x0d};
                    int lineNumber = 0;
                    char *cur = strtok(section, newLine);
                    while (cur != NULL)
                    {
                        lineNumber++;
                        cur = strtok(NULL, newLine);
                    }

                    if (lineNumber == 14)
                    {
                        sectionWithLines++;
                        if (sectionWithLines == 3)
                            break;
                    }
                }

                if (sectionWithLines < 3)
                    continue;
            }

            if (checkForSFFormat && S_ISREG(statbuf.st_mode))
                printf("%s\n", filePath);
            else if (!checkForSFFormat)
                printf("%s\n", filePath);
        }

        if (recursive && S_ISDIR(statbuf.st_mode))
            NFSC_DisplayFolderContents(filePath, recursive, permissions, maximumSize, checkForSFFormat);
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
