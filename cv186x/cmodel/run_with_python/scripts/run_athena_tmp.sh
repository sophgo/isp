DPU_DIR=$PWD/../../
PYTHON_EXE=$DPU_DIR/run_with_python/athena_pipeline/athena_algo_run.py
USER=$USER

IMG_FOLDER=sdr
SRC=/$HOME/cmodel_dataset/$IMG_FOLDER
DST=/$HOME/tmp/results_a2/$IMG_FOLDER

echo $(date +%H:%M:%S)
$PYTHON_EXE $IMG_FOLDER --src $SRC --dst $DST --fastMode 1 --end 2
#For example
#1.Use multiprocess
#$PYTHON_EXE $IMG_FOLDER --src $SRC --dst $DST
#2.Use singleprocess
#$PYTHON_EXE $IMG_FOLDER --src $SRC --dst $DST -sp
#3.Use cm04 build
#$PYTHON_EXE $IMG_FOLDER --src $SRC --dst $DST
#4.Use fastMode
#$PYTHON_EXE $IMG_FOLDER --src $SRC --dst $DST --fastMode 2
#5.Use cropMode
#$PYTHON_EXE $IMG_FOLDER --src $SRC --dst $DST --fastMode 2 --crop "0 719 0 479"
echo $(date +%H:%M:%S)
