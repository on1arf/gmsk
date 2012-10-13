#! /bin/bash


while [ 1==1 ]
do
	date
	for i in *dvtool
	do
		if [[ -s  $i ]]; then
			echo "found $i"

			mv -f $i done
			nc 127.0.0.1 12345 < done/$i
		fi;
	done
	sleep 1
done

