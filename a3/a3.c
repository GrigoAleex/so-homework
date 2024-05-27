#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define PIPE_WRITE "RESP_PIPE_33866"
#define PIPE_READ "REQ_PIPE_33866"

/**
 * GLOBALS
 */
int _FILE_SIZE = 0;

/**
 * HEADERS
 */
int CreateWritePipe();
void *CreateSharedMemory();
void Ping(int fileDescriptor);
void StartConnection(int writeFileDescriptor);
void ReadCommand(int fileDescription, char *command);
void RunProgram(int writeFileDescriptor, int readFileDescriptor);
void ReadLogical(int readFileDescriptor, int writeFileDescriptor);
void ReadFromOffset(int readFileDescriptor, int writeFileDescriptor);
void MapFileToMemory(int readFileDescriptor, int writeFileDescriptor);
void ReadFileSection(int readFileDescriptor, int writeFileDescriptor);
void WriteToSharedMemory(int readFileDescriptor, int writeFileDescriptor);
void CreateSharedMemoryCommand(int writeFileDescriptor, int readFileDescriptor);

int main(void)
{
    if (CreateWritePipe() == -1)
        return -1;

    int readFileDescriptor = open(PIPE_READ, O_RDONLY);
    int writeFileDescriptor = open(PIPE_WRITE, O_WRONLY);

    if (writeFileDescriptor == -1)
    {
        perror("Could not open {PIPE_WRITE} for writing");
        return 1;
    }

    if (readFileDescriptor == -1)
    {
        perror("Could not open {PIPE_READ} for reading");
        return 1;
    }

    StartConnection(writeFileDescriptor);
    RunProgram(writeFileDescriptor, readFileDescriptor);

    close(writeFileDescriptor);
    close(readFileDescriptor);
    unlink(PIPE_WRITE);

    return 0;
}

/**
 * SOURCES
 */

void Ping(int fileDescriptor)
{
    unsigned int variant = 0x0000844A;
    write(fileDescriptor, "PING#", 5);
    write(fileDescriptor, "PONG#", 5);
    write(fileDescriptor, &variant, 4);
}

void ReadCommand(int fileDescription, char *command)
{
    char c;
    size_t i = 0;
    ssize_t bytes_read;
    while ((bytes_read = read(fileDescription, &c, 1)) > 0)
    {
        if (c == '#')
            break;
        command[i++] = c;
    }
    command[i] = '\0';
}

void *CreateSharedMemory()
{
    int memoryFileDescriptor = shm_open("/aRZRVDT", O_CREAT | O_RDWR, 0664);
    fchmod(memoryFileDescriptor, 0664);
    int truncate = ftruncate(memoryFileDescriptor, 3150704);
    void *shm_ptr = mmap(NULL, 3150704, PROT_READ | PROT_WRITE, MAP_SHARED, memoryFileDescriptor, 0);

    if (memoryFileDescriptor < 0 || truncate == -1 || shm_ptr == MAP_FAILED)
        return MAP_FAILED;
    else
        return shm_ptr;
}

void CreateSharedMemoryCommand(int writeFileDescriptor, int readFileDescriptor)
{
    unsigned int size;
    read(readFileDescriptor, &size, 4);

    if (CreateSharedMemory() == MAP_FAILED)
        write(writeFileDescriptor, "CREATE_SHM#ERROR#", 17);
    else
        write(writeFileDescriptor, "CREATE_SHM#SUCCESS#", 19);
}

void RunProgram(int writeFileDescriptor, int readFileDescriptor)
{
    char request[255];

    for (;;)
    {
        ReadCommand(readFileDescriptor, request);

        if (strcmp(request, "PING") == 0)
            Ping(writeFileDescriptor);
        else if (strcmp(request, "CREATE_SHM") == 0)
            CreateSharedMemoryCommand(writeFileDescriptor, readFileDescriptor);
        else if (strcmp(request, "WRITE_TO_SHM") == 0)
            WriteToSharedMemory(readFileDescriptor, writeFileDescriptor);
        else if (strcmp(request, "MAP_FILE") == 0)
            MapFileToMemory(readFileDescriptor, writeFileDescriptor);
        else if (strcmp(request, "READ_FROM_FILE_OFFSET") == 0)
            ReadFromOffset(readFileDescriptor, writeFileDescriptor);
        else if (strcmp(request, "READ_FROM_FILE_SECTION") == 0)
            ReadFileSection(readFileDescriptor, writeFileDescriptor);
        else if (strcmp(request, "READ_FROM_LOGICAL_SPACE_OFFSET") == 0)
            ReadLogical(readFileDescriptor, writeFileDescriptor);
        else
            break;
    }
}

