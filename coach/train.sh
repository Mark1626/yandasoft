#!/bin/bash

coach_dir=$(dirname "${BASH_SOURCE[0]}")
COACH=./coach/coach.sh

simulate() {
  name=$1
  dest_path=$2
  model=$3
  feed_def=$4
  antenna_def=$5
  duration=$6

  mkdir -p training/$name

  m4 -DM_DATASET=$dest_path -DM_MODEL=$model -DM_ANTENNA_DEF=$antenna_def \
  -DM_FEED_DEF=$feed_def -DM_DURATION=$duration ./training/csimulator.in > ./training/$name/csimulator-$name.in

  echo "$COACH simulate -c ./training/$name/csimulator-$name.in"
  $COACH simulator -c ./training/$name/csimulator-$name.in
}

simulate_all() {
  # TODO: Migrate to cfg file
  simulate 'sim-1' './training/sim-1/1934-638_0.ms/' './training/models/1934.all.cleancomps' './training/parsets/ASKAP1feeds.in' './training/
parsets/ASKAP36.antpos.in' '0.25h'
  simulate 'sim-2' './training/sim-2/1934-638_0.ms/' './training/models/1934.all.cleancomps' './training/parsets/ASKAP1feeds.in' './training/
parsets/ASKAP36.antpos.in' '0.5h'
}

create_pgo_build() {
  export CPPFLAGS="-fprofile-generate=/tmp/yandasoft-profile"
  echo "$COACH build -b build-pgo -a configure"
  $COACH build -b build-pgo -a configure
  echo "$COACH build -b build-pgo -a build"
  $COACH build -b build-pgo -a build
}

use_pgo() {
  export CPPFLAGS="-fprofile-use=/tmp/yandasoft-profile"
  echo "$COACH build -b build-pgo -a build"
  $COACH build -b build-pgo -a build
}

image() {
  name=$1

  $(eval "sed -n '/^[ \\t]*\[$1\]/,/\[/s/^[ \\t]*\(.*\)[ \\t]*=[ \\t]*\(.*\)/export \1=\2/p' ./training/imager.cfg")
  mkdir -p ./training/imaging/$name

  m4 -DM_DATASET=$dataset \
  -DM_IMAGENAME=$names \
  -DM_IMAGESIZE=$imagesize \
  -DM_CELLSIZE=$cellsize \
  -DM_WORKERGROUPS=$workergroup \
  -DM_NCHAN_PER_CORE=$nchan_per_core \
  -DM_NWPLANES=$nwplanes \
  -DM_WSAMPLES=$wsamples \
  -DM_CLEAN_ITER=$clean_niter \
  ./training/cimager.in > ./training/imaging/$name/cimager-$name.in

  cd ./training/imaging/$name
  echo "$COACH imager -c cimager-$name.in"
  # $COACH imager -c cimager-$name.in
  cd ../../..
}

# simulate_all

# create_pgo_build

# image imaging-1
# image imaging-2

# use_pgo
