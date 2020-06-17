/* 
    -----------------------------------------------------------------
    Program 5: FTP Client
    CSS 432
    Aika Usui
    -----------------------------------------------------------------
    FTPClient.cpp
    Impelments an ftp client program based on the RFC protocol.
 */
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <pwd.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <fstream>
#include <cstring>
#include "Socket.h"

using namespace std;

/* 
    Separate the words everytime it gets to " "
    @param char user input
    @param char the array that commands are stored into
    @return number of tokens
 */
int tokenize(char *line, char **tokens)
{
    // http://www.cplusplus.com/reference/cstring/strtok/
    char *pch;
    pch = strtok(line, " ");
    int num = 0;
    while (pch != NULL)
    {
        //printf("Token: %s\n", pch);
        tokens[num] = pch;
        ++num;
        pch = strtok(NULL, " ");
    }
    return num;
}

/* 
    Separate the words everytime it gets to " ", ",", "()"
    This is used for calcuctating port number for passive mode
    @param char user input
    @param char the array that commands are stored into
    @return number of tokens
 */
int tokenizeP(char *line, char **tokens)
{
    // http://www.cplusplus.com/reference/cstring/strtok/
    char *pch;

    pch = strtok(line, " ,)(");
    int num = 0;
    while (pch != NULL)
    {
        //printf("Token: %s\n", pch);
        tokens[num] = pch;
        ++num;
        pch = strtok(NULL, " ,)(");
    }
    return num;
}

/* 
    Convert string value to char and write it to server
    @param int clientSD value
    @param string command
    @return write is suceeded or not
 */
int runCommand(int clientSD, string command)
{
    int messSize = (command.length() + 2);
    char commandBuffer[messSize];
    //zero out command buffer
    bzero(commandBuffer, sizeof(commandBuffer));
    //Copy the command
    strcpy(commandBuffer, command.c_str());
    //Add carriage return and new line to bfufer
    strcat(commandBuffer, "\r\n");
    return write(clientSD, commandBuffer, messSize);
}

/* 
    Retrive a single line message from server
    @param int clientSD value
    @return message from server
 */
char *singleMessage(int clientSD)
{
    static char buffer[8192];
    bzero(buffer, sizeof(buffer));
    //printf("before read \n");
    int length = read(clientSD, buffer, sizeof(buffer));
    printf("%s", buffer);
    return buffer;
}

/* 
    Retrive multiple lines of messages from server
    @param int clientSD value
    @return message from server
 */
char *getMessage(int clientSD)
{
    struct pollfd ufds;
    // Specifies what socket to receive data from
    ufds.fd = clientSD;
    ufds.events = POLLIN;
    ufds.revents = 0;
    int val = poll(&ufds, 1, 1000); // poll this socket for 1000msec (=1sec)
    static char buf[8192];
    while (val > 0)
    {
        memset(&buf[0], 0, sizeof(buf));
        int nread = read(clientSD, buf, 8192);
        if (nread == 0)
        {
            break;
        }
        printf("%s", buf);
        val = poll(&ufds, 1, 1000);
    }
    return buf;
}

/* 
    Prompt for username and send it to user with USER. Then, gets message from server.
    @param int clientSD value
    @param char hostmame
    @return message from server
 */
char *sendUsername(int clientSD, char *hostname)
{
    char *name = (char *)malloc(1000 * sizeof(char));
    string command;
    char *message[8000];
    bzero(message, sizeof(message));
    char buffer[8000];
    //char *serverIp; // let it point to one of argv[] that includes serverIp.

    string userString(getlogin());
    cout << "Name (" << hostname << ":" << userString << "): ";
    //printf("Name: ");
    cin.getline(name, 1000);
    if (name != NULL)
    {
        command = "USER ";
        command += name;
        if (runCommand(clientSD, command) < 0)
        {
            printf("Unable to send it \n");
            return NULL;
        }
        return getMessage(clientSD);
    }
    return NULL;
}

/* 
    Prompt for password and send it to user with PASS. Then, gets message from server.
    @param int clientSD value
    @return message from server
 */
