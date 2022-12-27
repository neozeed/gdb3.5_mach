#
# HISTORY
# $Log:	Makefile,v $
# Revision 1.26  90/09/04  18:28:41  mrt
# 	Define alloca for all machines, since the standard ones 
# 	bomb out on Suns, Encores ...
# 	[90/05/08            af]
# 
# Revision 1.25  90/04/11  14:41:00  af
# 	Added i386 defs.
# 	[90/03/24            af]
# 
# 	Created
# 	[90/03/20            mrt]
# 
# Revision 1.24  90/04/06  13:48:58  mrt
# Changes for Mach
# 
#
#  Makefile to fit into our build scheme, but to call
#  the original FSF Makefile to do the real work.
#

# i386 boxes
at386_makedefine="-DM_CLIBS=-lmach"
at386_paramfile=m-i386mach.h
at386_pinsfile=i386-pinsn.c
at386_opcodefile=/dev/null
at386_depfile=i386mach-dep.c
at386_munch="NAMES_HAVE_UNDERSCORE"

# Encore mmax (ns32k)
mmax_makedefine="-DM_CLIBS=-lmach"
mmax_paramfile=m-ns32kmach.h
mmax_pinsfile=ns32k-pinsn.c
mmax_opcodefile=ns32k-opcode.h
mmax_depfile=ns32kmach-dep.c
mmax_munch="NAMES_HAVE_UNDERSCORE COFF"

# Pmaxen (and all mips boxes)
pmax_makedefine="-DM_CLIBS=-lmach"
pmax_paramfile=m-mipsmach.h
pmax_pinsfile=mips-pinsn.c
pmax_opcodefile=mips-opcode.h
pmax_depfile=mipsmach-dep.c
pmax_munch=

# Sun3 (and all 68k boxes)
sun_makedefine="-DM_CLIBS=-lmach"
sun_paramfile=m-sun3mach.h
sun_pinsfile=m68k-pinsn.c
sun_opcodefile=m68k-opcode.h
sun_depfile=sun3mach-dep.c
sun_munch=NAMES_HAVE_UNDERSCORE

# Sun4 (and all sparc boxes)
sun4_makedefine="-DM_CLIBS=-lmach"
sun4_paramfile=m-sun4mach.h
sun4_pinsfile=sparc-pinsn.c
sun4_opcodefile=sparc-opcode.h
sun4_depfile=sparcmach-dep.c
sun4_munch=NAMES_HAVE_UNDERSCORE

# Vax
vax_makedefine="-DM_CLIBS=-lmach"
vax_paramfile=m-vaxmach.h
vax_pinsfile=vax-pinsn.c
vax_opcodefile=vax-opcode.h
vax_depfile=vaxmach-dep.c
vax_munch=NAMES_HAVE_UNDERSCORE

makedefine="$($(machine)_makedefine) -DM_CFLAGS=-I/afs/cs/project/mach/src/lib/threads -DM_ALLOCA=alloca.o"
paramfile=$($(machine)_paramfile)
pinsfile=$($(machine)_pinsfile)
opcodefile=$($(machine)_opcodefile)
depfile=$($(machine)_depfile)

machine_files= $(paramfile) $(pinsfile) $(opcodefile) $(depfile)

INSTALL_DIR=$(DSTBASE)/bin

install: all gdb.install

gdb.install: gdb
	-mach_install -d gdb $(INSTALL_DIR)
	-@touch gdb.install

all: ./Makefile.build gdb

./Makefile.build: Makefile.dist Makefile munch $(machine_files)
	echo "Editing Makefile"
	cp Makefile.dist tmp.c
	cc -E >./Makefile.build tmp.c $(makedefine) -DM_MAKEDEFINE="$(makedefine)"
	rm -f tmp.c
	-@if [ ! -r param.h ]; then \
		ln -s $(paramfile) param.h; \
	else true; fi
	-@if [ ! -r pinsn.c ]; then \
		ln -s $(pinsfile) pinsn.c; \
	else true; fi
	-@if [ ! -r opcode.h ]; then \
		ln -s $(opcodefile) opcode.h; \
	else true; fi
	-@if [ ! -r dep.c ]; then \
		ln -s $(depfile) dep.c; \
	else true; fi
	-@if [ ! -r munch ]; then \
		ln -s  munch ./munch; \
	else true; fi
	-@if [ ! -d ./readline ]; then \
	echo "mkdir ./readline" ; \
	mkdir ./readline ;\
	else true; fi


gdb: ./Makefile.build munch
	$(MAKE) MUNCH_DEFINE=$($(machine)_munch) -f ./Makefile.build