void ReadFileSection(int readFileDescriptor, int writeFileDescriptor)
{
    void *sharedMemory = CreateSharedMemory();
    unsigned int section, offset, bytesToRead;

    read(readFileDescriptor, &section, 4);
    read(readFileDescriptor, &offset, 4);
    read(readFileDescriptor, &bytesToRead, 4);

    short headerSize;
    memcpy(&headerSize, sharedMemory + _FILE_SIZE - 6, 2);

    short sections;
    memcpy(&sections, sharedMemory + _FILE_SIZE - headerSize + 4, 1);

    if (section > sections)
    {
        write(writeFileDescriptor, "READ_FROM_FILE_SECTION#ERROR#", 29);
        return;
    }

    unsigned int sectionOffset;
    memcpy(&sectionOffset, sharedMemory + _FILE_SIZE - headerSize + 15 + (section - 1) * 18, 4);

    unsigned int sectionSize;
    memcpy(&sectionSize, sharedMemory + _FILE_SIZE - headerSize + 19 + (section - 1) * 18, 4);

    if (offset + bytesToRead >= sectionSize)
    {
        puts("ERROR");
        write(writeFileDescriptor, "READ_FROM_FILE_SECTION#ERROR#", 29);
        return;
    }

    char buffer[bytesToRead];
    memcpy(buffer, sharedMemory + sectionOffset + offset, bytesToRead);
    memcpy(sharedMemory, buffer, bytesToRead);

    write(writeFileDescriptor, "READ_FROM_FILE_SECTION#SUCCESS#", 31);
}

void ReadLogical(int readFileDescriptor, int writeFileDescriptor)
{
    void *sharedMemory = CreateSharedMemory();
    unsigned int logicalOffset, bytesToRead;

    read(readFileDescriptor, &logicalOffset, 4);
    read(readFileDescriptor, &bytesToRead, 4);

    short headerSize;
    memcpy(&headerSize, sharedMemory + _FILE_SIZE - 6, 2);

    short sections;
    memcpy(&sections, sharedMemory + _FILE_SIZE - headerSize + 4, 1);

    short sectionNumber;
    short offset;
    int sectionStartingOffset = 0;
    for (size_t i = 0; i < sections; i++)
    {
        unsigned int sectionSize;
        memcpy(&sectionSize, sharedMemory + _FILE_SIZE - headerSize + 19 + i * 18, 4);

        if (sectionStartingOffset + sectionSize > logicalOffset)
        {
            sectionNumber = i;
            offset = logicalOffset - sectionStartingOffset;
            break;
        }

        sectionStartingOffset += (sectionSize / 5120 + 1) * 5120;
    }

    unsigned int sectionOffset;
    memcpy(&sectionOffset, sharedMemory + _FILE_SIZE - headerSize + 15 + (sectionNumber) * 18, 4);

    char buffer[bytesToRead];
    memcpy(buffer, sharedMemory + sectionOffset + offset, bytesToRead);
    memcpy(sharedMemory, buffer, bytesToRead);

    write(writeFileDescriptor, "READ_FROM_LOGICAL_SPACE_OFFSET#SUCCESS#", 39);
}

void ReadFromOffset(int readFileDescriptor, int writeFileDescriptor)
{
    unsigned int offset, bytesToRead;
    read(readFileDescriptor, &offset, 4);
    read(readFileDescriptor, &bytesToRead, 4);

    if (offset + bytesToRead > _FILE_SIZE)
    {
        write(writeFileDescriptor, "READ_FROM_FILE_OFFSET#ERROR#", 28);
        return;
    }

    void *sharedMemory = CreateSharedMemory();
    char buffer[bytesToRead];

    memcpy(buffer, sharedMemory + offset, bytesToRead);
    memcpy(sharedMemory, buffer, bytesToRead);
    write(writeFileDescriptor, "READ_FROM_FILE_OFFSET#SUCCESS#", 30);
}

void MapFileToMemory(int readFileDescriptor, int writeFileDescriptor)
{
    char fileName[255];
    ReadCommand(readFileDescriptor, fileName);

    int fd = open(fileName, O_RDONLY);
    if (fd == -1)
    {
        write(writeFileDescriptor, "MAP_FILE#ERROR#", 15);
        return;
    }

    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1)
    {
        write(writeFileDescriptor, "MAP_FILE#ERROR#", 15);
        close(fd);
        return;
    }

    _FILE_SIZE = file_stat.st_size;
    void *file_map = mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    void *sharedMemory = CreateSharedMemory();
    memcpy(sharedMemory, file_map, file_stat.st_size);

    write(writeFileDescriptor, "MAP_FILE#SUCCESS#", 17);
}

void WriteToSharedMemory(int readFileDescriptor, int writeFileDescriptor)
{
    unsigned int value, offset;
    read(readFileDescriptor, &offset, 4);
    read(readFileDescriptor, &value, 4);

    if (offset + sizeof(value) >= 3150704)
    {
        write(writeFileDescriptor, "WRITE_TO_SHM#ERROR#", 19);
        return;
    }

    void *shm_ptr = CreateSharedMemory();
    memcpy((char *)shm_ptr + offset, &value, sizeof(value));

    write(writeFileDescriptor, "WRITE_TO_SHM#SUCCESS#", 21);
}

void StartConnection(int writeFileDescriptor)
{
    write(writeFileDescriptor, "BEGIN#", 6);
}

int CreateWritePipe()
{
    if (mkfifo(PIPE_WRITE, 0600) != 0)
    {
        perror("Could not create pipe");
        return -1;
    }

    return 0;
}