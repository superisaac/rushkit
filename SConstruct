import sys

env = Environment(CPPPATH="include",
                  CXXFLAGS='-g ')

proto_src = ['rt_pool.c', 'buffer.c', 'packet.c', 'protocol.c',
	  'amf.c', 'utils.c']
env.Library('lib/librushcore.a', ['core/src/%s' % f for f in proto_src])


demo_src = ["connection.cpp", 'main.cpp']
cpppath = ['include']
libpath = ['lib']
if sys.platform.startswith('darwin'): # mac 
    cpppath.append('/opt/local/include')
    libpath.append('/opt/local/lib')

env = Environment(CPPPATH=cpppath,
                  CXXFLAGS='-g ',
                  LDFLAGS='-static',
                  LIBS=['event', 'rushcore'],
                  LIBPATH=libpath)
env.Program('bin/demo-server',
            ['aux/%s' % f for f in demo_src])

import re
import sys
 
def SWIGSharedLibrary(env, library, sources, **args):
  swigre = re.compile('(.*).i')
  if env.WhereIs('swig') is None:
    sourcesbis = []
    for source in sources:
      cName = swigre.sub(r'\1_wrap.c', source)
      cppName = swigre.sub(r'\1_wrap.cc', source)
      if os.path.exists(cName):
        sourcesbis.append(cName)
      elif os.path.exists(cppName):
        sourcesbis.append(cppName)
      else:
        sourcesbis.append(source)
  else:
    sourcesbis = sources
 
  if 'SWIGFLAGS' in args:
    args['SWIGFLAGS'] += ['-python']
  else:
    args['SWIGFLAGS'] = ['-python'] + env['SWIGFLAGS']
  args['SHLIBPREFIX']=""
  if sys.version >= '2.5':
    args['SHLIBSUFFIX']=".so"
 
  cat=env.SharedLibrary(library, sourcesbis, **args)
  return cat
 
env = Environment(CPPPATH=cpppath + ['/usr/include/python2.5'],
                  CXXFLAGS='-g ',
                  LDFLAGS='-static',
                  LIBS=['python2.5', 'rushcore'],
                  LIBPATH=['lib'])
env['BUILDERS']['PythonModule'] = SWIGSharedLibrary
env.PythonModule('_pyrushkit', ['bindings/rushkit.i', 'bindings/python/pyrushkit.c'])

