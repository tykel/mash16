VariantDir('build','src',duplicate = 0)

env = Environment()
env.Append(CCFLAGS = ['-g', '-O2', '-std=c99', '-Wall', '-Werror'])
env.Replace(CC = 'clang')

sources = Glob('./build/*.c')
sources.extend(Glob('./build/core/*.c'))
sources.extend(Glob('./build/header/*.c'))

libraries = ['SDLmain', 'SDL']

mash16 = env.Program(target = 'mash16',
                     source = sources,
                     LIBS = libraries)

