# Name: Jeff Hall
# Course: CS37
# Assignment: Program 2 ftclient
# Description: client program that connects to ftserver and receives file transfer
# Last modified: 3/5/19



##Sources:  used sample program from 
# https://docs.python.org/3/library/socket.html as starting file.  
# https://docs.python.org/3/howto/sockets.html#using-a-socket
#  Also used some recommendations from this stack overflow
# https://stackoverflow.com/questions/17667903/python-socket-receive-large-amount-of-data
# File checking based on https://stackabuse.com/python-check-if-a-file-or-directory-exists/

import socket
import sys
import os

dataHOST = ''

class ftclient:
    #Parses the command string and interprets arguments
    def interpretCommand(self):
        if (len(sys.argv) == 5 and sys.argv[3] == "-l"):
            self.commandString = sys.argv[3] + " " + sys.argv[4]  #send string of "-l <port>"
            self.command = "l"
            self.dataPort = int(sys.argv[4])
        elif (len(sys.argv) == 6 and sys.argv[3] == "-g"):
            self.commandString = sys.argv[3] + " " + sys.argv[4] + " " + sys.argv[5]
            self.command = "g"
            self.fileName = sys.argv[4]
            self.dataPort = int(sys.argv[5])
        else:
            print("Incorrect parameter combination")
            quit()
        self.controlHOST = sys.argv[1]
        self.controlPort = int(sys.argv[2])

    #Sends the command to server over control connection    
    def sendCommand(self):
        self.controlSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.controlSocket.connect((self.controlHOST, self.controlPort))
        self.controlSocket.sendall(bytes(self.commandString,"utf-8"))

    #Opens a port for listening for the data connection, server will connect to this
    def openDataConnection(self):
        self.dataSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.dataSocket.bind((dataHOST,self.dataPort))
        self.dataSocket.listen(1)
        self.dataConn, self.dataAddr = self.dataSocket.accept()

    #Receives a fixed length header indicating size of data transmission
    def receiveHeader(self):
        #first 32 characters is header containing size of data to be sent
        header = self.dataConn.recv(32).decode("utf-8")
        print("Header indicates data size: " + header)
        self.dataSize = int(header)

    #When -l command used, receives the file listing over the data connection
    def receiveFileListing(self):
        data = b''
        while(len(data) < self.dataSize):
            packet = self.dataConn.recv(self.dataSize - len(data))
            if not packet:
                print("Error receiving data packet")
                quit()
            data += packet
        print("Receiving file structure from " + str(self.controlHOST) + ":" + str(self.dataPort))
        print(data.decode("utf-8"))

    #after sending 'g' command, server will acknowledge 1=valid file, 0=invalid
    #if 0, server also sends string error message
    def fileTransferAcknowledged(self):
        data = self.controlSocket.recv(1).decode("utf-8") ##listen for acknowledgement on control
        #print("filetransferack: " + data)
        self.fileAck = int(data)
        
        if self.fileAck == 0:
            errormsg = self.controlSocket.recv(32).decode("utf-8")
            print(errormsg)


    # when using -g option, receives and saves the character data
    def receiveFileTransfer(self):
        #create local file to write data stream to
        #if file already exists with name, alter file name
        altFileName = self.fileName
        count = 1
        while(os.path.isfile(altFileName)):
            altFileName = altFileName + str(count) + ".txt"
            count = count+1

        f = open(altFileName, 'w')

        charsReceived=0
        data = b''
        while(charsReceived < self.dataSize):
            packet = self.dataConn.recv(self.dataSize - len(data)) #receive packet
            if not packet:
                print("Error receiving data packet")
                quit()
            charsReceived += len(packet) #keep track of number of characters received
            f.write(packet.decode("utf-8")) #write to file

        print("File transfer completed")
        f.close()



ft = ftclient()
ft.interpretCommand()
ft.sendCommand()

## for file listings, immediately start listening on data conection
if ft.command == 'l':
    ft.openDataConnection()
    ft.receiveHeader()
    ft.receiveFileListing()

# for data transfer, first listen on control for acknowledgement, then listen on data
if ft.command == 'g':
    ft.fileTransferAcknowledged()
    if ft.fileAck == 1:
        ft.openDataConnection()
        ft.receiveHeader()
        ft.receiveFileTransfer()

#s.sendall(b'Hello, world')
#data = s.recv(1024)