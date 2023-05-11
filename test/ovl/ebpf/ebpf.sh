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

get_kdir() {
	eval $($XCLUSTER env | grep -E '^KERNELDIR|__kver')
	test -n "$KERNELDIR" || die 'Not set [$KERNELDIR]'
	local kdir=$KERNELDIR/$__kver
	test -d "$kdir" || die "Not a directory [$kdir]"
	echo $kdir
}

##   perf_build
##     Build the kernel "perf" tool
cmd_perf_build() {
	local kdir=$(get_kdir)
	cd $kdir/tools/perf || die cd
	make || die "Make perf"
}

##   libbpf_build
##     Build libbpf from submodule
cmd_libbpf_build() {
	cd $(get_kdir)/tools/lib/bpf || die cd
	export BUILD_STATIC_ONLY=1
	make clean
	make -j$(nproc) || die "Make libbpf"
	make DESTDIR=root prefix=/usr install || die "Make libbpf install"
	make DESTDIR=$XCLUSTER_WORKSPACE/sys prefix=/usr install \
		|| die "Make libbpf install sys"
}

##   bpftool_build
##     Build bpftool from submodule
cmd_bpftool_build() {
	cd $(get_kdir)/tools/bpf/bpftool
	make -j$(nproc) || die "Make bpftool"
}


##   libxdp_build
##     Build static libxdp. Requires libbpf
cmd_libxdp_build() {
	local d=$GOPATH/src/github.com/xdp-project/xdp-tools
	test -d $d || die "Not a directory [$d]"
	cd $d
	# Dynamic libs doesn't work since libbpf.a is not built with -fPIC
	export BUILD_STATIC_ONLY=1
	export BPFTOOL=$(get_kdir)/tools/bpf/bpftool/bpftool
	export LIBBPF_DIR=$(get_kdir)/tools/lib/bpf
	export LIBBPF_INCLUDE_DIR=$XCLUSTER_WORKSPACE/sys/usr/include
	export LIBBPF_LIB_DIR=$XCLUSTER_WORKSPACE/sys/usr/lib64
        export LIBBPF_CFLAGS="-I${LIBBPF_INCLUDE_DIR} -L${LIBBPF_LIB_DIR}"
        export LIBBPF_LDLIBS="-lbpf"
	export DEBUG_CONFIGURE=1
	./configure
	make -j$(nproc) || die make
	make DESTDIR=$XCLUSTER_WORKSPACE/sys install || die "make install"
}

##   bpfexamples_build
##     Build bpf examples in xdp-project. Requires libxdp, libbpf
cmd_bpfexamples_build() {
	local d=$GOPATH/src/github.com/xdp-project/bpf-examples
	test -d $d || die "Not a directory [$d]"
	cd $d
	# Dynamic libs doesn't work since libbpf.a is not built with -fPIC
	export BUILD_STATIC_ONLY=1
	export BPFTOOL=$(get_kdir)/tools/bpf/bpftool/bpftool
	./configure
	make -j$(nproc) || die make
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
	test -n "$__nvm" || __nvm=2
	test -n "$__nrouters" || __nrouters=1
	xcluster_start network-topology iptools sctp .
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
