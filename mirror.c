#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <zlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
// maximum 100 clients can connect with mirror
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define CHUNK_SIZE 16384

const char *basepath = "/home";

// Function to add a file to tar file
void AddtoTar(char *tar, char *filePath)
{
  char buf[200];
  sprintf(buf, "tar -uvf %s %s", tar, filePath);
  system(buf);
}

// Function to send file to client through socket
void send_file(int client_socket, const char *filename)
{
  FILE *fp;
  int ret, len;
  char buf[BUFFER_SIZE] = {0};
  fp = fopen(filename, "rb");
  while (len = fread(buf, 1, 1024, fp))
  {
    ret = send(client_socket, buf, len, 0);
    if (ret == -1)
    {
      perror("Error sending data : Client");
    }
  }
  fclose(fp);
}

// Function to loop through all the files at root and add files to tar file if its fgets command or targzf command
void listFilesRecursively(const char *basePath, const char **tokens, int len,
                          char *tar_fd, int *found, int ext)
{
  struct dirent *entry;
  struct stat statbuf;
  // open root directory
  DIR *dir = opendir(basePath);
  if (dir == NULL)
  {
    perror("Error in opening directory.\n");
    return;
  }

  // loop through every files and directories
  while ((entry = readdir(dir)) != NULL)
  {
    char path[1024];
    // store the file path in path variable
    snprintf(path, sizeof(path), "%s/%s", basePath, entry->d_name);
    // get the details of the item
    if (stat(path, &statbuf) == -1)
    {
      perror("stat");
      continue;
    }
    // checks if it is a directory and will recursively calls itself until it finds a file
    if (S_ISDIR(statbuf.st_mode))
    {
      if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
      {
        listFilesRecursively(path, tokens, len, tar_fd, found, ext);
      }
    }
    else
    {
      // will loop through the tokens to compare file details
      for (int i = 1; i <= len; i++)
      {
        int cmp = 0;
        // if its targzf command, we pass flag 1
        if (ext)
        {
          // extract extension of the file
          char *ext_name = strrchr(entry->d_name, '.');
          if (ext_name != NULL)
          {
            // compares the extension with token passed and will store the answer in cmp variable
            cmp = (strcmp(ext_name + 1, tokens[i]) == 0);
          }
        }
        else
        {
          // if its fgets command, we pass flag 0, and compare the file name and store the answer in cmp variable
          cmp = (strcmp(entry->d_name, tokens[i]) == 0);
        }
        // if we found a matching file, we will add it to the tar file and set the found flag to 1, which we will use later to send the generated tar file
        if (cmp == 1)
        {
          *found = 1;
          AddtoTar(tar_fd, path);
          // strcat(tar_fd, " ");
          // strcat(tar_fd, path);
        }
      }
    }
  }
  closedir(dir);
}

// Function to loop through all the files at root and add files to tar file if its tarfgetz command or getdirf command
void listFilesRecursively_size(const char *basePath, long int *min,
                               long int *max, char *tar_fd, int *found,
                               int size_cmp)
{
  struct dirent *entry;
  struct stat statbuf;
  // open root directory
  DIR *dir = opendir(basePath);
  if (dir == NULL)
  {
    perror("opendir");
    return;
  }
  // loop through every files and directories
  while ((entry = readdir(dir)) != NULL)
  {
    char path[1024];
    // store the file path in path variable
    snprintf(path, sizeof(path), "%s/%s", basePath, entry->d_name);
    // get the details of the item
    if (stat(path, &statbuf) == -1)
    {
      perror("stat");
      continue;
    }
    // checks if it is a directory and will recursively calls itself until it finds a file
    if (S_ISDIR(statbuf.st_mode))
    {
      if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
      {
        listFilesRecursively_size(path, min, max, tar_fd, found, size_cmp);
      }
    }
    else
    {
      // checks if its tarfgetz command
      if (size_cmp)
      {
        // will check if the file size is between the passed size
        if (statbuf.st_size >= *min && statbuf.st_size <= *max)
        {
          *found = 1;
          AddtoTar(tar_fd, path);
        }
      }
      // checks if its getdirf command
      else
      {
        // will check if the file was created between the passed dates in token
        if (statbuf.st_ctime >= *min && statbuf.st_ctime <= *max)
        {
          *found = 1;
          AddtoTar(tar_fd, path);
        }
      }
    }
  }

  closedir(dir);
}

