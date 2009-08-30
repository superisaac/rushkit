import _pyrushkit as rushkit

rushkit.environment_init()
class AAA:
    def aaa(self, a):
        print '3'
        return 2 + a
    def __del__(self):
        print 'deleted'

a = AAA()
proto = rushkit.new_RTMP_PROTO()
rushkit.init_responder(proto, a)

b = rushkit.get_py_data(proto)
print id(a), id(b)

rushkit.free_responder(proto)
rushkit.feed_data(proto, '\x03')
rushkit.delete_RTMP_PROTO(proto)

