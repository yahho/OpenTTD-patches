#!/bin/bash

# $Id$

# This file is part of OpenTTD.
# OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.

# Set neutral locale so sort behaves the same everywhere
LC_ALL=C
export LC_ALL

# We really need gawk for this!
AWK=gawk

${AWK} --version > /dev/null 2> /dev/null
if [ "$?" != "0" ]; then
	echo "This script needs gawk to run properly"
	exit 1
fi

# This must be called from within a src/???/api directory.
scriptdir=`dirname $0`
apilc=`pwd | sed "s@/api@@;s@.*/@@"`

# Check if we are in the root directory of the API, as then we generate all APIs
if [ "$apilc" = "script" ]; then
	for api in `find -type d | cut -b3- | grep -v '\.svn\|/'`; do
		if [ -z "$api" ]; then continue; fi
		echo "Generating for API '$api' ..."
		cd $api
		sh $scriptdir/../`basename $0`
		cd ..
	done
	exit 0
fi

case $apilc in
	ai) apiuc="AI" ;;
	game) apiuc="GS" ;;
	*) echo "Unknown API type."; exit 1 ;;
esac

if [ -z "$1" ]; then
	for f in `ls ../*.hpp`; do
		bf=`basename ${f} | sed s@script_@${apilc}_@`

		# ScriptController has custom code, and should not be generated
		if [ "`basename ${f}`" = "script_controller.hpp" ]; then continue; fi
		if [ "`basename ${f}`" = "script_object.hpp" ]; then continue; fi

		${AWK} -v api=${apiuc} -f ${scriptdir}/squirrel_export.awk ${f} > ${bf}.tmp

		if [ "`wc -l ${bf}.tmp | cut -d\  -f1`" = "0" ]; then
			if [ -f "${bf}.sq" ]; then
				echo "Deleted: ${bf}.sq"
				svn del --force ${bf}.sq > /dev/null 2>&1
			fi
			rm -f ${bf}.tmp
		elif ! [ -f "${bf}.sq" ] || [ -n "`diff -I '$Id' ${bf}.tmp ${bf}.sq 2> /dev/null || echo boo`" ]; then
			mv ${bf}.tmp ${bf}.sq
			echo "Updated: ${bf}.sq"
			svn add ${bf}.sq > /dev/null 2>&1
			svn propset svn:eol-style native ${bf}.sq > /dev/null 2>&1
			svn propset svn:keywords Id ${bf}.sq > /dev/null 2>&1
		else
			rm -f ${bf}.tmp
		fi
	done
else
	${AWK} -v api=${apiuc} -f ${scriptdir}/squirrel_export.awk $1 > $1.tmp
	if [ `wc -l $1.tmp | cut -d\  -f1` -eq "0" ]; then
		if [ -f "$1.sq" ]; then
			echo "Deleted: $1.sq"
			svn del --force $1.sq > /dev/null 2>&1
		fi
		rm -f $1.tmp
	elif ! [ -f "${f}.sq" ] || [ -n "`diff -I '$Id' $1.sq $1.tmp 2> /dev/null || echo boo`" ]; then
		mv $1.tmp $1.sq
		echo "Updated: $1.sq"
		svn add $1.sq > /dev/null 2>&1
		svn propset svn:eol-style native $1.sq > /dev/null 2>&1
		svn propset svn:keywords Id $1.sq > /dev/null 2>&1
	else
		rm -f $1.tmp
	fi
fi

# Remove .hpp.sq if .hpp doesn't exist anymore
for f in `ls *_*.hpp.sq`; do
	f=`echo ${f} | sed "s/.hpp.sq$/.hpp/;s@${apilc}_@script_@"`
	if [ ! -f ../${f} ];then
		echo "Deleted: ${f}.sq"
		svn del --force ${f}.sq > /dev/null 2>&1
	fi
done


# Add stuff to ${apilc}.hpp.sq
f=${apilc}.hpp.sq

cat > ${f}.tmp << EOF
/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/* THIS FILE IS AUTO-GENERATED; PLEASE DO NOT ALTER MANUALLY */

EOF

grep -l '^static void SQ'${apiuc}'.*_Register *(Squirrel \*engine)$' *_*.hpp.sq |
        sort | sed -e "s/^/#include \"/" -e 's/$/"/' >> ${f}.tmp

echo >> ${f}.tmp ''
echo >> ${f}.tmp 'static void SQ'${apiuc}'_Register (Squirrel *engine)'
echo >> ${f}.tmp '{'

# List needs to be registered with squirrel before all List subclasses.
echo >> ${f}.tmp '	SQ'${apiuc}'List_Register (engine);'

sed -n -e 's/^static void \(SQ'${apiuc}'.*\)_Register *(Squirrel \*engine)$/\1/p' \
                *_*.hpp.sq | sort |
        sed -e '/^SQ'${apiuc}'Controller$/d' -e '/^SQ'${apiuc}'List$/d' \
                -e 's/^.*$/	&_Register (engine);/' >> ${f}.tmp

echo >> ${f}.tmp '}'

if ! [ -f "${f}" ] || [ -n "`diff -I '$Id' ${f} ${f}.tmp 2> /dev/null || echo boo`" ]; then
	mv ${f}.tmp ${f}
	echo "Updated: ${f}"
else
	rm -f ${f}.tmp
fi
