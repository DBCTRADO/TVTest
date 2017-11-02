#!/bin/bash

## Architecture
##  -a x86|x64
## CRT type
##  -c dynamic|static
## Output directory
##  -o dir
## Platform
##  -p xp
## Archive
##  -r 7z|bz2
## Target
##  -t debug|release

arch=
target=release
crt_type=static
platform=
out_dir=package
archive=7z

while getopts a:c:o:p:r:t: arg
do
    case $arg in
    a)
        arch=$OPTARG
        ;;
    c)
        crt_type=$OPTARG
        ;;
    o)
        out_dir=$OPTARG
        ;;
    p)
        platform=$OPTARG
        ;;
    r)
        archive=$OPTARG
        ;;
    t)
        target=$OPTARG
        ;;
    *)
        echo "Unknown parameter $arg" >&2
        exit 1
    esac
done

if [ "$arch" = "" ]
then
    if [ -d src/x64 ]
    then
        arch=x64
    else
        arch=x86
    fi
fi

if [ "$arch" = x86 ]
then
    src_arch_dir=Win32
else
    src_arch_dir=$arch
fi

src_bin_dir=src/${src_arch_dir}/${target}
if [ "$platform" = xp ]
then
    src_bin_dir=${src_bin_dir}_XP
fi
if [ "$crt_type" = dynamic ]
then
    src_bin_dir=${src_bin_dir}_MD
fi

dst_dir=${out_dir}/${arch}/${target}

rm -rf ${dst_dir}/*
mkdir -p ${dst_dir}
mkdir -p ${dst_dir}/Plugins
mkdir -p ${dst_dir}/Themes

cp -fp "${src_bin_dir}/TVTest.exe" "${dst_dir}/TVTest.exe"
if [ $? -ne 0 ]
then
    echo "Failed to copy TVTest.exe" >&2
    exit 1
fi

cp -fp "${src_bin_dir}/TVTest_Image.dll" "${dst_dir}/TVTest_Image.dll"

cp -fp doc/* "${dst_dir}"

data_files=(DRCSMap.sample.ini TVTest.chm TVTest.search.ini TVTest.style.ini TVTest.tuner.ini)
for data_file in ${data_files[@]}
do
    cp -fp "data/${data_file}" "${dst_dir}/${data_file}"
done

if [ "$arch" = x86 ]
then
    cp -fp data/TVTest_Logo.bmp "${dst_dir}/TVTest_Logo.bmp"
else
    cp -fp "data/Data_${arch}/TVTest_Logo.bmp" "${dst_dir}/TVTest_Logo.bmp"
fi

cp -fp data/Themes/*.httheme ${dst_dir}/Themes

## Copy only "useful" plugins
plugin_files=(DiskRelay.tvtp Equalizer.tvtp GamePad.tvtp HDUSRemocon.tvtp HDUSRemocon_KeyHook.dll LogoList.tvtp MemoryCapture.tvtp SignalGraph.tvtp SleepTimer.tvtp SpectrumAnalyzer.tvtp TunerPanel.tvtp)

plugin_src_dir=sdk/Samples/${src_arch_dir}/${target}
if [ "$crt_type" = static ]
then
    plugin_src_dir=${plugin_src_dir}_static
fi

for plugin_file in ${plugin_files[@]}
do
    cp -fp "${plugin_src_dir}/${plugin_file}" "${dst_dir}/Plugins/${plugin_file}"
done

version=
if [[ $(grep -aE ^#define[[:space:]]+VERSION_TEXT_A src/TVTestVersion.h) =~ \"(.+)\"$ ]]
then
    version=${BASH_REMATCH[1]}_
fi

git_hash=$(git rev-parse --short HEAD)

archive_name=${out_dir}/TVTest_${version}${git_hash}_${arch}

if [ "$archive" = 7z ]
then
    ## Archive with 7-Zip
    sevenz_exe='/c/Program Files/7-Zip/7z.exe'
    if [ ! -f "$sevenz_exe" ]
    then
        sevenz_exe='/c/Program Files (x86)/7-Zip/7z.exe'
        if [ ! -f "$sevenz_exe" ]
        then
            echo "Unable to find 7z.exe" >&2
            exit 1
        fi
    fi

    "$sevenz_exe" a "${archive_name}.7z" "./${dst_dir}/\*" -mx=9 -ms=on -myx=9
elif [ "$archive" = bz2 ]
then
    ## Archive with bzip2
    tar -jcf "${archive_name}.tar.bz2" -C "${dst_dir}" .
fi
