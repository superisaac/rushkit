from twisted.internet import reactor, defer
from twisted.internet.protocol import Protocol, Factory

import _pyrushkit as rushkit

class RTMPProtocol(Protocol):
    def handleWriteData(self, data, *args):
        self.transport.write(data)

    def amf_add(self, a, b):
        d = self.send('greeting', {'ans': a + b * 2})
        def pp(x):
            print 'got', x
        d.addCallback(pp)
        return a + b * 7

    def send(self, method, *args):
        reqid = rushkit.my_amf_call(self._proto, method, list(args))
        d = defer.Deferred()
        self._calls[reqid] = d
        return d
    
    def handleInvoke(self, reqid, methodname, args):
        if methodname == '_result':
            if reqid in self._calls:
                self._calls[reqid].callback(args[0])
                del self._calls[reqid]
        else:
            func = getattr(self, 'amf_%s' % methodname, None)
            if func:
                ret = func(*tuple(args))
                if ret:
                    rushkit.amf_return(self._proto, reqid, ret)
                
    def connectionMade(self):
        print 'connection made'
        self._calls = {}
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
