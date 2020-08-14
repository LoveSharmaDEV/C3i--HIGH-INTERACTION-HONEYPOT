import threading
import socket
import sys
import traceback
import paramiko
import logging
import os

# The class defines an interface for controlling the behaviour of Paramiko in server mode
# Methods in this class are called Paramiko's primary thread
PORT = 22
HOST_KEY = paramiko.RSAKey(filename='/PROXIES/keys/private.key')
logging.basicConfig(filename= '/PROXIES/logs/SSH_PROXY.log' , format='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p' , filemode='a')
logger=logging.getLogger()
logger.setLevel(logging.INFO)



class SSHsession(paramiko.ServerInterface):
    
  
    
    def __init__(self):
        self.event = threading.Event()
        
        
        
    def check_channel_request(self , kind , chanid):
        if kind == "session" :
            return paramiko.OPEN_SUCCEEDED
        return paramiko.OPEN_FAILED_ADMINISTRATIVELY_PROHIBITED
   
    def check_auth_password(self, username, password):
        
        logger.info("Username is | "+ username +" |  password is | "+ password + " |" )

        if username == 'root' and password =='root':
            return paramiko.AUTH_SUCCESSFUL
        else:
            return  paramiko.AUTH_FAILED

            

        # return successfull for all in condition
    
    def check_auth_publickey(self, username, key):
        return paramiko.AUTH_SUCCESSFUL
    
    def check_channel_shell_request(self, channel):
        self.event.set()
        return True
    
    def check_channel_pty_request(self, channel, term, width, height, pixelwidth, pixelheight, modes):
        return True
    
    def check_channel_exec_request(self, channel, command):

        return True
                
    def check_channel_env_request(self, channel, name, value):
        return True
 
    
 
class START_SERVER:
    
    
    def __init__(self , REMOTE_ADDRESS , REMOTE_USERNAME , REMOTE_PASSWORD):
        
        self.REMOTE_ADDRESS = REMOTE_ADDRESS
        self.REMOTE_USERNAME = REMOTE_USERNAME
        self.REMOTE_PASSWORD = REMOTE_PASSWORD
    
   
    def start_server(self):
        
        
        
        '''
        # 1. Create socket for endpoint connections
        # 2. Once the socket is created bind the socket to the port
        # 3. Now the incoming data will hit the port
        # 4. For attracting data socket has to start to listen
        # 5. After active listening start along with accepting connections
        '''
        
        try:
            sket = socket.socket(socket.AF_INET , socket.SOCK_STREAM)
            sket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR , 1)
            sket.bind(("" , PORT))
        except Exception as e:
            print("Bind Failed" + str(e))
            traceback.print_exc()
            sys.exit(1)
            
        
        try:
            sket.listen(100)
            print("listening for connection....")
            client , addr = sket.accept()
            logger.info("Connection has been established " + "IP " + addr[0] + "at port " + str(addr[1]))
        except Exception as e:
            print("*** Listen/accept failed***" + str(e))
            traceback.print_exc()
            sys.exit(1)
            
            
            '''
            # Now we have to start the paramiko SSH server on the port where we have created and activated Socket
            1. paramiko.Transport:
                An ssh Transport attaches to a stream , negotiates an encrypted session , authenticates and then 
                create stream tunnels, called channels across the session.
            2. transport.add_server_key():
                Add a host key to the list of keys used for server mode. When behaving as a server , the host key
                is used to sign certain packets during SSH negotiation, so that the client can trust that we are who
                we say we are.
                
            3. transport.start_server():
                Starts the server side of SSH
            '''
           
            
            
        try:
            transport = paramiko.Transport(client)
            transport.add_server_key(HOST_KEY)
            transport.local_version = "SSH-2.0-OpenSSH_7.6p1 Ubuntu-4ubuntu0.3"
            server = SSHsession()
            
            
            try:
                transport.start_server(server = server)
            except paramiko.SSHException:
                print("SSH negotiation failed")
                raise Exception("SSH negotiation failed")
                
                
            chan = transport.accept()
            if chan is None:
                print("No Channel")
                raise Exception("No Channel")
                
            server.event.wait(10)
            send_chan = ssh( self.REMOTE_ADDRESS, self.REMOTE_USERNAME , self.REMOTE_PASSWORD , chan ) 
            send_chan.openShell()

            if not server.event.is_set():
                    print('*** Client never asked for a shell.')
                    raise Exception("No shell request")
            try:
                chan.send("Welcome to the my control server\r\n\r\n")
                
                run = True
                while run:
                    chan.send(str('>'))
                    command = ""
                    while not command.endswith("\r"):
                        transport = chan.recv(1024)
                        #f = chan.makefile("rU")
                        #f.readline().strip("\r\n")
                        # Echo input to psuedo-simulate a basic terminal
                        chan.send(transport)
                        command += transport.decode("utf-8")
    
                    chan.send("\r\n")
                    command = command.rstrip()
                    logger.info("Command Entered | "+ command + " | By User IP |" + addr[0]+ " | At Port | "+ str(addr[1]) ) 
                    print(command)
                    if command == "exit":
                        sket.close()
                        run = False
                    else:
                        send_chan.sendShell(command , chan)
                        pass
                        ##handle_cmd(command, chan)
            except Exception as err:
                print('!!! Exception: {}: {}'.format(err.__class__, err))
                traceback.print_exc()
                try:
                    transport.close()
                except Exception:
                    pass
                
            chan.close()
            
        except Exception as err:
                print('!!! Exception: {}: {}'.format(err.__class__, err))
                traceback.print_exc()
                try:
                    transport.close()
                except Exception:
                    pass
             
class ssh:
    shell = None
    client = None
    transport = None
 
    def __init__(self, address, username, password , trans_chan):
        print("Connecting to server on ip", str(address) + ".")
        self.client = paramiko.client.SSHClient()
        self.client.set_missing_host_key_policy(paramiko.client.AutoAddPolicy())
        self.client.connect(address, username=username, password=password, look_for_keys=False)
        self.transport = paramiko.Transport((address, 22))
        self.transport.connect(username=username, password=password)
        self.trans_chan = trans_chan
        
 
        thread = threading.Thread(target=self.process)
        thread.daemon = True
        thread.start()
        
      
        
 
    def closeConnection(self):
        if(self.client != None):
            self.client.close()
            self.transport.close()
 
    def openShell(self):
        self.shell = self.client.invoke_shell()
 
    def sendShell(self, command , chan):
        self.chan = chan
        if(self.shell):
            self.shell.send(command + "\n")
        else:
            print("Shell not opened.")
 
    def process(self):
        global connection
        while True:
            # Print data when available
            if self.shell != None and self.shell.recv_ready():
                alldata = self.shell.recv(1024)
                while self.shell.recv_ready():
                    alldata += self.shell.recv(1024)
                strdata = str(alldata, "utf8")
                strdata.replace('\r', '')
                self.trans_chan.send(strdata)
                
                if(strdata.endswith("$ ")):
                    self.trans_chan.send("\n$ " + '\r \n')






if __name__ == "__main__":
    remoteHost = os.environ['REMOTE_IP_ADDRESS']
    init_server = START_SERVER(remoteHost, 'root','root')
    init_server.start_server()
   
    
     
   
