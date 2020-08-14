import socket
import select
import time
import sys
import logging
import os

# setting up the logging module, configuring the logging 

logging.basicConfig(filename= '/VULNWEB/HTTP_PROXY.log' , format='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p' , filemode='a')
logger=logging.getLogger()
logger.setLevel(logging.INFO)


remoteHost = os.environ['REMOTE_IP_ADDRESS']
buffer_size = 4096        
delay = 0.0001    #wait
forward_to = (remoteHost, 80)     # place where our server would be located

class forward_Conn:    
    def __init__(self):
        self.forw_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  #creating a socket      

    def start(self, host, port):
        try:
            self.forw_socket.connect((host, port))                 
            return self.forw_socket                                
        except Exception as e:
            print(e)
            return False

class TheHTTPServer:
    socket_list = []    # for storing the objects of the incoming connections
    channel = {}        #
    '''Creating a socket & binding it to the  HTTP port'''

    def __init__(self, host, port):
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)            #for the IPV4 network and the TCP stream formation
        self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)          #setsockopt for manipulating socket options
        self.server.bind((host, port))
        self.server.listen(200)
        
    ''' appending the socket to the list and once the socket is input ready, then we recv the request to the port '''

    def INITIATE_loop(self):
        self.socket_list.append(self.server)
        while 1:
            time.sleep(delay)
            sel = select.select
            input_init, output_init, except_throw = sel(self.socket_list, [], [])
            for self.socket_ready in input_init:
                if self.socket_ready == self.server:
                    self.accept_CONN()
                    
                    break
		#checking the response from the client
                elif self.socket_ready == self.clientsock:

                    self.data = self.socket_ready.recv(buffer_size)
                    if len(self.data) == 0:
                        self.close_CONN()
                        break
                    else:
                        
                        logger.info('\n' + str(self.data,'utf-8') )
                        print(str(self.data, 'utf-8') + "\n\n\n")
                        headers = str(self.data , 'utf-8').splitlines()
                        firstLine = headers.pop(0)
                        (verb, url, version) = firstLine.split()
                        request_dict = {'verb' : verb, 'url' : url, 'version' : version}
                        for h in headers:
                            h = h.split(': ')
                            if len(h) < 2:
                                continue
                            field=h[0]
                            value= h[1]
                            request_dict[field] = value
                   
                    
                    self.recv_CONN()
                # here we are not logging the response from the server,like we are doing in above  (if we want to log html response than we can do it here)
                elif self.socket_ready == self.forward:
                   
                    self.data = self.socket_ready.recv(buffer_size)
                    if len(self.data) == 0:
                        self.close_CONN()
                        break
                    else:
                        try:
                            headers = str(self.data , 'utf-8').splitlines()
                            firstLine = headers.pop(0)
                            (verb, url, version) = firstLine.split()
                            response_dict = {'version' : verb, 'status_code' : url, 'string_status' : version}
                            for h in headers:
                                h = h.split(': ')
                                if len(h) < 2:
                                    continue
                                field=h[0]
                                value= h[1]
                                response_dict[field] = value
                        except:
                            
                            #print(str(self.data , 'utf-8') +'\n\n\n')
                            pass
                        
                        self.recv_CONN()
                        
                    
                    
    '''
    waits for socket connection, appends client socket to list
    '''

    def accept_CONN(self):
        self.forward = forward_Conn().start(forward_to[0], forward_to[1])
        self.clientsock, self.clientaddr = self.server.accept()
        
        if self.forward:
            print(self.clientaddr[0] + "has connected")
            self.socket_list.append(self.clientsock)
            self.socket_list.append(self.forward)
            self.channel[self.clientsock] = self.forward
            self.channel[self.forward] = self.clientsock
        else:
            print("Connection can't be established with remote server")
            print ("Closing the connection with clientside " +  self.clientaddr[0])
            self.clientsock.close()



    def close_CONN(self):
        print(self.socket_ready.getpeername()[0], "has disconnected")
        #removing the  objects from input_list
        self.socket_list.remove(self.socket_ready)
        self.socket_list.remove(self.channel[self.socket_ready])
        out = self.channel[self.socket_ready]
        # closing the connection with client
        self.channel[out].close()  
        # closing  the connection with remote server
        self.channel[self.socket_ready].close()

        del self.channel[out]
        del self.channel[self.socket_ready]

    def recv_CONN(self):
        data = self.data
        self.channel[self.socket_ready].send(data)

if __name__ == '__main__':

        server = TheHTTPServer( '<IPADDRESS1>' , 80)
        try:
            server.INITIATE_loop()
        except KeyboardInterrupt:
            print("Ctrl C - Stopping the Server")
            sys.exit(1)


'''
 python select() package is a direct interface to 
 the underlying operating system implementation. It
 monitors socket's, open files and pipes 
 (anything with fileno() method that returns a valid 
 file decriptor) until they become readable or writable, or a 
 communication error occurs.
 
 select make sit easier to monitor multiple connections
 at the same time. In this case monitoring happens in the OS
 network layer, instead a interpreter.
 
 
 '''
