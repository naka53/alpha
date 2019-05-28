export ARCH = arm64
export CROSS_COMPILE = /home/users/optee/TZ/toolchains/aarch64/bin/aarch64-linux-gnu-

obj-m += alpha.o

all:
	make -C /home/users/optee/TZ/linux M=$(PWD) modules

clean:
	make -C /home/users/optee/TZ/linux M=$(PWD) clean
	@rm -f *~
