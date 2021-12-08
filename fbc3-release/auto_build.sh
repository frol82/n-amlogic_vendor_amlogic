#!/bin/bash

if [ -z $1 ];then
	board="t111"
	echo $board
elif [ "$1" = "help" ];then
	echo "You can build fbc via ./auto_build.sh board_name"
	echo "board_name:"
	echo "t111"
	echo "t112"
	echo "p350"
	exit
else
	board="$1"
	echo $board
fi

export board

if [ -z $2 ];then
	build_op=""
elif [ "$2" = "clean" ];then
	build_op="clean"
elif [ "$2" = "release" ];then
	build_op="release"
fi

TEST=`which hcac`
if [ -z $TEST ]; then
  TEST=`which arc-elf32-gcc`
  if [ -z $TEST ]; then
    CC_DIR=`dirname $TEST`
    export ARC_COMPILER="GNU"
    export ARC_TOOLCHAIN_PATH=$CC_DIR
    echo "Use GNU toolchain for ARC in $ARC_TOOLCHAIN_PATH."
  else
    echo "No compiler for ARC find, please install or add PATH of hcac or aec-elf32-gcc."
  fi
else
  CC_DIR=`dirname $TEST`
  export ARC_COMPILER="METAWARE"
  export ARC_TOOLCHAIN_PATH=$CC_DIR
  echo "Use Metaware toolchain for ARC in $ARC_TOOLCHAIN_PATH."
fi

echo "Set environment variable"
export PATH=$PATH:$(echo `pwd`)

make clean
if [ -z $build_op ];then
make spi
elif [ $build_op = "release" ];then
make release
elif [ $build_op = "clean" ];then
exit
fi
