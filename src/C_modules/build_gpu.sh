ml cuda/11.3

gcc -L/lib64 -I$TACC_CUDA_INC -o collect_gpu collect_gpu.c -lnvidia-ml
