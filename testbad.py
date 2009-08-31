from twisted.internet import reactor
from twisted.internet.protocol import Protocol, Factory

import _pyrushkit as rushkit

class RTMPProtocol(Protocol):
    def handleWriteData(self, data, *args):
        self.transport.write(data)

    def handleInvoke(self, reqid, methodname, args):
        print '---', methodname, args
        if methodname == 'add':
            if len(args) == 2:
                rushkit.amf_return(self._proto, reqid, args[0] + args[1])
                rushkit.amf_call(self._proto, "greeting", ["greet"])
    def connectionMade(self):
        print 'connection made'
        self._proto = rushkit.new_RTMP_PROTO()
        rushkit.init_responder(self._proto, self)        

    def dataReceived(self, data):
        rushkit.feed_data(self._proto, data)
    
    def connectionLost(self, reason):
        if self._proto:
            rushkit.delete_RTMP_PROTO(self._proto)
            self._proto = None

class RTMPFactory(Factory):
    protocol = RTMPProtocol
    
if __name__ == '__main__':
    rushkit.environment_init()
    factory = RTMPFactory()
    reactor.listenTCP(1935, factory)
    reactor.run()
