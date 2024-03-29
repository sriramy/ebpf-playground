#! /bin/sh
##
## ebpf.sh --
##
##   Help script for the xcluster ovl/ebpf.
##
## Commands;
##

prg=$(basename $0)
dir=$(dirname $0); dir=$(readlink -f $dir)
me=$dir/$prg
tmp=/tmp/${prg}_$$
gittop=$(git rev-parse --show-toplevel)

die() {
	echo "ERROR: $*" >&2
	rm -rf $tmp
	exit 1
}
help() {
	grep '^##' $0 | cut -c3-
	rm -rf $tmp
	exit 0
}
test -n "$1" || help
echo "$1" | grep -qi "^help\|-h" && help

log() {
	echo "$prg: $*" >&2
}
dbg() {
	test -n "$__verbose" && echo "$prg: $*" >&2
}

##   env
##     Print environment.
cmd_env() {

	if test "$cmd" = "env"; then
		set | grep -E '^(__.*)='
		retrun 0
	fi

	test -n "$XCLUSTER" || die 'Not set [$XCLUSTER]'
	test -x "$XCLUSTER" || die "Not executable [$XCLUSTER]"
	eval $($XCLUSTER env)
}

##   kernel_build
##     Build local kernel
cmd_kernel_build() {
	cmd_env
	XC_PATH=$(dirname $XCLUSTER); XC_PATH=$(readlink -f $XC_PATH)
	test -n "$__tmp" || export __tmp=$tmp

	test "$__kbin" = "$XCLUSTER_HOME/bzImage" && __kbin="$__kbin-$__kver"
	mkdir -p $__kobj
	$XCLUSTER cpio_list $XC_PATH/image/initfs > $__kobj/cpio_list
	$DISKIM kernel_build --kdir=$KERNELDIR/$__kver --kernel=$__kbin \
		--kver=$__kver --kobj=$__kobj --kcfg=$__kcfg \
		--menuconfig=$__menuconfig \
		|| die "Kernel build failed [$__kver]"
	if grep -q cpio_list $__kcfg; then
		$XCLUSTER emit_cpio_list > $__kobj/cpio_list
		$DISKIM kernel_build --kdir=$KERNELDIR/$__kver --kernel=$__kbin \
			--kver=$__kver --kobj=$__kobj --kcfg=$__kcfg \
			|| die "Kernel build failed [$__kver]"
	fi
}

##   perf_build
##     Build the kernel "perf" tool
cmd_perf_build() {
	eval $($XCLUSTER env | grep -E '^KERNELDIR|__kver')
	test -n "$KERNELDIR" || die 'Not set [$KERNELDIR]'
	local kdir=$KERNELDIR/$__kver
	test -d "$kdir" || die "Not a directory [$kdir]"
	cd $kdir/tools/perf || die cd
	make || die "Make perf"
}

##
##   test --list
##   test [--xterm] [test...] > logfile
##     Exec tests
##
cmd_test() {
	if test "$__list" = "yes"; then
		grep '^test_' $me | cut -d'(' -f1 | sed -e 's,test_,,'
		return 0
	fi

	cmd_env
	start=starts
	test "$__xterm" = "yes" && start=start
	rm -f $XCLUSTER_TMP/cdrom.iso

	if test -n "$1"; then
		for t in $@; do
			test_$t
		done
	else
		for t in start; do
			test_$t
		done
	fi

	now=$(date +%s)
	tlog "Xcluster test ended. Total time $((now-begin)) sec"

}

test_start_empty() {
	export __image=$XCLUSTER_HOME/hd.img
	echo "$XOVLS" | grep -q private-reg && unset XOVLS
	test -n "$TOPOLOGY" && \
		. $($XCLUSTER ovld network-topology)/$TOPOLOGY/Envsettings
	test -n "$__nvm" || __nvm=1
	test -n "$__nrouters" || __nrouters=1
	test -n "$__ntesters" || __ntesters=1
	export xcluster_PATH="/usr/local/bin:/sbin:/usr/sbin:/bin:/usr/bin"
	# Default mem=128 doesn't seem to be enough to run with CONFIG_DEBUG_INFO_BTF=y
	export __mem=768
	export __smp1=4
	export __append1="isolcpus=2,3 nohz_full=2,3"
	# export __append="panic=-1"
	xcluster_start network-topology iptools iperf .
}

test_start() {
	tlog "Start basic ebpf test"
	test_start_empty $@
}

. $($XCLUSTER ovld test)/default/usr/lib/xctest
indent=''

# Get the command
cmd=$1
shift
grep -q "^cmd_$cmd()" $0 $hook || die "Invalid command [$cmd]"

while echo "$1" | grep -q '^--'; do
	if echo $1 | grep -q =; then
	o=$(echo "$1" | cut -d= -f1 | sed -e 's,-,_,g')
	v=$(echo "$1" | cut -d= -f2-)
	eval "$o=\"$v\""
	else
	o=$(echo "$1" | sed -e 's,-,_,g')
	eval "$o=yes"
	fi
	shift
done
unset o v
long_opts=`set | grep '^__' | cut -d= -f1`

# Execute command
trap "die Interrupted" INT TERM
cmd_$cmd "$@"
status=$?
rm -rf $tmp
exit $status
