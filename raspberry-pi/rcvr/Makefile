#############################################################################
#
# License: GPL (General Public License)
# Author:  gnulnulf <arco@appeltaart.mine.nu>
# Date:    2013/02/07 (version 1.0)
#
# Description:
# ------------
# use make to create the binary
#

ifeq ($(wildcard ./Makefile.inc), )
	$(error Configuration not found. Run ./configure first)
endif

include ./Makefile.inc

# define all programs
PROGRAMS = rcvr

include Makefile.examples
