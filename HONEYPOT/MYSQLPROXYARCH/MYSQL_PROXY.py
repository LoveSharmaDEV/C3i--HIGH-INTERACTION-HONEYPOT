import socket
import select
import time
import sys
import logging
import os


logging.basicConfig(filename= './PROXIES/logs/MYSQL_PROXY.log' , format='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p' , filemode='a')
logger=logging.getLogger()
logger.setLevel(logging.INFO)

remoteHost = os.environ['REMOTE_IP_ADDRESS']
buffer_size = 4096
delay = 0.0001
forward_to = (remoteHost, 3306)

class forw_CONN:
    def __init__(self):
        self.forward_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def start(self, host, port):
        try:
            self.forward_socket.connect((host, port))
            return self.forward_socket
        except Exception as e:
            print(e)
            return False

class TheMYSQLServer:
    socket_list = []
    channel = {}
    ''' Create socket and bind it to HTTP port'''

    def __init__(self, host, port):
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server.bind((host, port))
        self.server.listen(200)
        
    ''' append the socket into the list , once the socket is input ready
    recv the request to the port'''

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
                elif self.socket_ready == self.clientsock:

                    self.data = self.socket_ready.recv(buffer_size)
                    if str(self.data, 'utf-8' , errors='ignore') == "BYE" or str(self.data, 'utf-8', errors='ignore' )== "EXIT":
                        sys.exit(1)
                    if len(self.data) == 0:
                        self.close_CONN()
                        break
                    else:
                        
                        self.hexdump(self.data)
                       
                   
                    
                    self.recv_CONN()
                        
                elif self.socket_ready == self.forward:
                   
                    self.data = self.socket_ready.recv(buffer_size)
                    if len(self.data) == 0:
                        self.close_CONN()
                        break
                    else:
                        try:
                            self.hexdump(self.data)
                            
                        except:
                            
                            #print(str(self.data , 'utf-8') +'\n\n\n')
                            pass
                        
                        self.recv_CONN()
                        
                    
                    
    '''
    waits for socket connection, appends client socket to list
    '''

    def accept_CONN(self):
        self.forward = forw_CONN().start(forward_to[0], forward_to[1])
        self.clientsock, self.clientaddr = self.server.accept()
        
        if self.forward:
            print(self.clientaddr[0] + "has connected")
            self.socket_list.append(self.clientsock)
            self.socket_list.append(self.forward)
            self.channel[self.clientsock] = self.forward
            self.channel[self.forward] = self.clientsock
        else:
            print("Can't establish connection with remote server.")
            print ("Closing connection with client side " +  self.clientaddr[0])
            self.clientsock.close()



    def close_CONN(self):
        print(self.socket_ready.getpeername()[0], "has disconnected")
        self.server.close()
        self.clientsock.close()
        sys.exit(0)
        #remove objects from input_list
        self.socket_list.remove(self.socket_ready)
        self.socket_list.remove(self.channel[self.socket_ready])
        out = self.channel[self.socket_ready]
        # close the connection with client
        self.channel[out].close()  # equivalent to do self.s.close()
        # close the connection with remote server
        self.channel[self.socket_ready].close()

        del self.channel[out]
        del self.channel[self.socket_ready]

    def recv_CONN(self):
        data = self.data
        self.channel[self.socket_ready].send(data)
        
        '''
        Simple implementation of hexdump
        https://gist.github.com/JonathonReinhart/509f9a8094177d050daa84efcd4486cb
        
        '''
    def hexdump(self, data, length=16):
        filter = ''.join([(len(repr(chr(x))) == 3) and chr(x) or '.' for x in range(256)])
        lines = []
        digits = 4 if isinstance(data, str) else 2
        for c in range(0, len(data), length):
            chars = data[c:c+length]
            hex = ' '.join(["%0*x" % (digits, (x)) for x in chars])
            printable = ''.join(["%s" % (((x) <= 127 and filter[(x)]) or '.') for x in chars])
            lines.append("%04x  %-*s  %s\n" % (c, length*3, hex, printable))
        print(''.join(lines))
        logger.info(''.join(lines)  )


if __name__ == '__main__':

        server = TheMYSQLServer('<IPADDRESS1>', 55555)
        try:
            server.INITIATE_loop()
        except KeyboardInterrupt:
            print("Ctrl C - Stopping server")
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
