#!/bin/bash


###stop ds first

if [ -e "/home/app/ds/src/ds_sys.pid" ]
then
        echo "ds is running already, please run stop script first!"
        exit
fi


###rm ds

echo "removing ds system......"
sleep 2

rm -rf /home/app/ds
rm /root/start_ds
rm /root/stop_ds

###delete stuff in /etc/rc.local
cat /etc/rc.local |grep -v "remove pid files" > /tmp/.rc.local.tmp
cat /tmp/.rc.local.tmp > /etc/rc.local

cat /etc/rc.local |grep -v "ds start script" > /tmp/.rc.local.tmp
cat /tmp/.rc.local.tmp > /etc/rc.local

rm /tmp/.rc.local.tmp



