#include <arpa/inet.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

char *tar_filename = "temp.tar.gz";

void write_file(int sockfd, int *size)
{
  int n, ret;
  FILE *fp;
  int b_cnt = 0;
  char buffer[BUFFER_SIZE];

  fp = fopen(tar_filename, "wb");
  while ((n = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0)
  {
    b_cnt += n;
    ret = fwrite(buffer, 1, n, fp);
    if (ret == -1)
    {
      perror("Error in writing file. \n");
    }
    if (b_cnt >= *size)
    {
      break;
    }
  }
  fclose(fp);
}

// function to send and receive commands through client socket
void send_and_receive(int client_socket, const char *command, int unzip)
{
  char response[100];
  int size = 0;
  // Send the command to the server
  send(client_socket, command, strlen(command), 0);
  // Receive and print server's response
  memset(response, 0, sizeof(response));
  recv(client_socket, response, 10, 0);

  size = atoi(response);
  // if server has sent a response then write it to client socket
  if (size > 0)
  {
    write_file(client_socket, &size);
    printf("Server response: File transfered successfully.\n");
  }
  else
  {
    memset(response, 0, sizeof(response));
    recv(client_socket, response, 100, 0);
    printf("Server response: %s\n", response);
    return;
  }
  // if there was a command with unzip option then unzip the tar file at client root
  if (unzip)
  {
    char buf[100000];
    char *folder_name = "Unzipped_Folder";
    sprintf(buf, "rm -r %s  > /dev/null 2>&1 ; mkdir %s && tar -xvf %s -C %s > /dev/null 2>&1", folder_name, folder_name, tar_filename, folder_name);
    system(buf);
  }
}

void tokenize_command(char *command, char **tokens, int *num_tokens)
{
  char input[BUFFER_SIZE];
  strcpy(input, command);
  int i = 0;

  while (input[i] != '\n')

  {
    if (input[i] == '\0')
    {
      break;
    }
    ++i;
  }
  // truncate the string at position where newline character was found
  input[i] = '\0';
  // tokenize the string
  char *token = strtok(input, " ");
  *num_tokens = 0;

  //  extract the tokens and count how many tokens are there
  while (token != NULL)
  {
    tokens[*num_tokens] = strdup(token);
    token = strtok(NULL, " ");
    ++(*num_tokens);
  }
}

// Function to connect to server socket
int connect_to_socket(int port)
{
  int server_socket;
  struct sockaddr_in server_addr;
  // Create socket
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1)
  {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  // Set up server address
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port); // Change to your server's port
  server_addr.sin_addr.s_addr =
      inet_addr("172.31.22.224"); // Change to your server's IP

  // Connect to server
  if (connect(server_socket, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) == -1)
  {
    perror("Connection failed");
    exit(EXIT_FAILURE);
  }
  return server_socket;
}

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    printf("Please provide port number\n");
    return 1;
  }
  int client_socket;
  struct sockaddr_in server_addr;
  int server_port = atoi(argv[1]);
  int mirror_port = (argc >= 3) ? atoi(argv[2]) : (server_port + 1);

  char input[BUFFER_SIZE];
  char input_create[BUFFER_SIZE];
  char command[BUFFER_SIZE];
  char *tokens[10];
  int num_tokens = 0;

  char response[11];
  client_socket = connect_to_socket(server_port);
  recv(client_socket, response, 10, 0);
  if (strncmp(response, "mirror_f", 8) == 0)
  {
    close(client_socket);
    client_socket = connect_to_socket(mirror_port);
    printf("Connected to mirror\n");
  }
  else
  {
    printf("Connected to server\n");
  }

  while (1)
  {

    // Get user input for command
    printf("Enter a command: ");
    fgets(input, sizeof(input), stdin);

    // Parse the command and arguments
    tokenize_command(input, tokens, &num_tokens);

    if (num_tokens > 0)
    {
      if (strcmp(tokens[0], "fgets") == 0)
      {
        // Handle fgets command
        if (num_tokens < 2 || num_tokens > 5)
        {
          printf("\nMax 4 arguments are allowed.\n");
        }
        else
        {
          send_and_receive(client_socket, input, 0);
        }
      }
      else if (strcmp(tokens[0], "tarfgetz") == 0)
      {
        // Handle tarfgetz command
        int u_opt = strcmp(tokens[num_tokens - 1], "-u") == 0;
        if (num_tokens != (3 + u_opt))
        {
          printf("\nPlease enter two sizes.\n");
        }
        else if (atoi(tokens[1]) > atoi(tokens[2]))
        {
          printf("\nSize 1 should be less than or equal to Size 2.\n");
        }
        else
        {
          send_and_receive(client_socket, input, u_opt);
        }
      }
      else if (strcmp(tokens[0], "filesrch") == 0)
      {
        // Handle filesrch command
        if (num_tokens != 2)
        {
          printf("\nOnly 1 argument allowed.\n");
        }
        else
        {
          send_and_receive(client_socket, input, 0);
        }
      }
      else if (strcmp(tokens[0], "targzf") == 0)
      {
        // Handle targzf command
        int u_opt = strcmp(tokens[num_tokens - 1], "-u") == 0;
        if (num_tokens < 2 || num_tokens > 5 + u_opt)
        {
          printf("\nMinimum 2 or maximum 5 arguments are allowed.\n");
        }
        else
        {
          char *token = strtok(input, "-u");
          send_and_receive(client_socket, token, u_opt);
        }
      }
      else if (strcmp(tokens[0], "getdirf") == 0)
      {
        // Handle getdirf command
        int u_opt = strcmp(tokens[num_tokens - 1], "-u") == 0;
        if (num_tokens != 3 + u_opt)
        {
          printf("\nPlease enter two dates.\n");
        }
        else
        {
          long int epoch[2];
          int year, month, day;
          struct tm tm;
          for (int i = 0; i < 2; ++i)
          {
            if (strptime(tokens[i + 1], "%Y-%m-%d", &tm) != NULL)
            {
              tm.tm_hour = 0;
              tm.tm_min = 0;
              tm.tm_sec = 0;
              tm.tm_isdst = -1;
              epoch[i] = mktime(&tm);
              // printf("%stime%d:%ld\n", tokens[i + 1], i, epoch[i]);
            }
            else
            {
              printf("Date %d is invalid.\n", i);
              break;
            }
          }
          if (epoch[0] > epoch[1])
          {
            printf("Date1 should be less than Date2\n");
          }
          if (epoch[0] == epoch[1])
          {
            epoch[1] += 71999;
          }
          sprintf(input_create, "%s %ld %ld", tokens[0], epoch[0], epoch[1]);
          send_and_receive(client_socket, input_create, u_opt);
        }
      }
      else if (strcmp(tokens[0], "quit") == 0)
      {
        // Handle quit command
        send(client_socket, tokens[0], strlen(tokens[0]), 0);
        printf("Client process terminated.\n");
        break;
      }
      else
      {
        printf("Invalid command.\n");
      }
    }
    else
    {
      printf("Invalid input.\n");
    }
  }

  // Close the socket
  close(client_socket);

  return 0;
}