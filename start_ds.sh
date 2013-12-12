#!/bin/bash

cd /home/app/ds/src

if [ -e "ds_sys.pid" ]
then
	echo "ds is running already, please run stop script first!"
	exit
fi



rm -f *.pid

touch ds_sys.pid



./start_bst.sh 2> /dev/null
./start.pl ./ds 2>/dev/null
./start.pl ./calc_rate 2>/dev/null
./start.pl ./ttp 2>/dev/null
./start_lsnr
./initmmap task_list
./initmmap pid_list

echo ""
echo "starting modules..."
echo ""
sleep 2

        num=`ps -e|grep ttp|wc -l`
        if (( $num == 1 ))
        then
                echo "ttp started successfully!"
                echo ""
        else
                echo "failed to start ttp"
                exit
        fi
sleep 1
        num=`ps -e|grep ds|grep -v ds_monitor|grep -v start_ds|wc -l`
        if (( $num == 1 ))
        then
                echo "ds started successfully!"
                echo ""
        else
                echo "failed to start ds"
                exit
        fi
sleep 1
        num=`ps -e|grep bst|wc -l`
        if (( $num == 1 ))
        then
                echo "bst started successfully!"
                echo ""
        else
                echo "failed to start bst"
                exit
        fi
sleep 1
        num=`ps -e|grep calc_rate|wc -l`
        if (( $num == 1 ))
        then
                echo "cacl_rate started successfully!"
                echo ""
        else
                echo "failed to start calc_rate"
                exit
        fi
sleep 1
        num=`ps -e|grep tcpserver|wc -l`
        if (( $num == 1 ))
        then
                echo "ds listener started successfully!"
                echo ""
        else
                echo "failed to start ds_listener"
                exit
        fi

