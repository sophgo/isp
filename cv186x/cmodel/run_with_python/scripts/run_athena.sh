. /ic/tools/anaconda/etc/profile.d/conda.sh
conda activate
ulimit -n 4096

# path setting
DPU_DIR=$PWD/../../
PYTHON_EXE=$DPU_DIR/run_with_python/athena_pipeline/athena_algo_run.py
USER=$USER

###################################################################################################
# We suggest that the output directory is set to "/tmp", which can speed up the running
IMG_FOLDER=oliver_test/athena
SRC=/home/$USER/$IMG_FOLDER
DST=/tmp/$USER/Desktop/results_athena_ori/$IMG_FOLDER

echo $(date +%H:%M:%S)
bsub -m cm04 -Is $PYTHON_EXE $IMG_FOLDER --src $SRC --dst $DST --fastMode 1 --end 20
#For example
#1.Use multiprocess
#$PYTHON_EXE $IMG_FOLDER --src $SRC --dst $DST
#2.Use singleprocess
#$PYTHON_EXE $IMG_FOLDER --src $SRC --dst $DST -sp
#3.Use cm04 build
#bsub -m cm04 -Is $PYTHON_EXE $IMG_FOLDER --src $SRC --dst $DST
#4.Use fastMode
#bsub -m cm04 -Is $PYTHON_EXE $IMG_FOLDER --src $SRC --dst $DST --fastMode 2
#5.Use cropMode
#bsub -m cm04 -Is $PYTHON_EXE $IMG_FOLDER --src $SRC --dst $DST --fastMode 2 --crop "0 719 0 479"
echo $(date +%H:%M:%S)
