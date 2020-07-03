#!/usr/bin/env python3
#
# Create Docker images for all cases as specified in the settings below.
# Possible targets are:
# - Specific machine (currently only Galaxy, with Cray MPICH)
# - Generic machine, with these MPI implementations
#   - MPICH
#   - OpenMPI of various versions
#
# Note that the images are split into base image (for components that are 
# seldom changed) and the final image (Yandasoft).
# This is done for deployment efficiency.
#
# Usage:
# 1) Make sure that the targets are correct (see SETTINGS section below).
# 2) Execute the script.
#    If you want only the final images (since base image is still the same):
#    ./make_docker_image.py -f
#
#    If you want to make both base and final images:
#    ./make_docker_image.py -bf
#
#    If you want no image, just Dockerfiles, for example for dry run: 
#    ./make_docker_image.py
#
# Author: Paulus Lahur <paulus.lahur@csiro.au>
# Copyright: CSIRO 2020
#
#------------------------------------------------------------------------------
# USER SETTINGS
#
# Set machine targets in the list below.
# Currently, we target generic HPCs and a specific HPC: Galaxy.
# When a specific machine target is chosen, MPI target is ignored.
# Choose one or both of this list of target.
# machine_targets = ["generic", "galaxy"]
machine_targets = ["generic"]

# Set MPI implementations for generic machine in the list below.
# Note that a specific machine requires no MPI specification.
# The format of the MPI specification is:
#     mpi_type[-X.Y.Z]
# where
# - mpi_type is either "mpich" or "openmpi".
# - X, Y and Z are version numbers (major, minor and revision).
# When the numbers are specified, MPI library will be built from source code.
# When they are not, the default version from the base OS will be installed
# using the simplest method (apt-get install).
# Choose a subset (or all) of this complete list of targets:
# mpi_targets = ["mpich", "mpich-3.3.2", "openmpi", "openmpi-4.0.2", "openmpi-3.1.4", "openmpi-2.1.6", "openmpi-1.10.7"]
# mpi_targets = ["mpich", "mpich-3.3.2", "openmpi", "openmpi-4.0.4", "openmpi-3.1.6", "openmpi-2.1.6", "openmpi-1.10.7"]
mpi_targets = ["mpich"]

git_branch = "develop"
# git_branch = "master"

casacore_ver = "3.3.0"

#------------------------------------------------------------------------------
# TODO: Add logging
# TODO: Add timing
# TODO: Add error handling, as this is going to be used within CI/CD
# Consider checking whether all files to be downloaded actually exist!
# TODO: Slim down the image. Some dev stuff can be removed from final image.
# THINK: Why is native app much faster?

import sys
import argparse
import subprocess
import re
import os
from pathlib import Path

nproc_available = os.cpu_count()
nproc = 1
if nproc_available > 1:
    nproc = nproc_available - 1
print("nproc:", nproc)

# Git repository of Yandasoft. 
# No longer needed, as this is set directly downstream now.
git_repository = "https://github.com/ATNF/yandasoft.git"

# Header for all automatically generated Dockerfiles
header = ("# This file is automatically created by " + __file__ + "\n")

# MPI wrapper for g++
cmake_cxx_compiler = "-DCMAKE_CXX_COMPILER=mpicxx"
# cmake_cxx_compiler = "-DCMAKE_CXX_COMPILER=/usr/local/bin/mpiCC"

mpi_dir = "/usr/local"
MPI_COMPILE_FLAGS = "-I/usr/local/include -pthread"

forbidden_chars_string = "?!@#$%^&* ;<>?|\"\a\b\f\n\r\t\v"
forbidden_chars = list(forbidden_chars_string)
# print(forbidden_chars)

# Sanitizing parameters
machine_targets = list(map(str.lower, machine_targets))
mpi_targets = list(map(str.lower, mpi_targets))


