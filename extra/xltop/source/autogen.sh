#!/bin/sh -e

if [ -f .git/hooks/pre-commit.sample -a ! -f .git/hooks/pre-commit ] ; then
        cp -p .git/hooks/pre-commit.sample .git/hooks/pre-commit && \
        chmod +x .git/hooks/pre-commit && \
        echo "Activated pre-commit hook."
fi

autoreconf --install --symlink

libdir() {
        echo $(cd $1/$(gcc -print-multi-os-directory); pwd)
}

echo "Autostuff done.  Now run \`./configure --help' for options."
