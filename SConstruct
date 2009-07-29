import sys

env = Environment(CPPPATH="include",
                  CXXFLAGS='-g ')

proto_src = ['rt_pool.c', 'buffer.c', 'packet.c', 'protocol.c',
	  'amf.c', 'utils.c']
env.Library('lib/librushkit.a', ['src/%s' % f for f in proto_src])


demo_src = ["connection.cpp", 'main.cpp']
cpppath = ['include']
libpath = ['lib']
if sys.platform.startswith('darwin'): # mac 
    cpppath.append('/opt/local/include')
    libpath.append('/opt/local/lib')

env = Environment(CPPPATH=cpppath,
                  CXXFLAGS='-g ',
                  LDFLAGS='-static',
                  LIBS=['event', 'rushkit'],
                  LIBPATH=libpath)
env.Program('bin/demo-server',
            ['aux/%s' % f for f in demo_src])
