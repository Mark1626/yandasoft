#!/bin/bash

set -e

print_usage() {
  echo "Usage: $0 [options]"
  echo
  echo "Options: "
  echo "-m                             Mount yandasoft"
  echo "-b                             Build Images"
  echo "-v <version>                   Version of Image to Mount/Build"
  echo "-i <image>                     Name of the Image (for mount only)"
}

version=1.0
dockerfiledir="$PWD"/deploy/docker/custom
yandasoftdir="$PWD"
build=false
mount=false
mount_image="003-casarest"

build_base() {
  m4 $dockerfiledir/001-yandabase | docker build -t 001-yandabase:$version -
}

build_mpi() {
  m4 -Dlatest=$version $dockerfiledir/001b-mpich-yandabase | docker build -t 001b-mpich-yandabase:$version -
}

build_casa() {
  m4 -Dlatest=$version $dockerfiledir/002-casacore | docker build -t 002-casacore:$version -
  m4 -Dlatest=$version $dockerfiledir/003-casarest | docker build -t 003-casarest:$version -
}

build_slim_base() {
  m4 -Dlatest=$version $dockerfiledir/alpine-base | docker build -t alpine-base:$version -
}

mount_yandasoft() {
  docker run --rm -it -v $yandasoftdir:/home/yanda $mount_image:$version
}

if [ $# -eq 0 ]; then
  print_usage
  exit 0
fi

while getopts "bmv:i:h?" opt
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
  i)
    mount_image="$OPTARG"
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
  build_slim_base
fi

if $mount ; then
  mount_yandasoft
fi