def is_proper_name(name):
    '''
    Return true if the name is non-empty and does not contain certain characters. 
    False otherwise.
    '''
    if type(name) != str:
        raise TypeError("Name is not string")
    if name == "":
        return False
    for c in forbidden_chars:
        if name.find(c) >= 0:
            return False
    return True


class DockerClass:
    recipe_name = ""
    image_name = ""
    recipe = ""

    def set_recipe_name(self, recipe_name):
        '''Set Dockerfile name'''
        if is_proper_name(recipe_name):
            self.recipe_name = recipe_name
        else:
            raise ValueError("Illegal recipe_name:", recipe_name)

    def set_recipe(self, recipe):
        '''Set the content of Dockerfile'''
        if type(recipe) == str:
            if recipe != "":
                self.recipe = recipe
            else:
                raise ValueError("Recipe is empty string")
        else:
            raise TypeError("Recipe is not string")

    def set_image_name(self, image_name):
        '''Set Docker image name'''
        if is_proper_name(image_name):
            self.image_name = image_name
        else:
            raise ValueError("Illegal image_name:", image_name)

    def write_recipe(self):
        '''Write recipe into Dockerfile'''
        if self.recipe_name == "":
            raise ValueError("Docker recipe file name has not been set")
        elif self.recipe == "":
            raise ValueError("Docker recipe content has not been set")
        else:
            with open(self.recipe_name, "w") as file:
                file.write(self.recipe)

    def get_build_command(self):
        '''Return build command'''
        if (self.recipe_name == ""):
            raise ValueError("Docker recipe file name has not been set")
        elif (self.image_name == ""):
            raise ValueError("Docker image file name has not been set")
        else:
            return ("docker build -t " + self.image_name + " -f " + self.recipe_name + " .")
         
    def build_image(self):
        '''Build the Docker image'''
        build_command = self.get_build_command()
        if (self.recipe_name == ""):
            raise ValueError("Docker recipe file name has not been set")
        else:
            file = Path(self.recipe_name)
            if file.is_file():
                # TODO: store log file, handle error
                subprocess.run(build_command, shell=True)
            else:
                raise FileExistsError("Docker recipe file does not exist:", self.recipe_name)



def split_version_number(input_ver):
    '''
    Split a given version number in string into 3 integers.
    '''
    string_list = re.findall(r'\d+', input_ver)
    if (len(string_list) == 3):
        int_list = [int(x) for x in string_list]
        return int_list
    else:
        return []


def compose_version_number(int_list):
    '''
    Given a list of 3 integers, compose version number.
    '''
    if (len(int_list) == 3):
        return (str(int_list[0]) + '.' + str(int_list[1]) + '.' + str(int_list[2]))
    else:
        return ""


def get_mpi_type_and_version(mpi_name):
    '''
    Given the full name of MPI, return MPI type (mpich / openmpi)
    as well as version.
    When the version is not specified, the simplest version to install 
    is chosen (ie. using "apt-get install").
    Input should be in one of these formats:
    - mpich
    - openmpi
    - mpich-X.Y.Z
    - openmpi-X.Y.Z
    Where "X.Y.Z" is version number.
    '''
    length = len(mpi_name)
    if (type(mpi_name) == str):
        if (length < 5):
            raise ValueError("MPI name is too short:", mpi_name)

        elif (length == 5):
            # Unspecified MPICH
            if (mpi_name == "mpich"):
                return("mpich", "")
            else:
                raise ValueError("Expecting mpich:", mpi_name)

        elif (length == 6):
            raise ValueError("Illegal MPI name:", mpi_name)

        elif (length == 7):
            # Unspecified OpenMPI
            if (mpi_name == "openmpi"):
                return("openmpi", "")
            else:
                raise ValueError("Expecting openmpi:", mpi_name)

        else:
            # Length > 7
            if (mpi_name[0:5] == "mpich"):
                # MPICH with specified version number
                int_ver = split_version_number(mpi_name[6:])
                if (len(int_ver) == 3):
                    return ("mpich", compose_version_number(int_ver))
                else:
                    raise ValueError("Illegal mpich version:", mpi_name[6:])

            elif (mpi_name[0:7] == "openmpi"):
                # OpenMPI with specified version number
                int_ver = split_version_number(mpi_name[8:])
                if (len(int_ver) == 3):
                    return ("openmpi", compose_version_number(int_ver))
                else:
                    raise ValueError("Illegal openmpi version:", mpi_name[8:])
            else:
                raise ValueError("Illegal MPI name:", mpi_name)
    else:
        raise TypeError("MPI name is not a string:", mpi_name)



