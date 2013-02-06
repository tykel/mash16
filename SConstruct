import os
VariantDir('build','src',duplicate = 0)

env = Environment()

svnno = os.popen('svnversion .').read()[:-1]
svnno = svnno.replace('M',':')
svnno = svnno.split(':')[1]

env.Append(CCFLAGS = ['-O2', '-std=c99', '-Wall', '-Werror', '-DMAJOR=0', '-DMINOR=5', '-DREV=0', '-DBUILD='+svnno])
#env.Replace(CC = 'clang')

sources = Glob('./build/*.c')
sources.extend(Glob('./build/core/*.c'))
sources.extend(Glob('./build/header/*.c'))

libraries = ['SDLmain', 'SDL']

mash16 = env.Program(target = 'mash16',
                     source = sources,
                     LIBS = libraries)

