# Makefile constructed by configure from Makefile.in and itself.
# Do not check in Makefile itself!

RM = rm -f

# For cpp-debugging with gcc.
# Missing -Wswitch -Wunused-parameter -Wunused-variable from the -Wall set
WARNINGS=\
 -Wno-import \
 -Wchar-subscripts \
 -Wcomment \
 -Wformat \
 -Wimplicit \
 -Wmain \
 -Wmultichar \
 -Wparentheses \
 -Wreturn-type \
 -Wtrigraphs \
 -Wunused-function \
 -Wunused-value \
 -Wuninitialized \
 -Wreorder \
 -Wunknown-pragmas
WARNINGS=

include .depend

############################## REAL TARGETS ###################################
OBJDIR=Opt/
# OFILES definition becomes part of .depend
$(OFILES) .dummy2:
	@ mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $(@F:.o=.c) -o $@
$(LIBTARGET) .dummy::  .depend $(OFILES)
	@ $(RM) $@
	ar qv $@ $(OFILES)

########################### META_TARGETS ######################################
.PHONY: clean fullclean cleanall install uninstall debug
clean::
	$(RM) Opt/*.$(OBJEXT) Debug/*.$(OBJEXT) $(LIBTARGET)
	if [ -d Opt ];  then rmdir Opt;  fi
	if [ -d Debug ]; then rmdir Debug; fi

fullclean cleanall:  clean
	$(RM) Makefile .depend

Makefile: Makefile.in
	(cd $(TOPDIR) && configure)

install:
uninstall: 

debug:
	$(MAKE) OBJDIR=./Debug/ OPTDEBUGFLAGS=$(COMPILER_DEBUGFLAGS)

###############################################################################
# Dependency file creation.  This creates .depend, a make dependency
# list that gets included in this makefile in subsequent makes.
#
# It also sets the value of OFILES from SRC, something extremely easy to
# do directly in gmake but near-impossible in older makes.
#
# This doesn't always get rebuilt -- it'll build it if it isn't there,
# but if the user changes a dependency chain, they should rebuild it by hand.
#
# (This has to be done file by file with the sed so that the
# target *.o files are in $(OBJDIR) rather than the current directory.)
###############################################################################
.depend:  Makefile.in $(TOPDIR)/make_depend
	@ echo "*** Building .depend file for $$(pwd) ***"
	@ mkdir -p $(@D) Opt Debug
	@ rm -f $@
	@ echo "OFILES=\\" >$@
	@ echo "$(SRC) " | sed 's#\([^ ]*\).c #$$(OBJDIR)\1.o #g' >>$@
	@ echo >>$@
	@ for file in $(SRC); do \
	    $(MAKE_DEPEND) $(CFLAGS) $$file $(MAKE_DEPEND_OUTPUT_FD)>&1 \
	        $(MAKE_DEPEND_DISCARD_FD)>/dev/null | \
	      sed 's#^\([A-Za-z0-9_]*\.o\) *:#Opt/\1 Debug/\1 $$(OBJDIR)\1:#' >>$@; \
	done
.PHONY: depend dep
depend dep: .depend