def make_base_image(machine, mpi, prepend, append, actual):
    '''
    Make base image for components that are seldom changed:
    base OS, upgrades, standard libraries and apps, MPI, Casacore and Casarest.
    '''
    docker_target = DockerClass()

    # First, make Dockerfile, which is composed of:
    # - Common header
    # - Base system part
    # - Common top part
    # - MPI part
    # - Common bottom part

    # Construct common top part

    apt_install_part = (
    "ENV DEBIAN_FRONTEND=\"noninteractive\"\n"
    "RUN apt-get update \\\n"
    "    && apt-get upgrade -y \\\n"
    "    && apt-get autoremove -y \\\n"
    "    && apt-get install -y"
    )

    # Applications to install, as packaged in the base system.
    # Note that most have the default versions as set by the base system.
    # TODO: Remove stuff that is not needed
    apt_install_items = [
    "g++",
    "gfortran",
    "m4",
    "autoconf",
    "automake",
    "libtool",      
    "flex",
    "bison",
    "make",
    "libncurses5-dev",
    "libreadline-dev",
    "libopenblas-dev",        
    "liblapacke-dev",
    "libcfitsio-dev",
    "wcslib-dev",
    "libhdf5-serial-dev", 
    "libfftw3-dev",
    "libpython-dev", 
    "python-pip",          
    "python-numpy",
    "python-scipy",
    "libboost-python-dev", 
    "libboost-dev",   
    "libboost-filesystem-dev", 
    "libboost-program-options-dev", 
    "libboost-signals-dev",
    "libboost-system-dev",  
    "libboost-thread-dev",   
    "libboost-regex-dev",  
    "libcppunit-dev",  
    "git",
    "libffi-dev",     
    "libgsl-dev",        
    "liblog4cxx-dev", 
    "patch",           
    "subversion",          
    "wget",     
    "docker",       
    "libxerces-c-dev",
    "libcurl4-openssl-dev",
    "xsltproc",
    "gcovr",
    "zeroc-ice-all-dev",
    "zeroc-ice-all-runtime",
    "libczmq-dev"]

    for apt_install_item in apt_install_items:
        apt_install_part += " \\\n" + "        " + apt_install_item
    apt_install_part += " \n"
    # apt_install_part += "    && rm -rf /var/lib/apt\n"

    cmake_ver = "3.17.3"
    cmake_source = "cmake-" + cmake_ver + ".tar.gz"

    cmake_cxx_flags = "-DCMAKE_CXX_FLAGS=\"" + MPI_COMPILE_FLAGS + "\" -DCMAKE_BUILD_TYPE=Release"

    common_top_part = (
    apt_install_part +
    "# Build the latest cmake\n"
    "RUN mkdir /usr/local/share/cmake\n"
    "WORKDIR /usr/local/share/cmake\n"
    "RUN wget https://github.com/Kitware/CMake/releases/download/v" + cmake_ver + "/" + cmake_source + " \\\n"
    "    && tar -zxf " + cmake_source + " \\\n"
    "    && rm " + cmake_source + "\n"
    "WORKDIR /usr/local/share/cmake/cmake-" + cmake_ver + "\n"
    "RUN ./bootstrap --system-curl \\\n"
    "    && make \\\n"
    "    && make install\n"
    )

    common_bottom_part = (
    "# Build the latest measures\n"
    "RUN mkdir /usr/local/share/casacore \\\n"
    "    && mkdir /usr/local/share/casacore/data\n"
    "WORKDIR /usr/local/share/casacore/data\n"
    "RUN wget ftp://ftp.astron.nl/outgoing/Measures/WSRT_Measures.ztar \\\n"
    "    && mv WSRT_Measures.ztar WSRT_Measures.tar.gz \\\n"
    "    && tar -zxf WSRT_Measures.tar.gz \\\n"
    "    && rm WSRT_Measures.tar.gz \\\n"
    "    && mkdir /var/lib/jenkins \\\n"
    "    && mkdir /var/lib/jenkins/workspace \n"
    "# Build casacore\n"
    "WORKDIR /usr/local/share/casacore\n"
    "RUN wget https://github.com/casacore/casacore/archive/v" + casacore_ver + ".tar.gz \\\n"
    "    && tar -xzf v" + casacore_ver + ".tar.gz\\\n"
    "    && rm v" + casacore_ver + ".tar.gz\n"
    "WORKDIR /usr/local/share/casacore/casacore-" + casacore_ver + "\n"
    "RUN mkdir build\n"
    "WORKDIR build\n"
    "RUN cmake " + cmake_cxx_compiler + " -DCMAKE_BUILD_TYPE=Release .. \\\n"
    "    && make -j" + str(nproc) + " \\\n"
    "    && make install\n"
    "WORKDIR /usr/local/share/casacore/\n"
    "RUN wget https://github.com/steve-ord/casarest/tarball/078f94e \\\n"
    "    && tar -xvzf 078f94e \\\n"
    "    && rm 078f94e\n"
    "WORKDIR steve-ord-casarest-078f94e\n"
    "RUN mkdir build\n"
    "WORKDIR build\n"
    "RUN cmake " + cmake_cxx_compiler + " -DCMAKE_BUILD_TYPE=Release .. \\\n"
    "    && make -j" + str(nproc) + " \\\n"
    "    && make install \n"
    "WORKDIR /usr/local/share/casacore\n"
    "RUN rm -rf casacore \\\n"
    "    && rm -rf steve-ord-casarest-078f94e \\\n"
    "    && apt-get clean \n"
    "# Build LOFAR\n"
    "WORKDIR /usr/local/share\n"
    "RUN mkdir LOFAR\n"
    "WORKDIR /usr/local/share/LOFAR\n"
    "RUN git clone https://bitbucket.csiro.au/scm/askapsdp/lofar-common.git\n"
    "WORKDIR /usr/local/share/LOFAR/lofar-common\n"
    # "RUN git checkout " + git_branch + "\n"
    "RUN mkdir build\n"
    "WORKDIR /usr/local/share/LOFAR/lofar-common/build\n"
    "RUN cmake " + cmake_cxx_compiler + " " + cmake_cxx_flags + " .. \\\n"
    "    && make -j" + str(nproc) + " \\\n"
    "    && make install\n"
    "WORKDIR /usr/local/share/LOFAR\n"
    "RUN git clone https://bitbucket.csiro.au/scm/askapsdp/lofar-blob.git\n"
    "WORKDIR /usr/local/share/LOFAR/lofar-blob\n"
    # "RUN git checkout " + git_branch + "\n"
    "RUN mkdir build\n"
    "WORKDIR /usr/local/share/LOFAR/lofar-blob/build\n"
    "RUN cmake " + cmake_cxx_compiler + " " + cmake_cxx_flags + " .. \\\n"
    "    && make -j" + str(nproc) + " \\\n"
    "    && make install\n"
    "# Set environment variables\n"
    "ENV LD_LIBRARY_PATH=/home/install/lib:/usr/local/lib \n"
    "ENV PATH=/home/install/bin:$PATH \n"
    "# Put start-up message in .bashrc\n"
    "RUN echo \"echo \" >> ~/.bashrc \n"
    "RUN echo \"echo ================================================================================\" >> ~/.bashrc \n"
    "RUN echo \"echo Welcome to Yandabase container for developers! \" >> ~/.bashrc \n"
    "RUN echo \"echo Version x.y.z \" >> ~/.bashrc \n"
    "RUN echo \"echo More information: \" >> ~/.bashrc \n"
    "RUN echo \"echo https://confluence.csiro.au/display/ASDP/Containers+for+end+users+and+developers \" >> ~/.bashrc \n"
    "RUN echo \"echo ================================================================================\" >> ~/.bashrc \n"
    "# Protect user from accidental git execution\n"
    "RUN echo \"alias git=\'echo DO NOT USE GIT INSIDE CONTAINER!\'\" >> ~/.bashrc \n"
    # TODO: Switch to normal user here
    "WORKDIR /home/\n"
    )

    # Construct MPI part
    mpi_part = ""
    if machine == "generic":
        base_system_part = ("FROM ubuntu:bionic as buildenv\n")
        (mpi_type, mpi_ver) = get_mpi_type_and_version(mpi)

        if (mpi_type == "mpich"):
            if (mpi_ver == ""):
                # if MPICH version is not specified, get the precompiled version
                mpi_part = (
                "RUN apt-get install -y libmpich-dev \n"
                # "    && rm -rf /var/lib/apt\n"
                )

            else:
                # else (if version is specified), download the source from website and build           
                web_dir = "https://www.mpich.org/static/downloads/" + mpi_ver

                # TODO: Check whether the version is correct and the file exists

                mpi_part = (
                "# Build MPICH\n"
                "WORKDIR /home\n"
                "RUN wget " + web_dir + "/" + mpi + ".tar.gz \\\n"
                "    && tar -zxf " + mpi + ".tar.gz\n"
                "    && rm " + mpi + ".tar.gz \n"
                "WORKDIR /home/" + mpi + "\n"
                "RUN ./configure --prefix=" + mpi_dir + " \\\n"
                "    && make -j" + str(nproc) + " \\\n"
                "    && make install \n"
                "ENV PATH=$PATH:" + mpi_dir + "/bin\n"
                "ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:" + mpi_dir + "/lib\n"
                # "ENV MPI_INCLUDE_PATH=" + mpi_dir + "/include/mpich\n"
                )

        elif (mpi_type == "openmpi"):
            if (mpi_ver == ""):
                # if OpenMPI version is not specified, get the precompiled version
                mpi_part = (
                "RUN apt-get install -y libopenmpi-dev \n"
                # "    && rm -rf /var/lib/apt\n"
                )

            else:
                # Download the source from OpenMPI website and build
                # TODO: Check whether the version number is correct
                # TODO: Make this works for the case where version number is of generic format!
                #       Convert from string to a list of 3 integers

                int_ver = split_version_number(mpi_ver)
                ver_dir = "v" + str(int_ver[0]) + "." + str(int_ver[1])
                web_dir = "https://download.open-mpi.org/release/open-mpi/" + ver_dir

                # Note: Enable C++ binding when configuring, because some programs use it.
                # ./configure --enable-mpi-cxx

                mpi_part = (
                "# Build OpenMPI\n"
                "WORKDIR /home\n"
                "RUN wget " + web_dir + "/" + mpi + ".tar.gz \\\n"
                "    && tar -zxf " + mpi + ".tar.gz \\\n"
                "    && rm " + mpi + ".tar.gz \n"
                "WORKDIR /home/" + mpi + "\n"
                "RUN ./configure --enable-mpi-cxx \\\n"
                "    && make all -j" + str(nproc) + " \\\n"
                "    && make install\n"
                "ENV PATH=/usr/local/bin:$PATH\n"
                "ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH\n"
                "ENV MPI_INCLUDE_PATH=\"/usr/local/include\"\n"
                "ENV MPI_LIBRARIES=\"/usr/local/lib\"\n"
                "ENV MPI_COMPILE_FLAGS=\"-I/usr/local/include -pthread\"\n" 
                )

        else:
            raise ValueError("Unknown MPI target:", mpi)

        docker_target.set_recipe_name("Dockerfile-yandabase-" + mpi)
        docker_target.set_image_name(prepend + mpi + append)

    elif (machine == "galaxy"):
        # Galaxy (of Pawsey) has Docker image with its MPICH implementation already baked into 
        # an Ubuntu base.
        base_system_part = ("FROM pawsey/mpi-base:latest as buildenv\n")
        docker_target.set_recipe_name("Dockerfile-yandabase-" + machine)
        docker_target.set_image_name(prepend + machine + append)

    else:
        raise ValueError("Unknown machine target:", machine)

    docker_target.set_recipe(header + base_system_part + common_top_part + mpi_part + common_bottom_part)
    docker_target.write_recipe()

    # If requested, actually generate the image
    if actual:
        docker_target.build_image()
    else:  # Otherwise, just echo the command to generate the image
        print(docker_target.get_build_command())

    return docker_target



