import gc
print gc.DEBUG_LEAK
#gc.set_debug(gc.DEBUG_LEAK|gc.DEBUG_STATS|gc.DEBUG_OBJECTS)
import _pyrushkit as rushkit

rushkit.environment_init()
class AAA:
    def __del__(self):
        print 'deleted'

a = AAA()
print 'a1', len(gc.get_referrers(a))
proto = rushkit.new_RTMP_PROTO()
rushkit.init_responder(proto, a)

print 'a2', len(gc.get_referrers(a))
b = rushkit.get_py_data(proto)
print 'b1', len(gc.get_referrers(b))
print id(a), id(b)

rushkit.free_responder(proto)
#print 'a3', len(gc.get_referrers(a))

