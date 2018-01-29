#cd /mnt/media/mmcblk0p1/MIJIA_RECORD_VIDEO

find ./ -name "*.mp4" > /tmp/allfile.txt
#通过程序截取文件名中的时间戳.. 1 表示截取时间戳 
./exescanfile 1 /tmp/allfile.txt /tmp/allfile_ts.txt
#排序
sort /tmp/allfile_ts.txt > /tmp/allfile_ts.sort
#通过程序转换成hex
./exescanfile 2 /tmp/allfile_ts.sort /tmp/allfile_ts.32bit
#拷贝更新时间戳文件
#cp /tmp/allfile_ts.32bit /mnt/media/mmcblk0p1/MIJIA_RECORD_VIDEO/.timestamp -rf
cp /tmp/allfile_ts.32bit ./timestamp -rf

rm /tmp/allfile.txt
rm /tmp/allfile_ts.txt
rm /tmp/allfile_ts.sort
rm /tmp/allfile_ts.32bit
#cd -
