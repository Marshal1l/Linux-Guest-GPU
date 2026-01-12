make CROSS_COMPILE=aarch64-linux-gnu- ARCH=arm64 Image -j16
cp -v ~/cca/gpu/Linux-Guest-GPU/arch/arm64/boot/Image ~/cca/gpu/GPU_SFTP/firecracker-bins/
