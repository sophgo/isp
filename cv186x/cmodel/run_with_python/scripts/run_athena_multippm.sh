. /ic/tools/anaconda/etc/profile.d/conda.sh
conda activate
ulimit -n 4096

# path setting
DPU_DIR=$PWD/../../
PYTHON_EXE=$DPU_DIR/run_with_python/athena_pipeline/athena_algo_run.py
USER=$USER

IMG_FOLDER=dataset_for_fpga
SRC=../athena_pipeline/$IMG_FOLDER
DST=/home/$USER/Desktop/results_athena/$IMG_FOLDER

#remove the previous result
rm -rf /home/$USER/Desktop/results_athena/$IMG_FOLDER

echo $(date +%H:%M:%S)
python3 $PYTHON_EXE $IMG_FOLDER \
        --src $SRC \
        --dst $DST --not_using_real_raw \
        --sp --end="4" 2>&1| tee multi_ppm.log 
         
echo $(date +%H:%M:%S)
