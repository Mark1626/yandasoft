#!/bin/bash
# coach build -b "build" -a "configure" -- -DCMAKE_C_COMPILER:FILEPATH=/path/to/gcc
# coach build -b "build" -a "build" -t 16 -- --target imager
help_build() {
  echo "Usage: $ProgName build [options] "
  echo "Options:"
  echo "    -b Folder to create build"
  echo "    -a configure / build"
  echo "    -t threads to use in build step"
  echo "    -c Config file"
  echo ""
}

build=false
configure=false

# Build options
threads=4
build_dir="build"

source $coach_dir/build.cfg

run_configure() {
  cmake -B $build_dir -S $yandasoftdir -DCMAKE_BUILD_TYPE=$release -DENABLE_OPENMP=${openmp^^} \
    -DENABLE_PROFILE=${profile^^} -DENABLE_TUNING=${tuning^^} -DBUILD_TESTING=${build_testing^^} \
    -DFETCH_AND_COMPILE_DEPS=${compile_deps^^} -DBENCHMARK=${benchmark^^} $@
  # echo "cmake -B $build_dir -S $yandasoftdir -DCMAKE_BUILD_TYPE=$release -DENABLE_OPENMP=${openmp^^} \\
  #   -DENABLE_PROFILE=${profile^^} -DENABLE_TUNING=${tuning^^} -DBUILD_TESTING=${build_testing^^} \\ 
  #   -DFETCH_AND_COMPILE_DEPS=${compile_deps^^} -DBENCHMARK=${benchmark^^} $@"
}

run_build() {
  cmake --build $build_dir -j $threads $@
  # echo "cmake --build $build_dir -j $threads $@"
}

sub_build() {
  while getopts "b:a:t:c:h?" opt
  do
    case "$opt" in
    [h?])
      help_build
      exit 0
      ;;
    b)
      build_dir="$OPTARG"
      ;;
    a)
      action="$OPTARG"
      if [ "$action" == "configure" ]; then
        configure=true
      elif [ "$action" == "build" ]; then
        build=true
      else
        echo "Unknown action - Possible values are (configure/build)"
      fi
      ;;
    t)
      threads="$OPTARG"
      ;;
    c)
      "Loading args in file"
      source "$OPTARG"
      ;;
    *)
      help_build
      exit 1
      ;;
    esac
  done

  shift $(($OPTIND - 1))
  echo "Additional args passed $@"

  if $build ; then
    run_build $@
  elif $configure ; then
    run_configure $@
  else
    echo "Unknown action - Possible values are (configure/build)"
    help_build
  fi

}