char *sendPassword(int clientSD)
{
    char *pass = (char *)malloc(1000 * sizeof(char));
    string command;
    char *message[8000];
    bzero(message, sizeof(message));
    char buffer[8000];
    printf("Password: ");
    cin.getline(pass, 1000);
    if (pass != NULL)
    {
        command = "PASS ";
        command += pass;
        if (runCommand(clientSD, command) < 0)
        {
            printf("Unable to send it \n");
            return NULL;
        }
        singleMessage(clientSD);
        return getMessage(clientSD);
    }
    return NULL;
}

/* 
    Open a socket and make a connection to ftp.
    Process based on the message[0].
    @param char server name
    @param int port number
    @return clientSD
 */
int openSocket(char *server, int port)
{
    Socket *serverSocket = new Socket(port);
    int clientSD;
    clientSD = serverSocket->getClientSocket(server);
    char buffer[8000];
    if (clientSD == -1)
    {
        printf("Fail to connect. \n");
        return clientSD;
    }
    else
    {
        printf("Connected to %s.\n", server);
    }
    char *message[8000];
    char originalMessage[8000];
    string command;
    tokenize(getMessage(clientSD), message);
    // if 220
    // ask username,
    if (strcmp(message[0], "220") == 0)
    {
        char *pass = (char *)malloc(1000 * sizeof(char));
        memset(&message[0], 0, sizeof(message));
        memset(&originalMessage[0], 0, sizeof(originalMessage));
        strcpy(originalMessage, sendUsername(clientSD, server));
        tokenize(originalMessage, message);
        // if 331
        // ask password
        if (strcmp(message[0], "331") == 0)
        {
            memset(&message[0], 0, sizeof(message));
            memset(&originalMessage[0], 0, sizeof(originalMessage));
            strcpy(originalMessage, sendPassword(clientSD));
            tokenize(originalMessage, message);
            //if 230
            //SYST
            if (strcmp(message[0], "230"))
            {
                command = "SYST";
                runCommand(clientSD, command);
                getMessage(clientSD);
            }
        }
    }
    return clientSD;
}

/* 
    Open a new socket for data channel based on the port number and server name.
    @param char server name
    @param int port number
    @return passiveSD
 */
int passiveOpen(char *server, int port)
{
    Socket *serverSocket = new Socket(port);
    int passiveSD;
    passiveSD = serverSocket->getClientSocket(server);
    if (passiveSD == -1)
    {
        printf("Fail to connect. \n");
        return passiveSD;
    }
    return passiveSD;
}

/* 
    Process ls command. 
    Fork a process and in the child process, gets the data. in the parent process,
    send "LIST" command and gets message from server.
    @param int parent = cliendSD
    @param int childSD = passiveSD
 */
void ls(int parentSD, int childSD)
{
    pid_t childStatus, pid = fork();
    string command;
    if (pid < 0)
    {
        printf("Fork failed!\n");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)  // fork success and child process
    {
        singleMessage(childSD);
        getMessage(childSD);
        close(childSD);
        exit(EXIT_SUCCESS);
    }
    else // parent process
    {
        command = "LIST";
        if (runCommand(parentSD, command) < 0)
        {
            printf("Unable to send it \n");
            return;
        }
        singleMessage(parentSD);
        wait(&childStatus);
        printf("\n");
        singleMessage(parentSD);
    }
}

/* 
    Retrive a single line message from server and hold it in the char value.
    @param int clientSD value
    @return char value that holds message from server
 */
char *singleMessageF(int clientSD)
{
    static char buffer[8192];
    bzero(buffer, sizeof(buffer));
    //printf("before read \n");
    int length = read(clientSD, buffer, sizeof(buffer));
    //printf("%s", buffer);
    return buffer;
}

/* 
    Retrive multiple lines of message from server and hold it in the char value.
    @param int clientSD value
    @return char value that holds message from server
 */
char *getMessageF(int clientSD)
{
    struct pollfd ufds;
    // Specifies what socket to receive data from
    ufds.fd = clientSD;
    ufds.events = POLLIN;
    ufds.revents = 0;
    int val = poll(&ufds, 1, 1000); // poll this socket for 1000msec (=1sec)
    static char buf[8192];
    while (val > 0)
    {
        memset(&buf[0], 0, sizeof(buf));
        int nread = read(clientSD, buf, 8192);
        if (nread == 0)
        {
            break;
        }
        //printf("%s", buf);
        val = poll(&ufds, 1, 1000);
    }
    return buf;
}

