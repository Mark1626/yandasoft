#!/bin/bash
# coach simulate
help_imager() {
  echo "Usage: $ProgName imager [options]\n"
  echo "Options:"
  echo "    -n Number of processes to run on"
  echo "    -c Parset to use for running imager"
  echo ""
}

processes=4
imager_parset="cimager.in"

run_imager() {
  mkdir -p logs
  mpirun -n $processes $IMAGER -c $imager_parset > "logs/imager-run-$(date +%y-%m-%d-%H-%M-%S)"
  # echo "mpirun -n $processes $IMAGER -c $imager_parset > \"logs/imager-run-$(date +%y-%m-%d-%H-%M-%S)\""
}

sub_imager() {
  while getopts "c:n:h?" opt
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
    *)
      help_imager
      exit 1
      ;;
    esac
  done

  run_imager

}