#!/bin/bash

#sleep 30

cd /home/app/ds/src/
./start.pl ./ds
#sleep 5
./start_lsnr
./start.pl ./calc_rate
./start.pl ./ttp

