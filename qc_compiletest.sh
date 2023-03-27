declare -a boards=("dragonboard410c"
		   "dragonboard820c"
		   "dragonboard845c"
		   "qcs404evb"
		   "starqltechn") # missing IPQ4019

do_make() {
	make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) $1
}

compile () {
	do_make mrproper
	do_make $1_defconfig
	do_make
}

set -e

for board in "${boards[@]}"; do
	compile $board
done