/* 
    Process get command.
    Get a file from the current remote directory.
    @param int clientSD value
    @param int childSD value
    @param string file name
 */
void getFile(int parentSD, int childSD, string file)
{
    string command;
    fstream f;
    char* fileContent[8000];

    // Open Binary mode
    command = "TYPE I";
    if (runCommand(parentSD, command) < 0)
    {
        printf("Unable to send it \n");
        return;
    }

    char buffer[70]; //allocate buffer
    char *message[8000];
    bzero(buffer, 70);
    strcpy(buffer, getMessage(parentSD));
    int size = tokenize(buffer, message);
    //cout << file << endl;
    if (strcmp(message[0], "200") == 0)
    {
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        int fileFD = open(file.c_str(), O_WRONLY | O_CREAT, mode);
        command = "RETR ";
        command += file;
        if (runCommand(parentSD, command) < 0)
        {
            printf("Unable to send it \n");
            return;
        }
        // send the content of file
        f.open(file);
        string line;
        string fileBuf;
        fileBuf += singleMessageF(childSD);
        fileBuf += getMessageF(childSD);
        // write into the file
        f.write(fileBuf.c_str(), sizeof(fileBuf.c_str()));

        strcpy(buffer, singleMessage(parentSD));
        int size = tokenize(buffer, message);
        if (strcmp(message[0], "150") == 0)
        {
            singleMessage(parentSD);
        }
    }
    close(childSD);
}

/* 
    Process put command.
    Store a file into the current remote directory
    @param int clientSD value
    @param int childSD value
    @param string command
    @param string local file name
 */
void putFile(int parentSD, int childSD, string command, string localfile)
{
    string comm;

    //prepare the files
    int fileFD = open(localfile.c_str(), O_RDONLY);
    if (fileFD < 0)
    {
        printf("local file is not found \n");
        return;
    }
    fstream file;
    file.open(localfile);
    string line;

    // Open Binary mode
    comm = "TYPE I";
    if (runCommand(parentSD, comm) < 0)
    {
        printf("Unable to send it \n");
        return;
    }

    char buffer[70]; //allocate buffer
    char *message[8000];
    bzero(buffer, 70);
    strcpy(buffer, getMessage(parentSD));
    int size = tokenize(buffer, message);
    if (strcmp(message[0], "200") == 0)
    {
        if (runCommand(parentSD, command) < 0)
        {
            printf("Unable to send it \n");
            return;
        }
        // send the content of file
        while (getline(file, line))
        {
            write(childSD, line.c_str(), sizeof(line.c_str()));
            //cout << line << endl;
        }
        file.close();
        strcpy(buffer, singleMessage(parentSD));
        int size = tokenize(buffer, message);
        if (strcmp(message[0], "150") == 0)
        {
            close(childSD);
            close(fileFD);
            fileFD = -1;
            singleMessage(parentSD);
        }
    }
    close(childSD);
}