def make_final_image(machine, mpi, prepend, append, base_image, actual):
    '''
    Make the final image on top of base image.
    '''

    cmake_cxx_flags = "-DCMAKE_CXX_FLAGS=\"" + MPI_COMPILE_FLAGS + "\" -DCMAKE_BUILD_TYPE=Debug "
    # cmake_build_flags = "-DBUILD_ANALYSIS=OFF -DBUILD_PIPELINE=OFF -DBUILD_COMPONENTS=OFF -DBUILD_SERVICES=OFF"
    cmake_build_flags = (
    "-DIce_HOME=/usr/lib/x86_64-linux-gnu/ "
    "-DBUILD_ANALYSIS=ON "
    "-DBUILD_PIPELINE=ON "
    "-DBUILD_COMPONENTS=ON "
    "-DBUILD_ANALYSIS=ON "
    "-DBUILD_SERVICES=OFF "
    "-DCMAKE_CXX_FLAGS=\"-coverage\" -DCMAKE_EXE_LINKER_FLAGS=\"-coverage\" "
    "-DCMAKE_INSTALL_PREFIX=/builds/ASKAPSDP/install"
    )

    common_part = (
    "# Follow the steps here to build manually\n"
    "FROM " + base_image + " as buildenv\n"
    "# Remove the line that prevents git from working\n"
    "RUN head -n -1 ~/.bashrc > temp.txt ; mv temp.txt ~/.bashrc\n"
    "# Build yandasoft\n"
    "WORKDIR /home\n"
    "RUN git clone https://github.com/ATNF/all_yandasoft.git\n"
    "WORKDIR /home/all_yandasoft\n"
    "RUN ./git-do clone\n"
    "RUN ./git-do checkout -b " + git_branch + "\n"
    "RUN mkdir build\n"
    "WORKDIR /home/all_yandasoft/build\n"
    "RUN cmake " + cmake_cxx_compiler + " " + cmake_cxx_flags + " " + cmake_build_flags + " .. \\\n"
    "    && make -j" + str(nproc) + " \\\n"
    "    && make install\n"
    "# Remove dev tools\n"
    "RUN apt-get remove -y git \\\n"
    "    && apt-get autoremove -y \n"
    "WORKDIR /home/\n"
    )

    # TODO: Remove more dev tools as we want the final image to be as small as possible
   
    if machine == "generic":
        docker_target = DockerClass()
        docker_target.set_recipe_name("Dockerfile-yandasoft-" + mpi)
        docker_target.set_recipe(header + common_part)
        docker_target.set_image_name(prepend + mpi + append)

    elif (machine == "galaxy"):
        docker_target = DockerClass()
        docker_target.set_recipe_name("Dockerfile-yandasoft-" + machine)
        docker_target.set_recipe(header + common_part)
        docker_target.set_image_name(prepend + machine + append)

    else:
        raise ValueError("Unknown machine target:", machine)

    docker_target.write_recipe()
    if actual:
        docker_target.build_image()
    else:
        print(docker_target.get_build_command())

    return docker_target



