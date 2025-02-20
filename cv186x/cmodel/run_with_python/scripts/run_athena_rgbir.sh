. /ic/tools/anaconda/etc/profile.d/conda.sh
conda activate
ulimit -n 4096


# path setting
DPU_DIR=$PWD/../../
PYTHON_EXE=$DPU_DIR/run_with_python/athena_pipeline/athena_algo_run.py
USER=$USER


###################################################################################################
# IMG_FOLDER=dataset_HDR_3exp
# SRC=$PWD/../..//run_with_python/athena_pipeline/$IMG_FOLDER
# DST=/home/$USER/Desktop/results_athena/$IMG_FOLDER

# echo $(date +%H:%M:%S)
# python3 $PYTHON_EXE $IMG_FOLDER \
#         --src $SRC \
#         --dst $DST \
#         --sp
# echo $(date +%H:%M:%S)


IMG_FOLDER=dataset_RGBIR_2exp
SRC=$PWD/../..//run_with_python/athena_pipeline/$IMG_FOLDER
DST=/home/$USER/Desktop/results_athena/$IMG_FOLDER

echo $(date +%H:%M:%S)
python3 $PYTHON_EXE $IMG_FOLDER \
        --src $SRC \
        --dst $DST \
        --sp 
echo $(date +%H:%M:%S)


# IMG_FOLDER=mars_test_1
# SRC=/home/cmodel_release/$IMG_FOLDER
# DST=/home/$USER/Desktop/results_athena/$IMG_FOLDER

# #bsub -m cm02 -Is \
# echo $(date +%H:%M:%S)
# python3 $PYTHON_EXE $IMG_FOLDER \
#        --ini_file /home/huichong.li/athena/run_with_python/athena_pipeline/dataset_RGBIR_2exp/rgbir_test_1/20211119183132_RCCB_Athena.ini \
#        --folder_name "rgbir_test_1" \
#        --get_ini_mode 1 \
#         --src $SRC \
#         --dst $DST \
#         --sp --first_stage "crop0" --last_stage "tnr" 
# echo $(date +%H:%M:%S)
