#!/bin/bash
help_csimulator() {
  echo "Usage: $ProgName simulator [options]"
  echo "Options:"
  echo "    -c Parset to use for running csimulator"
  echo ""
}

simulator_parset="csimulator.in"

run_csimulator() {
  mkdir -p logs
  echo "$SIMULATOR -c $simulator_parset > \"logs/csimulator-run-$(date +%y-%m-%d-%H-%M-%S)\""
  $SIMULATOR -c $simulator_parset > "logs/csimulator-run-$(date +%y-%m-%d-%H-%M-%S)"
}

sub_simulator() {
  print_directive

  while getopts "c:h?" opt
  do
    case "$opt" in
    [h?])
      help_csimulator
      exit 0
      ;;
    c)
      simulator_parset="$OPTARG"
      ;;
    *)
      help_csimulator
      exit 1
      ;;
    esac
  done

  run_csimulator

}