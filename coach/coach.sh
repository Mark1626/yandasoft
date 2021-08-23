#!/bin/bash
#
# coach imager
# coach simulate
ProgName=$(basename $0)

sub_help() {
    echo "Usage: $ProgName <subcommand> [options]\n"
    echo "Subcommands:"
    echo "    imager Run Imager"
    echo "    simulate Simulate dataset"
    echo ""
    echo "For help with each subcommand run:"
    echo "$ProgName <subcommand> -h|--help"
    echo ""
}

coach_dir=$(dirname "${BASH_SOURCE[0]}")
source $coach_dir/imager.sh
source $coach_dir/simulator.sh

IMAGER=imager
SIMULATOR=csimulator

subcommand=$1
case $subcommand in
    "" | "-h" | "--help")
        sub_help
        ;;
    *)
        shift
        sub_${subcommand} $@
        if [ $? = 127 ]; then
            echo "Error: '$subcommand' is not a known subcommand." >&2
            echo "       Run '$ProgName --help' for a list of known subcommands." >&2
            exit 1
        fi
        ;;
esac
