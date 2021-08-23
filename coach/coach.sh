#!/bin/bash
#
# coach build
# coach imager
# coach simulate
ProgName=$(basename $0)

sub_help() {
    echo "Usage: $ProgName <subcommand> [options]\n"
    echo "Subcommands:"
    echo "    build Build yandasoft"
    echo "    imager Run Imager"
    echo "    simulate Simulate dataset"
    echo ""
    echo "For help with each subcommand run:"
    echo "$ProgName <subcommand> -h|--help"
    echo ""
}

coach_dir=$(dirname "${BASH_SOURCE[0]}")
yandasoftdir="$(dirname "$coach_dir")"
source $coach_dir/imager.sh
source $coach_dir/simulator.sh
source $coach_dir/build.sh

IMAGER=imager
SIMULATOR=csimulator

if [[ ! -z "$COACH_CFG" ]]; then
    echo "Reading config file $COACH_CFG"
    source $COACH_CFG
fi

print_directive() {
    echo "Coach $coach_dir "
    echo "Yandasoft dir: $yandasoftdir"
    echo "Imager dir: $IMAGER"
    echo "Simulator dir: $SIMULATOR"
    echo
}

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
