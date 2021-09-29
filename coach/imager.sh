#!/bin/bash
# coach simulate
help_imager() {
  echo "Usage: $ProgName imager [options]"
  echo "Options:"
  echo "    -n Number of processes to run on"
  echo "    -c Parset to use for running imager"
  echo "    -s Run serial without MPI"
}

processes=4
imager_parset="cimager.in"
serial=false

run_imager_parallel() {
  mkdir -p logs
  echo "mpirun -n $processes $IMAGER -c $imager_parset > \"logs/imager-run-$(date +%y-%m-%d-%H-%M-%S)\""
  mpirun -n $processes $IMAGER -c $imager_parset > "logs/imager-run-$(date +%y-%m-%d-%H-%M-%S)"
}

run_imager_serial() {
  mkdir -p logs
  echo "$IMAGER -c $imager_parset > \"logs/imager-run-$(date +%y-%m-%d-%H-%M-%S)\""
  $IMAGER -c $imager_parset > "logs/imager-run-$(date +%y-%m-%d-%H-%M-%S)"
}

sub_imager() {
  print_directive

  while getopts "c:n:sh?" opt
  do
    case "$opt" in
    [h?])
      help_imager
      exit 0
      ;;
    c)
      imager_parset="$OPTARG"
      ;;
    n)
      processes="$OPTARG"
      ;;
    s)
      serial=true
      ;;
    *)
      help_imager
      exit 1
      ;;
    esac
  done

  if $serial ; then
    run_imager_serial
  else
    run_imager_parallel
  fi

}