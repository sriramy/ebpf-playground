#! /bin/sh

. /etc/profile

die() {
	echo "ERROR: $*" >&2
	rm -rf $tmp
	exit 1
}

prog=$1
test -n $prog || die "No program provided!"
dev=$2
test -n $dev || die "No device provided!"

cmd="$prog --dev=$dev"
log=/var/log/$prog-$dev.log
PATH="/root/$prog/:$PATH" nohup $cmd > $log 2>&1 &
xdp-loader status
