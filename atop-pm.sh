#!/bin/sh

case "$1" in
	pre)	/usr/bin/systemctl stop atop
		exit 0
		;;
	post)	/usr/bin/systemctl start atop
		exit 0
		;;
 	*)	exit 1
		;;
esac
