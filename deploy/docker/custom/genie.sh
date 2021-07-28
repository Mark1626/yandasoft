#!/bin/bash

set -e

print_usage() {
  echo "Usage: $0 [options]"
  echo
  echo "Options: "
  echo "-m -v <version>                Mount yandasoft"
  echo "-b -v <version>                Build Images"
}

version=1.0
workdir="$PWD"
yandasoftdir="$PWD"/../../../
build=false
mount=false

build_base() {
  docker build -t 001-yandabase:$version -f 001-yandabase .
}

build_mpi() {
  m4 -Dlatest=$version 001b-mpich-yandabase | docker build -t 001b-mpich-yandabase:$version -
}

build_casa() {
  m4 -Dlatest=$version 002-casacore | docker build -t 002-casacore:$version -
  m4 -Dlatest=$version 003-casarest | docker build -t 003-casarest:$version -
}

mount_yandasoft() {
  docker run --rm -it -v $yandasoftdir:/tmp/data 003-casarest:$version
}

if [ $# -eq 0 ]; then
  print_usage
  exit 0
fi

while getopts "bmv:h?" opt
do
  case "$opt" in
  [h?])
    print_usage
    exit 0
    ;;
  b)
    build=true
    ;;
  m)
    mount=true
    ;;
  v)
    version="$OPTARG"
    ;;
  *)
    print_usage
    exit 1
    ;;
  esac
done

if $build ; then
  build_base
  build_mpi
  build_casa
fi

if $mount ; then
  mount_yandasoft
fi
