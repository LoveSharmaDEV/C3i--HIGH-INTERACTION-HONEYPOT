import sys
import os
from twisted.internet import protocol, reactor
import logging


LISTEN_PORT = 23
SERVER_PORT = 23 
SERVER_ADDR = os.environ['REMOTE_IP_ADDRESS']


logging.basicConfig(filename= '/TELNETPROXY/TELNET_PROXY.log' , format='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p' , filemode='a')
logger=logging.getLogger()
logger.setLevel(logging.INFO)

class ServerProtocol(protocol.Protocol):
    
    def __init__(self):
        self.buffer = None
        self.client = None
        self.commandBuffer = ''
        
    def connectionMade(self, trigger=0):
        
        self.ban = open("/TELNETPROXY/banner.dat", "rb")
        self.l = self.ban.read(1024)
        while (self.l):
            self.transport.write(self.l)
            self.l = self.ban.read(1024)
        self.ban.close()
        
  
        self.attack_ip, self.attack_port = self.transport.client 
        factory = protocol.ClientFactory()
        factory.protocol = ClientProtocol
        factory.server = self   
        reactor.connectTCP(SERVER_ADDR, SERVER_PORT, factory)


    
    def dataReceived(self, data):
        
        
        
        if(data != b'\r\x00'):
            self.commandBuffer +=  str(  data , 'utf-8' , errors='ignore')
        else:
            print(self.commandBuffer)
            logger.info("Activity from IP " + self.attack_ip + " at port " +  str(self.attack_port) + "command used "+ self.commandBuffer  )
            if self.commandBuffer == 'exit':
                sys.exit()
            self.commandBuffer = ''
            

        
        if self.client:
            self.client.write(data)
        else:
            print("in buffer section")
            self.buffer = data
            
    
    def write(self,data):
        
        self.transport.write(data)
        
        
            
# As client 
class ClientProtocol(protocol.Protocol):
    def connectionMade(self):
        self.factory.server.client = self
        self.write(self.factory.server.buffer)
        self.factory.server.buffer = ''
 
    # Server => Proxy
    def dataReceived(self, data):
        self.factory.server.write(data)
 
    # Proxy => Server
    def write(self, data):
        if data:
            self.transport.write(data)
 
 
 
def main():
    factory = protocol.ServerFactory()
    factory.protocol = ServerProtocol
 
    reactor.listenTCP(LISTEN_PORT, factory)
    reactor.run()
 
 
if __name__ == '__main__':
	main()

            
