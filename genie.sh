#!/bin/bash

set -e

print_usage() {
  echo "Usage: $0 [options]"
  echo
  echo "Build Image"
  echo
  echo "Options: "
  echo "-b                             Build Images"
  echo "-v <version>                   Version of Image to Mount/Build"
  echo "-t <num-threads>               Number of threads to use (default: 4)"
  echo
  echo "Mount Image"
  echo
  echo "Options: "
  echo "-m                             Mount image on yandasoft folder"
  echo "-i <image>                     Name of the Image"
  echo "-d                             Debug Mode (enables perf)"
}

version=1.0
dockerfiledir="$PWD"/deploy/docker/custom
yandasoftdir="$PWD"
build=false
mount=false
debug=false
# mount_image="003-casarest"
mount_image="004-yandasoft-dev-base"
base_image="ubuntu:bionic"
num_threads="4"

build_base() {
  m4 -Dbaseimage=$base_image $dockerfiledir/001-yandabase | docker build -t 001-yandabase:$version -
}

build_mpi() {
  m4 -DNUM_THREADS=$num_threads -Dlatest=$version $dockerfiledir/001b-mpich-yandabase | docker build -t 001b-mpich-yandabase:$version -
}

build_casa() {
  m4 -DNUM_THREADS=$num_threads -Dlatest=$version $dockerfiledir/002-casacore | docker build -t 002-casacore:$version -
  m4 -DNUM_THREADS=$num_threads -Dlatest=$version $dockerfiledir/003-casarest | docker build -t 003-casarest:$version -
}

build_dev_base() {
  m4 -Dlatest=$version $dockerfiledir/004-yandasoft-dev-base | docker build -t 004-yandasoft-dev-base:$version -
}

mount_yandasoft() {
  if $debug ; then
    if [ -e $dockerfiledir/seccomp-perf.json ] ; then
      docker run --rm -it -v $yandasoftdir:/home/yanda --security-opt seccomp=$dockerfiledir/seccomp-perf.json $mount_image:$version
    else
      echo 'Cannot enable perf_event_open seccomp-perf.json does not exist'
      echo "Create a seccomp-perf.json in $dockerfiledir allowing perf_event_open"
      echo 'Please refer to https://docs.docker.com/engine/security/seccomp/'
      exit 1
    fi
  fi

  docker run --rm -it -v $yandasoftdir:/home/yanda $mount_image:$version
}

if [ $# -eq 0 ]; then
  print_usage
  exit 0
fi

while getopts "bmdv:i:t:h?" opt
do
  case "$opt" in
  [h?])
    print_usage
    exit 0
    ;;
  b)
    build=true
    ;;
  d)
    debug=true
    ;;
  m)
    mount=true
    ;;
  i)
    mount_image="$OPTARG"
    ;;
  t)
    num_threads="$OPTARG"
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
  build_dev_base
fi

if $mount ; then
  mount_yandasoft
fi