int main(int argc, char **argv)
{
    char *input = (char *)malloc(1000 * sizeof(char));
    char *hostname = NULL;
    int port = 21;
    int clientSD;
    char *message[8000];
    char originalMessage[8000];
    char *args[1000];
    string command;
    printf("opening socket \n");

    hostname = argv[1];
    // open socket port 21 and connect to ftp
    clientSD = openSocket(hostname, port);

    bool run = true;
    while (run)
    {
        printf("ftp> ");
        cin.getline(input, 1000);
        int num_of_tokens = tokenize(input, args);
        args[num_of_tokens] = NULL;
        if (strcmp(args[0], "cd") == 0)
        {
            if (strcmp(args[1], "NULL") == 0)
            {
                printf("Enter subdir value \n");
                break;
            }
            command = "CWD ";
            command += args[1];
            if (runCommand(clientSD, command) < 0)
            {
                printf("Unable to send it \n");
                break;
            }
            getMessage(clientSD);
        }
        else if (strcmp(args[0], "ls") == 0)
        {
            command = "PASV";
            if (runCommand(clientSD, command) < 0)
            {
                printf("Unable to send it \n");
                break;
            }
            memset(message, 0, sizeof(message));
            char buffer[70]; //allocate buffer
            bzero(buffer, 70);
            strcpy(buffer, getMessage(clientSD));
            int size = tokenizeP(buffer, message);
            int passiveSD;
            if (strcmp(message[0], "227") == 0)
            {
                //227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).
                //calculate port num use the formula: (first value(p1) x 256) + second value(p2).
                int p1 = atoi(message[8]);
                int p2 = atoi(message[9]);
                int pPort = (atoi(message[8]) * 256) + atoi(message[9]);
                // open a socket
                passiveSD = passiveOpen(hostname, pPort);
                // create chile process and execute
                ls(clientSD, passiveSD);
            }
            memset(&message[0], 0, sizeof(message));
            memset(&originalMessage[0], 0, sizeof(originalMessage));
        }
        else if (strcmp(args[0], "get") == 0)
        {
            if (strcmp(args[1], "NULL") == 0)
            {
                printf("Enter filename \n");
                break;
            }
            command = "PASV";
            if (runCommand(clientSD, command) < 0)
            {
                printf("Unable to send it \n");
                break;
            }
            memset(message, 0, sizeof(message));
            char buffer[70]; //allocate buffer
            bzero(buffer, 70);
            strcpy(buffer, getMessage(clientSD));
            int size = tokenizeP(buffer, message);
            int passiveSD;
            if (strcmp(message[0], "227") == 0)
            {
                //227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).
                //calculate port num use the formula: (first value(p1) x 256) + second value(p2).
                int p1 = atoi(message[8]);
                int p2 = atoi(message[9]);
                int pPort = (atoi(message[8]) * 256) + atoi(message[9]);
                //open with the port
                passiveSD = passiveOpen(hostname, pPort);
                string file = args[1];
                // execute command
                getFile(clientSD, passiveSD, file);
            }
            memset(&message[0], 0, sizeof(message));
            memset(&originalMessage[0], 0, sizeof(originalMessage));
        }
        else if (strcmp(args[0], "put") == 0)
        {
            string localfile;
            string remotefile;
            printf("(local-file) ");
            cin.getline(input, 1000);
            num_of_tokens = tokenize(input, args);
            localfile = args[0];
            printf("(remote-file) ");
            cin.getline(input, 1000);
            num_of_tokens = tokenize(input, args);
            remotefile = args[0];

            command = "PASV";
            if (runCommand(clientSD, command) < 0)
            {
                printf("Unable to send it \n");
                break;
            }
            memset(message, 0, sizeof(message));
            char buffer[70]; //allocate buffer
            bzero(buffer, 70);
            strcpy(buffer, getMessage(clientSD));
            int size = tokenizeP(buffer, message);
            int passiveSD;

            if (strcmp(message[0], "227") == 0)
            {
                printf("passive start \n");
                //227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).
                //calculate port num use the formula: (first value(p1) x 256) + second value(p2).
                int p1 = atoi(message[8]);
                int p2 = atoi(message[9]);
                int pPort = (atoi(message[8]) * 256) + atoi(message[9]);
                printf("%d\n", pPort);
                //open with the port
                passiveSD = passiveOpen(hostname, pPort);
                printf("passive open done \n");
                command = "STOR ";
                command += remotefile;
                // exesute command
                putFile(clientSD, passiveSD, command, localfile);
            }
            memset(&message[0], 0, sizeof(message));
            memset(&originalMessage[0], 0, sizeof(originalMessage));
        }
        else if (strcmp(args[0], "close") == 0) //Close the connection but not quit ftp.
        {
            command = "QUIT";
            if (runCommand(clientSD, command) < 0)
            {
                printf("Unable to send it \n");
                break;
            }
            strcpy(originalMessage, singleMessage(clientSD));
            tokenize(originalMessage, message);
        }
        else if (strcmp(args[0], "quit") == 0) //Close the connection and quit ftp.
        {
            command = "QUIT";
            if (runCommand(clientSD, command) < 0)
            {
                printf("Unable to send it \n");
                break;
            }
            run = false;
            singleMessage(clientSD);
            close(clientSD);
            exit(1);
        }
    }
    return 0;
}