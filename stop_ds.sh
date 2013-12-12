#!/bin/bash

cd /home/app/ds/src

if [ ! -e "ds_sys.pid" ]
then 
	echo "ds is not running!"
	echo ""
	exit
fi

	echo "stopping modules..."
	echo ""
	
	
	kill `cat bst.pid  `
	kill `cat calc_rate.pid  `
	kill `cat ds.pid  `
	kill `cat ttp.pid`

	killall tcpserver 2> /dev/null
	killall DST_cmd_recv 2> /dev/null
	
	sleep 1

	rm ../taskts/* 2> /dev/null
	rm ../pidts/* 2> /dev/null
	sleep 3

	num=`ps -e|grep ttp|wc -l`
	if (( $num == 0 ))
	then
		echo "ttp stopped successfully!"
		echo ""
	else
		echo "failed to stop ttp"
		exit
	fi

	num=`ps -e|grep ds|grep -v ds_monitor|grep -v stop_ds|wc -l`
	if (( $num == 0 ))
	then
		echo "ds stopped successfully!"
		echo ""
	else
		echo "failed to stop ds"
		exit
	fi

	num=`ps -e|grep bst|wc -l`
	if (( $num == 0 ))
	then
		echo "bst stopped successfully!"
		echo ""
	else
		echo "failed to stop bst"
		exit
	fi
	
	num=`ps -e|grep calc_rate|wc -l`
	if (( $num == 0 ))
	then
		echo "cacl_rate stopped successfully!"
		echo ""
	else
		echo "failed to stop calc_rate"
		exit
	fi
	
	num=`ps -e|grep tcpserver|wc -l`
	if (( $num == 0 ))
	then
		echo "ds listener stopped successfully!"
		echo ""
	else
		echo "failed to stop ds_listener"
		exit
	fi

	

./initmmap task_list > /dev/null
./initmmap pid_list > /dev/null

rm ds_sys.pid
