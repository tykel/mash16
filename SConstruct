# mash16 SConstruct, 2013

maj = '0'
min = '5'
rev = '8'

import os

VariantDir('build','src',duplicate = 0)

env = Environment(TARFLAGS = '-cz' )
env.Alias('install', ['/usr/local/bin'])

# Get Git tag with git rev-parse

gittag = ""
if(not os.path.isdir(".git")):
    f = open("gitrev","r")
    gittag = f.read()
    f.close()
else:
    gittag = "\\\""+os.popen('git rev-parse HEAD | cut -c-10').read()[:-1]+"\\\""
    f = open("gitrev","w")
    f.write(gittag)
    f.close()

# Standard C flags, use svn revision too

env.Append(CCFLAGS = ['-O2', '-std=c99', '-Wall', '-DMAJOR='+maj, '-DMINOR='+min, '-DREV='+rev, '-DBUILD='+gittag])

cc = ARGUMENTS.get('CC',0)
if cc:
    env.Replace(CC = cc)

env.Append(CCFLAGS= os.popen('pkg-config --cflags sdl').read().split())

sources = Glob('./build/*.c')
sources.extend(Glob('./build/core/*.c'))
sources.extend(Glob('./build/header/*.c'))

libraries = os.popen('pkg-config --libs sdl').read().split()

# Compile

mash16 = env.Program(target = 'mash16',
                     source = sources,
                     LIBS = libraries)

tar_src = env.Tar('archive/mash16-'+maj+'.'+min+'.'+rev+'-src.tar.gz',
             ['src', 'INSTALL', 'LICENSE', 'SConstruct', 'SPEC.1.1', 'gitrev'])
tar = env.Tar('archive/mash16-'+maj+'.'+min+'.'+rev+'.tar.gz', 'mash16')

#Install

env.Install(dir = "/usr/local/bin", source = mash16)
