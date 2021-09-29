#!/bin/bash

COACH=$COACH_DIR/coach.sh

simulate() {

  name=$1

  $(eval "sed -n '/^[ \\t]*\[$1\]/,/\[/s/^[ \\t]*\(.*\)[ \\t]*=[ \\t]*\(.*\)/export \1=\2/p' ./profiling/simulate.cfg")

  mkdir -p profiling/simulations/$name


  m4 -DM_DATASET=$dest_path -DM_MODEL=$model -DM_ANTENNA_DEF=$antenna_def \
  -DM_FEED_DEF=$feed_def -DM_DURATION=$duration ./profiling/csimulator.in > ./profiling/simulations/$name/csimulator-$name.in

  cd ./profiling/simulations/$name
  echo "$COACH simulate -c csimulator-$name.in"
  $COACH simulator -c csimulator-$name.in
  cd ../../..
}

create_pgo_build() {
  export CPPFLAGS="-fprofile-generate=/home/user/yandasoft-profile"
  echo "$COACH build -b build-pgo -a configure"
  $COACH build -b build-pgo -a configure
  echo "$COACH build -b build-pgo -a build"
  $COACH build -b build-pgo -a build
}

use_pgo() {
  cd build-pgo
  make clean
  cd ..
  export CPPFLAGS="-fprofile-use=/home/user/yandasoft-profile -fprofile-correction"
  echo "$COACH build -b build-pgo -a configure"
  $COACH build -b build-pgo -a configure
  echo "$COACH build -b build-pgo -a build"
  $COACH build -b build-pgo -a build -- --target imager
}

image() {
  name=$1

  $(eval "sed -n '/^[ \\t]*\[$1\]/,/\[/s/^[ \\t]*\(.*\)[ \\t]*=[ \\t]*\(.*\)/export \1=\2/p' ./profiling/imager.cfg")
  mkdir -p ./profiling/imaging/$name

  m4 -DM_DATASET=$dataset \
  -DM_IMAGENAME=$imagename \
  -DM_IMAGESIZE=$imagesize \
  -DM_CELLSIZE=$cellsize \
  -DM_WORKERGROUPS=$workergroup \
  -DM_NCHAN_PER_CORE=$nchan_per_core \
  -DM_NWPLANES=$nwplanes \
  -DM_WSAMPLES=$wsamples \
  -DM_CLEAN_ITER=$clean_niter \
  ./profiling/cimager.in > ./profiling/imaging/$name/cimager-$name.in

  cd ./profiling/imaging/$name
  echo "$COACH imager -c cimager-$name.in"
  $COACH imager -c cimager-$name.in
  cd ../../..
}

image_serial() {
  name=$1

  $(eval "sed -n '/^[ \\t]*\[$1\]/,/\[/s/^[ \\t]*\(.*\)[ \\t]*=[ \\t]*\(.*\)/export \1=\2/p' ./profiling/imager.cfg")
  mkdir -p ./profiling/imaging/$name

  m4 -DM_DATASET=$dataset \
  -DM_IMAGENAME=$imagename \
  -DM_IMAGESIZE=$imagesize \
  -DM_CELLSIZE=$cellsize \
  -DM_WORKERGROUPS=$workergroup \
  -DM_NCHAN_PER_CORE=$nchan_per_core \
  -DM_NWPLANES=$nwplanes \
  -DM_WSAMPLES=$wsamples \
  -DM_CLEAN_ITER=$clean_niter \
  ./profiling/cimager.in > ./profiling/imaging/$name/cimager-$name.in

  cd ./profiling/imaging/$name
  echo "$COACH imager -s -c cimager-$name.in"
  $COACH imager -s -c cimager-$name.in
  cd ../../..
}
