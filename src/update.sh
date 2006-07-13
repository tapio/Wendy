#!/bin/sh
#
# This script creates the Makefile.am file, for use with automake.
#

self=`basename $0`

# Collect header files.
files=`ls *.cpp`
if [ -z "$files" ]; then
  echo "no source files: punting"
  exit 1
fi

rm -f Makefile.am

echo "# Generated by $self." >> Makefile.am
echo "# DO NOT EDIT THIS FILE!" >> Makefile.am
echo >> Makefile.am

echo "AM_CXXFLAGS = @moira_CFLAGS@" >> Makefile.am
echo >> Makefile.am

echo "lib_LIBRARIES = libwendy.a" >> Makefile.am
echo >> Makefile.am

echo -n "libwendy_a_SOURCES =" >> Makefile.am
for file in $files; do
  echo -n " $file" >> Makefile.am
done
echo >> Makefile.am
echo >> Makefile.am

# Rule for redistributing self.
echo "EXTRA_DIST = $self" >> Makefile.am
echo >> Makefile.am

