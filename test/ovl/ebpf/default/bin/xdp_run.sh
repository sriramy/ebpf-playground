#! /bin/sh

. /etc/profile
export PATH="/usr/local/bin:$PATH"

die() {
	echo "ERROR: $*" >&2
	rm -rf $tmp
	exit 1
}

prog=$1
test -n $prog || die "No program provided!"
dev=$2
test -n $dev || die "No device provided!"

xdp-loader unload $dev --all
cmd="$prog --dev=$dev"
log=/var/log/$prog-$dev.log
nohup $cmd &> $log &
xdp-loader status $dev
