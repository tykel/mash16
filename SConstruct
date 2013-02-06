# mash16 SConfigure, 2013

import os
VariantDir('build','src',duplicate = 0)

env = Environment()
env.Alias('install', ['/usr/local/bin'])

# Use `svnversion` for revision number

svnno = os.popen('svnversion .').read()[:-1]
svnno = svnno.replace('M',':')
svnno = svnno.split(':')[1]

# Standard C flags, use svn revision too

env.Append(CCFLAGS = ['-O2', '-std=c99', '-Wall', '-Werror', '-DMAJOR=0', '-DMINOR=5', '-DREV=0', '-DBUILD='+svnno])

# Clang generates smaller binaries... but gcc is more widely installed

#env.Replace(CC = 'clang')

sources = Glob('./build/*.c')
sources.extend(Glob('./build/core/*.c'))
sources.extend(Glob('./build/header/*.c'))

libraries = ['SDLmain', 'SDL']

# Compile

mash16 = env.Program(target = 'mash16',
                     source = sources,
                     LIBS = libraries)

#Install

env.Install(dir = "/usr/local/bin", source = mash16)
