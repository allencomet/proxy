import os, sys, time
from glob import glob

ccflags = ['-std=c++11']

if ARGUMENTS.get('release', '0') == '0':
  ccflags += ['-O2', '-g2', '-Werror', '-Wno-deprecated']	
else:
  ccflags += ['-O2', '-g0', '-Wall', ]		

ccdefines = {'_FILE_OFFSET_BITS' : '64', 'DEBUG' : 1, }

env = Environment(CC = 'gcc', 
	LIBS = ['stdc++', 'm', 'pthread', 'dl', 'rt'], 
	LIBPATH = ['/usr/lib', '/usr/local/lib', './lib'], 
	LINKFLAGS = ['-Wl,--no-as-needed','-rdynamic'],
	CPPPATH = ['.'])

env.Append(CPPFLAGS = ccflags)
env.Append(CPPDEFINES = ccdefines)

common_source_files = glob('util/impl/*.cc') + \
			glob('util/json/impl/*.cc') + \
			glob('base/*.cc') + \
	      	glob('base/ccflag/*.cc') + \
	       	glob('base/cclog/*.cc') + \
	       	glob('base/cctest/*.cc')

proxy_common_source_files = glob('proxy/*.cc') + \
			glob('proxy/core/*.cc') + \
			glob('proxy/lb/*.cc') + \
			common_source_files

rpc_source_files = ['rpc.cc'] + \
		glob('proxy/rpc/*.cc') + \
		proxy_common_source_files 

ipc_source_files = ['ipc.cc'] + \
		glob('proxy/ipc/*.cc') + \
		proxy_common_source_files


test_source_files = glob("test/*.cc") + \
			glob("client/*.cc") + \
			glob('proxy/*.cc') + \
			glob('proxy/ipc/*.cc') + \
			glob('proxy/rpc/*.cc') + \
			glob('proxy/core/*.cc') + \
			common_source_files 

#print("mt source code list: >>")
#for s in rpc_source_files:
#	print(os.path.realpath(s))
#print('')

env.Program('bin/proxy', rpc_source_files)
env.Program('bin/bus/backend', ipc_source_files)
env.Program('bin/test', test_source_files)


#os.system('mv proxy debug/bin/')


