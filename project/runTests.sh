#!/bin/bash
inputdir=${1}
outputdir=${2}
maxthreads=${3}

validint='^[0-9]+$'

if [ ! -d "$inputdir" ];
    then
        echo ${inputdir} is not a directory.
        exit 1
fi

if [ ! -d "$outputdir" ];
    then
        echo ${outputdir} is not a directory.
        exit 1
fi

if ! [[ $maxthreads =~ $validint ]];
    then
        echo MaxThreads must be an integer.
        exit 1
fi

for input in ${inputdir}/*.txt
do
 for threadnum in $(seq 1 ${maxthreads})
 do
  filename=${input##*/}
  echo InputFile=${filename%.*} NumThreads=${threadnum}
  ./tecnicofs < "tecnicofs" "${input}" "${outputdir}/${filename%.*}-${threadnum}.txt" "${threadnum}" | grep "TecnicoFS"
 done
done