// Function to loop through all the files at root and send details of the matching file's first occurence if its filesrch command
void search_and_send_details(const char *basePath, const char **tokens, int len,
                             int client_socket, char *buf, int *found)
{
  if (*found)
  {
    return;
  }
  struct dirent *entry;
  struct stat statbuf;
  // open root directory
  DIR *dir = opendir(basePath);
  if (dir == NULL)
  {
    perror("opendir");
    return;
  }

  // loop through every files and directories
  while ((entry = readdir(dir)) != NULL)
  {
    char path[1024];
    // store the file path in path variable
    snprintf(path, sizeof(path), "%s/%s", basePath, entry->d_name);
    // get the details of the item
    if (stat(path, &statbuf) == -1)
    {
      perror("stat");
      continue;
    }
    // checks if it is a directory and will recursively calls itself until it finds a file
    if (S_ISDIR(statbuf.st_mode))
    {
      if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
      {
        search_and_send_details(path, tokens, len, client_socket, buf, found);
      }
    }
    else
    {
      // when it gets a file, it will compare its name and send its details in response to client
      if (strcmp(entry->d_name, tokens[1]) == 0)
      {
        *found = 1;
        snprintf(buf, 100, "Filename: %s Size: %ld bytes Date: %s",
                 entry->d_name, (long)statbuf.st_size,
                 ctime(&statbuf.st_ctime));
        break;
      }
    }
  }

  closedir(dir);
}

// Function to create an empty tar file at the beginning
void createEmptyTar(char *fileName)
{
  char buf[100];
  sprintf(buf, "tar -cf %s -T /dev/null", fileName);
  system(buf);
}

// Function to check which command is passed by the clients and call the respective action command
void search_and_send_files(const char **tokens, int len, int client_socket,
                           int what_)
{
  char response[BUFFER_SIZE];
  // flag to check if the command was successful or not
  int found = 0;
  // create a buffer to store response
  char buff[1000];

  const char *zipFilename = "temp.tar.gz";
  // remove the existing same named tar file
  remove(zipFilename);
  // create a new empty tar file
  createEmptyTar(zipFilename);

  switch (what_)
  {
  case 1:
    // case 1 is for fgets command
    listFilesRecursively(basepath, tokens, len, zipFilename, &found, 0);
    break;
  case 2:
    if (len >= 2)
    {
      // case 2 is for tarfgetz command
      long int s1 = atoi(tokens[1]);
      long int s2 = atoi(tokens[2]);
      listFilesRecursively_size(basepath, &s1, &s2, zipFilename, &found, 1);
    }
    break;
  case 3:
    // case 3 is for filesrch command
    memset(buff, 0, sizeof(buff));
    search_and_send_details(basepath, tokens, len, client_socket, buff, &found);
    // in this case we dont need to send any files so we can send the response string to client right here
    if (found)
    {
      response[0] = '0';
      response[1] = '\0';
      send(client_socket, response, 10, 0);
      send(client_socket, buff, 100, 0);
      return;
    }
    break;
  case 4:
    // case 4 is for targzf command
    listFilesRecursively(basepath, tokens, len, zipFilename, &found, 1);
    break;
  case 5:
    // case 5 is for getdirf command
    if (len > 1)
    {
      long int s1 = atoi(tokens[1]);
      long int s2 = atoi(tokens[2]);
      listFilesRecursively_size(basepath, &s1, &s2, zipFilename, &found, 0);
    }
    break;
  default:
    break;
  }
  // if the result of any of the above command is found then send the file to client socket
  if (found)
  {

    FILE *fp = fopen(zipFilename, "r");

    if (fp == NULL)
    {
      return -1;
    }

    fseek(fp, 0L, SEEK_END);
    long int res = ftell(fp);
    // firstly send the response message
    snprintf(response, 10, "%ld", res);
    send(client_socket, response, 10, 0);
    // then send the response zip file
    send_file(client_socket, zipFilename);
  }
  else
  {
    // if nothing was found then send appropriate response message
    response[0] = '0';
    response[1] = '\0';
    send(client_socket, response, 10, 0);
    sprintf(response, "File Not found");
    send(client_socket, response, 100, 0);
  }
}

