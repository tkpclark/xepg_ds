#!/bin/bash
#install_script for ds

echo "installing DS system......"
sleep 2

app_path=/home/app
ds_path=$app_path/ds

if [ -e $ds_path ]
then
        echo "ds already exists!"
        exit
fi


echo "rm /home/app/ds/src/ds_sys.pid ##remove pid files" >> /etc/rc.local
echo "/root/start_ds  ##ds start script" >> /etc/rc.local


rm *.pid

mkdir $ds_path
cp -r ../src $ds_path

cd $ds_path
mkdir log mmap taskts tmp_pidts pidts
touch log/ds.log

cd src
make
chmod +x start.pl start_lsnr
./initmmap ts_buffer
./initmmap task_list
./initmmap pid_list

ln -s /home/app/ds/src/stop_ds.sh /root/stop_ds
ln -s /home/app/ds/src/start_ds.sh /root/start_ds
