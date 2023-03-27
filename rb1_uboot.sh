make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) mrproper && \
make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) rb1_defconfig && \
make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) && \
cd board/qualcomm/rb1 && \
mkimage -f rb1.its rb1.itb && \
cd ../../../ && \
gzip -f u-boot.bin && \
cat u-boot.bin.gz arch/arm/dts/qrb2210-rb1.dtb > u-boot.bin-dtb && \
mkbootimg \
	--kernel u-boot.bin-dtb \
	--ramdisk board/qualcomm/rb1/rb1.itb \
	--pagesize 4096 \
	--base 0x8000 \
	-o boot.img && \
fastboot --set-active=a && fastboot boot boot.img