def make_batch_file(machine, mpi):
    '''
    Make sample batch files for SLURN
    '''

    batch_common_part = (
    "#!/bin/bash -l\n"
    "## This file is automatically created by " + __file__ + "\n"
    "#SBATCH --ntasks=5\n"
    "##SBATCH --ntasks=305\n"
    "#SBATCH --time=02:00:00\n"
    "#SBATCH --job-name=cimager\n"
    "#SBATCH --export=NONE\n\n"
    "module load singularity/3.5.0\n")

    (mpi_type, mpi_ver) = get_mpi_type_and_version(mpi)
    if (mpi_type == "mpich"):
        module = "mpich/3.3.0"
        image = "yandasoft-mpich_latest.sif"
        batch_mpi_part = (
        "module load " + module + "\n\n"
        "mpirun -n 5 singularity exec " + image +
        " cimager -c dirty.in > dirty_${SLURM_JOB_ID}.log\n")

    elif (mpi_type == "openmpi"):
        if (mpi_ver != None):
            module = "openmpi/" + mpi_ver + "-ofed45-gcc"
            image = "yandasoft-" + mpi_ver + "_latest.sif"
            batch_mpi_part = (
            "module load " + module + "\n\n"
            "mpirun -n 5 -oversubscribe singularity exec " + image +
            " cimager -c dirty.in > dirty_${SLURM_JOB_ID}.log\n")

    else:
        raise ValueError("Unknown MPI target:", mpi)

    batch_file = "sample-" + machine + "-" + mpi + ".sbatch"
    print("Making batch file:", batch_file)
    with open(batch_file, "w") as file:
        file.write(batch_common_part + batch_mpi_part)



