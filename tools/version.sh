#!/bin/sh
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#

# Usage: version.sh [source-root]
#        version.sh --release [source-root]

# Prints the revision of the repository.
#
# The format is rNNNNNNNNNN[M]-YYMMDD
#
# The M indicates the revision has been locally modified
#
# This logic is pulled from the Linux's scripts/setlocalversion (also GPL)
# and tweaked for rockbox.
gitversion() {
    # This verifies we are in a git directory
    if head=`git -C "$1" rev-parse --verify --short=10 HEAD 2>/dev/null`; then

	# Are there uncommitted changes?
	export GIT_WORK_TREE="$1"
	if git -C "$1" diff --name-only HEAD | read dummy; then
	    mod="M"
	elif git -C "$1" diff --name-only --cached HEAD | read dummy; then
	    mod="M"
	fi

	echo "${head}${mod}"
	# All done with git
	exit
    fi
}

#
# The release tag a build belongs to, printed by "version.sh --release":
# the tag HEAD is on, otherwise the release the tree is heading to (the top
# row of the table in docs/RELEASE_NOTES.md), otherwise the nearest tag.
# Prints nothing when the tree has no tags at all.
releasetag() {
    if [ -n "$1" ]; then TOP=$1; else TOP=..; fi

    # Exactly on a release tag: that is the version.
    tag=`git -C "$TOP" describe --tags --match 'v[0-9]*' --exact-match 2>/dev/null`
    case "$tag" in
        v[0-9]*) echo "$tag"; return ;;
    esac

    # Otherwise the release the notes say is next.
    notes="$TOP/docs/RELEASE_NOTES.md"
    if [ -r "$notes" ]; then
        tag=`sed -n 's/^| *\[\(v[0-9][0-9.]*\)\].*/\1/p' "$notes" | head -1`
        if [ -n "$tag" ]; then echo "$tag"; return; fi
    fi

    git -C "$TOP" describe --tags --match 'v[0-9]*' --abbrev=0 2>/dev/null
}

if [ "$1" = "--release" ]; then
    releasetag "$2"
    exit
fi

# First locate the top of the src tree (passed in usually)
#
if [ -n "$1" ]; then TOP=$1; else TOP=..; fi

# setting VERSION var on commandline has precedence
if [ -z $VERSION ]; then
    # If the VERSIONFILE exisits we use that
    VERSIONFILE=docs/VERSION
    if [ -r $TOP/$VERSIONFILE ]; then
        VER=`cat $TOP/$VERSIONFILE`;
    else
        # Ok, we need to derive it from the Version Control system
	    VER=`gitversion $TOP`
    fi
if [ -z $SOURCE_DATE_EPOCH ]; then
	VERSION=$VER-`date -u +%y%m%d`
else
	VERSION=$VER-`date -d @$SOURCE_DATE_EPOCH -u +%y%m%d`
fi
fi
echo $VERSION