// Function to tokenize the received command
void tokenize_command(char *command, char **tokens, int *num_tokens)
{
  int i = 0;
  // loop till the end of command
  while (command[i] != '\n')
  {
    if (command[i] == "\0")
    {
      break;
    }
    ++i;
  }
  // truncate the string at position where newline character was found
  command[i] = '\0';
  // tokenize the string
  char *token = strtok(command, " ");
  *num_tokens = 0;

  //  extract the tokens and count how many tokens are there
  while (token != NULL)
  {
    tokens[*num_tokens] = strdup(token);
    token = strtok(NULL, " ");
    ++(*num_tokens);
  }
}

// find out which command is requested by client
void process_command(const char *command, int client_socket)
{
  // create a default server response buffer
  char response[BUFFER_SIZE] = "Server response: ";
  char *tokens[10];
  int num_tokens = 0;
  // tokenize the command
  tokenize_command(command, tokens, &num_tokens);

  // if user has passed a command, then segregate the commands and call respective switch case
  if (num_tokens > 0)
  {

    if (strcmp(tokens[0], "fgets") == 0)
    {
      search_and_send_files(tokens, num_tokens - 1, client_socket, 1);
    }
    else if (strcmp(tokens[0], "tarfgetz") == 0)
    {
      search_and_send_files(tokens, num_tokens - 1, client_socket, 2);
    }
    else if (strcmp(tokens[0], "filesrch") == 0)
    {
      search_and_send_files(tokens, num_tokens - 1, client_socket, 3);
    }
    else if (strcmp(tokens[0], "targzf") == 0)
    {
      search_and_send_files(tokens, num_tokens - 1, client_socket, 4);
    }
    else if (strcmp(tokens[0], "getdirf") == 0)
    {
      search_and_send_files(tokens, num_tokens - 1, client_socket, 5);
    }
    else
    {
      snprintf(response, sizeof(response), "Invalid command");
    }
  }
  else
  {
    snprintf(response, sizeof(response), "No command received");
  }
}

// function to process the commands received from client socket
void processclient(int client_socket)
{
  char buffer[BUFFER_SIZE];
  ssize_t bytes_received;

  while (1)
  {
    // Receive command from client
    bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    // check if user has passed anything in the socket
    if (bytes_received <= 0)
    {
      break;
    }
    buffer[bytes_received] = '\0';
    process_command(buffer, client_socket);
  }

  close(client_socket);
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("Please provide port number\n");
    return 1;
  }
  int server_socket, client_socket;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);

  key_t key = ftok("shared_memory_key", 65);
  int shmid = shmget(key, 4, IPC_CREAT | 0666);

  if (shmid == -1)
  {
    perror("shmget");
    return 1;
  }

  //  Create socket
  int option = 1;
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option));
  if (server_socket == -1)
  {
    perror("Socket creation error");
    exit(EXIT_FAILURE);
  }

  // Initialize server_addr structure
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(atoi(argv[1])); // Change port if needed

  // Bind socket to address
  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) == -1)
  {
    perror("Bind error");
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  // Listen for client connections
  if (listen(server_socket, MAX_CLIENTS) == -1)
  {
    perror("Listen error");
    close(server_socket);
    exit(EXIT_FAILURE);
  }

  printf("Mirror waiting for connections on port : %d\n", atoi(argv[1]));

  // Accept and handle client connections
  while (1)
  {
    client_socket =
        accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
    if (client_socket == -1)
    {
      perror("Server:Accept error");
      continue;
    }

    printf("Client connected\n");

    pid_t child_pid = fork();
    if (child_pid == -1)
    {
      perror("Fork error");
      close(client_socket);
      continue;
    }
    else if (child_pid == 0)
    {
      close(server_socket); // Close parent socket
      processclient(client_socket);
      printf("Mirror: Client disconnected\n");
      close(client_socket);
      exit(EXIT_SUCCESS);
    }
    else
    {
      close(client_socket); // Close client socket
    }
  }

  return 0;
}