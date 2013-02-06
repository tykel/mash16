# mash16 SConfigure, 2013

maj = '0'
min = '5'
rev = '0'

import os
VariantDir('build','src',duplicate = 0)

env = Environment(TARFLAGS = '-cz' )
env.Alias('install', ['/usr/local/bin'])

# Use `svnversion` for revision number

svnno = os.popen('svnversion .').read()[:-1].replace('M',':').split(':')[1]

# Standard C flags, use svn revision too

env.Append(CCFLAGS = ['-O2', '-s', '-std=c99', '-Wall', '-Werror', '-DMAJOR='+maj, '-DMINOR='+min, '-DREV='+rev, '-DBUILD='+svnno])

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

tar_src = env.Tar('mash16-'+maj+'.'+min+'.'+rev+'-src.tar.gz',
             ['src', 'INSTALL', 'LICENSE', 'SConstruct', 'SPEC.1.1'])
tar = env.Tar('mash16-'+maj+'.'+min+'.'+rev+'.tar.gz', 'mash16')

#Install

env.Install(dir = "/usr/local/bin", source = mash16)
