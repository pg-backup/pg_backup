#!/usr/bin/env bash
set -xe

if [ -z ${PG_VERSION+x} ]; then
	echo PG_VERSION is not set!
	exit 1
fi
export PG_BIN_DIR=${PWD}/pg/${PG_VERSION}

#if [ -z ${PG_BRANCH+x} ]; then
#	echo PG_BRANCH is not set!
#	exit 1
#fi

#if [ -z ${PTRACK_PATCH_PG_BRANCH+x} ]; then
#	PTRACK_PATCH_PG_BRANCH=OFF
#fi

# sanitize environment
export DEBIAN_FRONTEND=noninteractive
sudo apt-get update
sudo apt-get purge -y $(dpkg -l | awk '{print$2}' | grep postgres) libpq5 libpq-dev

# install deps
#apt-get install -q -y curl ca-certificates gnupg lsb-release build-essential gcc make zlib1g-dev python3 python3-pip python3-setuptools
sudo apt-get install -q -y gnupg lsb-release build-essential gcc make gettext zlib1g-dev python3 python3-pip python3-setuptools
sudo pip3 install testgres

# Clone Postgres
echo "############### Getting Postgres sources:"
git clone https://github.com/postgres/postgres.git -b $PG_TAG --depth=1

## Clone ptrack
#if [ "$PTRACK_PATCH_PG_BRANCH" != "OFF" ]; then
#    git clone https://github.com/postgrespro/ptrack.git -b master --depth=1 postgres/contrib/ptrack
#    export PG_PROBACKUP_PTRACK=ON
#else
#    export PG_PROBACKUP_PTRACK=OFF
#fi

# Compile and install Postgres
echo "############### Compiling Postgres:"
cd postgres # Go to postgres dir
#if [ "$PG_PROBACKUP_PTRACK" = "ON" ]; then
#    git apply -3 contrib/ptrack/patches/${PTRACK_PATCH_PG_BRANCH}-ptrack-core.diff
#fi
CFLAGS="-Og" ./configure --prefix=$PG_BIN_DIR \
    --enable-debug --enable-cassert --enable-depend \
    --without-readline --enable-nls --with-python
make -s -j$(nproc) install
make -s -j$(nproc) -C contrib/ install

# Override default Postgres instance
#export PATH=$PG_BIN_DIR/bin:$PATH
export LD_LIBRARY_PATH=$PG_BIN_DIR/lib
export PG_CONFIG=$PG_BIN_DIR/bin/pg_config

#if [ "$PG_PROBACKUP_PTRACK" = "ON" ]; then
#    echo "############### Compiling Ptrack:"
#    make -C contrib/ptrack install
#fi

# Get amcheck if missing
if [ ! -d "contrib/amcheck" ]; then
    echo "############### Getting missing amcheck:"
    git clone https://github.com/petergeoghegan/amcheck.git --depth=1 contrib/amcheck
    make -C contrib/amcheck install
fi

# Build and install pg_probackup (using PG_CPPFLAGS and SHLIB_LINK for gcov)
echo "############### Compiling and installing pg_probackup:"
export PG_SRC=$PWD
export PATH=$PG_BIN_DIR/bin:$PATH
cd ..
# make USE_PGXS=1 PG_CPPFLAGS="-coverage" SHLIB_LINK="-coverage" top_srcdir=$CUSTOM_PG_SRC install
make USE_PGXS=1 top_srcdir=$PG_SRC
#make USE_PGXS=1 top_srcdir=$PG_SRC install
