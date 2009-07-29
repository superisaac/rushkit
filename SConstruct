import sys

env = Environment(CPPPATH="include",
                  CXXFLAGS='-g ')

proto_src = ['rt_pool.c', 'buffer.c', 'packet.c', 'protocol.c',
	  'amf.c', 'utils.c']
env.Library('lib/librtmpproto.a', ['src/%s' % f for f in proto_src])


sample_src = ["connection.cpp", 'main.cpp']
cpppath = ['include']
if sys.platform.startswith('darwin'): # mac 
    cpppath.append('/opt/local/include')
env = Environment(CPPPATH=cpppath,
                  CXXFLAGS='-g ',
                  LDFLAGS='-static',
                  LIBS=['event', 'rtmpproto'],
                  LIBPATH=['lib', '/opt/local/lib'])
env.Program('bin/demo-server',
            ['aux/%s' % f for f in sample_src])
