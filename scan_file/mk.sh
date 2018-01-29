arm-linux-gcc scanfile.c -o exescanfile 
arm-linux-strip exescanfile
#tar -zcvf exescanfile.tar.gz exescanfile scanfile.sh
tar -zcvf exescanfile.tar.gz exescanfile 
cp exescanfile.tar.gz /home/share/bcg/