def show_targets():
    print("The list of Docker targets: ")
    for machine in machine_targets:
        print("- Machine:", machine)
        if machine == "generic":
            for mpi in mpi_targets:
                print("  - MPI:", mpi)
    print("Note that specific machine has a preset MPI target")



def main():
    parser = argparse.ArgumentParser(
        description="Make Docker images for various MPI implementations",
        epilog="The targets can be changed from inside the script (the SETTINGS section)")
    parser.add_argument('-b', '--base_image', help='Create base image', action='store_true')
    parser.add_argument('-d', '--dev_image', help='Create developer image', action='store_true')
    parser.add_argument('-f', '--final_image', help='Create final image', action='store_true')
    parser.add_argument('-s', '--show_targets_only', help='Show targets only', action='store_true')
    #parser.add_argument('-s', '--slurm', help='Create sample batch files for SLURM', action='store_true')
    args = parser.parse_args()

    if args.show_targets_only:
        show_targets()
        sys.exit(0)

    # The common components of image names in DockerHub
    # base_prepend = "csirocass/yandabase-"
    base_prepend = "yandabase-"
    base_append = ":latest"
    # final_prepend = "csirocass/yandasoft-"
    final_prepend = "yandasoft-"
    final_append = ":latest"

    if args.base_image:
        print("Making base images ...")
    else:
        print("Base image will not be made")

    if args.final_image:
        print("Making final images ...")
    else:
        print("Final image will not be made")

    for machine in machine_targets:
        if machine == "generic":
            for mpi in mpi_targets:
                docker = make_base_image(machine, mpi, base_prepend, base_append, args.base_image)
                if docker != None:
                    docker = make_final_image(machine, mpi, final_prepend, final_append, 
                        docker.image_name, args.final_image)
                    if docker == None:
                        raise ValueError("Failed to make final image:", machine, mpi)
                else:
                    raise ValueError("Failed to make base image:", machine, mpi)
        else:
            # Specific machine
            docker = make_base_image(machine, None, base_prepend, base_append, args.base_image)
            if docker != None:
                docker = make_final_image(machine, None, final_prepend, final_append, 
                    docker.image_name, args.final_image)
                if docker == None:
                    raise ValueError("Failed to make final image:", machine)
            else:
                raise ValueError("Failed to make base image:", machine)



if (__name__ == "__main__"):
    if sys.version_info[0] == 3:
        main()
    else:
        raise ValueError("Must use Python 3")
