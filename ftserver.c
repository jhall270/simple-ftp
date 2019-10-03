// Name: Jeff Hall
// Course: CS372
// Assignment: Program 2 ftserver
// Description: accepts connections from client and sends files
// Last modified: 3/5/19
/*   Source Notes:  
*   CS344 server.c used as initial starting code and was modified to meet requirements
*   directory listing based on https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.3.0/com.ibm.zos.v2r3.bpxbd00/rtread.htm
*   filesize based on https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h> 





struct controlCommand{
    char command;
    char filename[64];
    int port;
}
;

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

//function configures server control socket, opens socket and begins listening
int controlSetup(struct sockaddr_in* address, int portNumber){
    int socketFD;

	address->sin_family = AF_INET; // Create a network-capable socket
	address->sin_port = htons(portNumber); // Store the port number
	address->sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(socketFD, (struct sockaddr *)address, sizeof(*address)) < 0) // Connect socket to port
		error("ERROR on binding");
	listen(socketFD, 1); // Flip the socket on - it can now receive up to 5 connections

    return socketFD;
}

//gets command from client, stores in struct
//returns 1 if received a correct command, 0 if bad
int getClientCommand(struct controlCommand * curr, int connectionFD){
    // Get the message from the client and display it
    char buffer[512];
    char temp[64];
	int i,j, charsRead;
    
    memset(buffer, '\0', sizeof(buffer));
    memset(temp, '\0', sizeof(temp));
    memset(curr->filename, '\0',sizeof(curr->filename));

	charsRead = recv(connectionFD, buffer, 512, 0); // Read the client's message from the socket
	if (charsRead < 0) error("ERROR reading from socket");
	printf("RAW COMMAND INPUT: %s \n", buffer);

    //try to tokenize command and args

    i=0;
    j=0;
    //skip any leading whitespace
    while(buffer[i] == ' '){
        i++;
    }

    while(buffer[i] != ' ' && buffer[i] != '\0'){
        temp[j]=buffer[i];
        i++;
        j++;
    }

    //-l is valid command
    if(strcmp(temp,"-l") == 0){
        curr->command = 'l';        
    }
    //-g is valid commmand, next token should be filename
    else if(strcmp(temp,"-g") == 0){
        curr->command = 'g';
        //get filename from buffer
        memset(temp, '\0', sizeof(temp));
        j=0;

        while(buffer[i] == ' '){
            i++;
        }

        //copy next token into temp, should be filename
        while(buffer[i] != ' ' && buffer[i] != '\0'){
            temp[j]=buffer[i];
            i++;
            j++;
        }

        if(strlen(temp) == 0){
            printf("Error no filename detected with -g command\n");
            return 0;
        }

        //copy filename to struct
        strcpy(curr->filename, temp);

    }
    else{
        //invalid command received
        printf("Command %s not recognized\n", temp);
        return 0;
    }

    memset(temp, '\0', sizeof(temp));
    j=0;
    //last token for -l or -g should be port
    while(buffer[i] == ' '){
        i++;
    }

    //copy next token into temp, should be port
    while(buffer[i] != ' ' && buffer[i] != '\0'){
        temp[j]=buffer[i];
        i++;
        j++;
    }

    if(strlen(temp) == 0){
        printf("Error no port detected with -l or -g command\n");
        return 0;
    }

    //copy port to struct
    curr->port = atoi(temp);


}

//reads current directory listing, writes to string buffer
//Reference note: based on some material from this link
//https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.3.0/com.ibm.zos.v2r3.bpxbd00/rtread.htm
void createLSString(char* buffer){
    memset(buffer, '\0', sizeof(buffer));

    DIR *dir;
    struct dirent *entry;
    int i=0;

    if ((dir = opendir("./")) == NULL)
        perror("opendir() error");
    else {
        while ((entry = readdir(dir)) != NULL){
            i+=sprintf(buffer + i, "  %s\n", entry->d_name);
        }
            
        closedir(dir);
    }
}

//sends an initial fixed size 32 character header which includes size of data transmission
void sendDataHeader(int socketFD, int size){
    char header[32];
    int i=0;
    int charsWritten;

    i = sprintf(header, "%i", size);

    //pad remaining header with blanks
    while(i<32){
        header[i]=' ';
        i++;
    }

    //printf("HEADER: %s\n", header);

    charsWritten = send(socketFD, header, 32, 0);

    if(charsWritten < 32){
        printf("Error sending data header\n");
        exit(1);
    }

}

//sends file listing 
void sendStringData(int socketFD, char* data, int size){
    int charsWritten;

    charsWritten = send(socketFD, data, size, 0);

    if(charsWritten < size){
        printf("Error sending file listing\n");
        exit(1);        
    }
}


//gets size of file determined by filename in struct
//return -1 if file not found
int getFileSize(struct controlCommand* curr){
    int status, size;

    struct stat st;
    status = stat(curr->filename, &st);
    
    //check if file not found
    if(status == -1){
        return -1;
    }
    
    //file found return size
    size = st.st_size;
    printf("Found file, size is %d\n", size);

    return size;

}

//sends character data from file over socket to client
sendFileData(struct controlCommand* curr, int socketFD, int fileSize){
    int charsWritten, charsRead, packetSize;
    int fileFD;
    char buffer[1024];
    int l;

    fileFD = open(curr->filename, O_RDONLY);
    if(fileFD < 0 ){
        perror("Error opening file");
        exit(1);
    }

    l=0;  //left index of read window
    while(l < fileSize){
        //packet is minimum of 1024 or remaining characters
        packetSize = 1024;
        if((fileSize - l) < packetSize){
            packetSize = fileSize - l;
        }

        //read packet into buffer from file
        memset(buffer, '\0', sizeof(buffer));
        charsRead = read(fileFD, buffer, packetSize);
        l+=charsRead;

        //transmit packet
        charsWritten = send(socketFD, buffer, packetSize, 0);
        if(charsWritten < packetSize){
            printf("Error sending packet\n");
            exit(1);        
        }
    }

}

//FOR TESTING
void printCommand(struct controlCommand* curr){
    printf("command: %c\nfilename: %s\nport: %d\n", curr->command, curr->filename, curr->port);
}



int main(int argc, char *argv[])
{
	int listenSocketFD, controlFD, dataSocketFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char controlBuffer[512];
	struct sockaddr_in serverControlAddress, clientControlAddress, clientDataAddress; //serverDataAddress,
    struct controlCommand clientCommand;
    int isValid;
    int peerName, len;
    char dataBuffer[2048];
    int fileSize;

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// intializing variables for control connection
	memset((char *)&serverControlAddress, '\0', sizeof(serverControlAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string

    //call function to set up control connection, start listening
    listenSocketFD = controlSetup(&serverControlAddress, portNumber);
    printf("Server open on %d\n",portNumber);

    //main server loop, continues listening for new requests
    while(1){
        // Accept a connection for control socket, blocking if one is not available until one connects
        sizeOfClientInfo = sizeof(clientControlAddress); // Get the size of the address for the client that will connect
        controlFD = accept(listenSocketFD, (struct sockaddr *)&clientControlAddress, &sizeOfClientInfo); // Accept
        if (controlFD < 0) error("ERROR on accept");

        len = sizeof(peerName);
        peerName = getpeername(controlFD,(struct sockaddr*)&clientControlAddress, &len);
        printf("Connection from %d\n", peerName);

        //gets command string from client, parses and saves to struct
        isValid = getClientCommand(&clientCommand, controlFD);

        

        if(isValid){
            //set up data connection
            portNumber = clientCommand.port; //port identified in client's request
            
            memset((char*)&clientDataAddress, '\0', sizeof(clientDataAddress));

            clientDataAddress.sin_family = AF_INET; // Create a network-capable socket
            clientDataAddress.sin_port = htons(portNumber); // Store the port number
            clientDataAddress.sin_addr.s_addr = clientControlAddress.sin_addr.s_addr;

            dataSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
            if (dataSocketFD < 0) {
                printf("ERROR opening socket");
                exit(1);
            }
            

            
            if(clientCommand.command == 'l'){
                //sleep to allow time for client to open connection
                printf("Trying to connect data connection\n");
                sleep(2);

                // Connect to address
                if (connect(dataSocketFD, (struct sockaddr*)&clientDataAddress, sizeof(clientDataAddress)) < 0){
                    perror("ERROR connecting data connection to client");
                    exit(1);
                }

                createLSString(dataBuffer);
                //printf("data listing string: %s\n", dataBuffer);

                //send header specifying data size
                sendDataHeader(dataSocketFD, strlen(dataBuffer));
                //send data itself
                sendStringData(dataSocketFD, dataBuffer, strlen(dataBuffer));

            }
            else if(clientCommand.command == 'g'){
                //try to find file requested
                fileSize = getFileSize(&clientCommand);

                //if cannot find file, send error message
                if(fileSize < 0 ){
                    memset(controlBuffer, '\0', sizeof(controlBuffer));
                    strcpy(controlBuffer, "0 ERROR FILE NOT FOUND");
                    sendStringData(controlFD, controlBuffer, strlen(controlBuffer));
                }
                else{
                    //valid file is requested
                    //first send positive acknowledgment over control connection
                    memset(controlBuffer, '\0', sizeof(controlBuffer));
                    strcpy(controlBuffer, "1");
                    sendStringData(controlFD, controlBuffer, strlen(controlBuffer));


                    //sleep to allow time for client to open data connection
                    printf("Trying to connect data connection\n");
                    sleep(2);

                    // Connect to data connection address
                    if (connect(dataSocketFD, (struct sockaddr*)&clientDataAddress, sizeof(clientDataAddress)) < 0){
                        perror("ERROR connecting data connection to client");
                        exit(1);
                    }

                    //send data over data connection
                    //send header specifying data size
                    sendDataHeader(dataSocketFD, fileSize);
                    sendFileData(&clientCommand, dataSocketFD, fileSize);

                }
            }
            else{
                printf("Error -- command not recognized\n");
            }

        }
        else{
            //Invalid command
            printf("Invalid command received\n");

        }


        close(controlFD); // Close the existing socket which is connected to the client
        close(dataSocketFD);

    }


	close(listenSocketFD); // Close the listening socket
	return 0; 
}
