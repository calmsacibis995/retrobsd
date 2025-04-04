#!/bin/sh -
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
CV=`cat .compileversion`
CV=`expr $CV + 1`
OV=`cat .oldversion`
GITREV=`git rev-list HEAD --count`
GITBR=`git rev-parse --abbrev-ref HEAD`

if [ "x$GITREV" = "x" ]
then
    GITREV="Untracked"
fi

if [ "x$GITREV" != "x$OV" ]
then
    CV=1
fi
echo $CV >.compileversion
echo $GITREV >.oldversion

echo $GITREV `date +'%y%m%d'` `date +'%H%M'` $GITBR $CV| \
awk ' {
    version = $1;
    date = $2;
    time = $3;
    branch = $4;
    cv = $5
    printf "const char version[] = \"RetroBSD 1.%s.%d.%s.%s-%s:\\n", version, cv, branch, date, time;
    printf "const char release[] = \1.%s\";\n", version;
}'
