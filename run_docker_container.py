#!/usr/bin/env python3
import subprocess
import argparse
import platform

docker_image = "yandabase-mpich:latest"
country = "Australia"
city = "Sydney"

parser = argparse.ArgumentParser(
    description="Run Docker container for Yandabase",
    epilog="This is for developers of Yandasoft")
parser.add_argument('-d', '--dir', default='${PWD}', 
    help='Directory to mount in absolute path (default: current directory)')
args = parser.parse_args()

dir = args.dir

print("Running Docker container on host:", platform.system())
print("From image: " + docker_image)
print("Mounted at directory:")
subprocess.run("echo " + dir, shell=True)
print("Using this Docker command:")
command = ("docker container run --rm -it " + 
    "-v " + dir + ":/home/yanda-user/all_yandasoft " +
    "-e TZ=" + country + "/" + city + " " +
    docker_image + " /bin/bash")
subprocess.run("echo '" + command + "'", shell=True)
print()

subprocess.run(command, shell=True)