# From: /work2/05392/cylu/share/remora/binaries/C_collectors_070723
CC = gcc
CXXFLAGS=-O3

OBJECTS += data_collect_cpu.o data_collect_eth.o data_collect_ib.o data_collect_memory.o data_collect_numa.o data_collect_opa.o data_collect_nv_power.o data_collect_nv_temperature.o

BINARIES += data_collect_cpu data_collect_eth data_collect_ib data_collect_memory data_collect_numa data_collect_opa data_collect_nv_power data_collect_nv_temperature

ifeq ($(shell ldconfig -p | grep libnvidia-ml >& /dev/null && echo 1 || echo 0), 1)
OBJECTS  += data_collect_gpu.o
BINARIES += data_collect_gpu
endif

BUILD_DIR=../binary_data_collectors

all: $(BINARIES)
	@make clean

data_collect_cpu: data_collect_cpu.o
	$(shell mkdir -p $(BUILD_DIR))
	@$(CC) data_collect_cpu.o -o $(BUILD_DIR)/data_collect_cpu
	$(info [CC] Building data_collect_cpu)

data_collect_eth: data_collect_eth.o
	$(shell mkdir -p $(BUILD_DIR))
	@$(CC) data_collect_eth.o -o $(BUILD_DIR)/data_collect_eth
	$(info [CC] Building data_collect_eth)

data_collect_ib: data_collect_ib.o
	$(shell mkdir -p $(BUILD_DIR))
	@$(CC) data_collect_ib.o -o $(BUILD_DIR)/data_collect_ib
	$(info [CC] Building data_collect_ib)

data_collect_memory: data_collect_memory.o
	$(shell mkdir -p $(BUILD_DIR))
	@$(CC) data_collect_memory.o -o $(BUILD_DIR)/data_collect_memory
	$(info [CC] Building data_collect_memory)

data_collect_numa: data_collect_numa.o
	$(shell mkdir -p $(BUILD_DIR))
	@$(CC) data_collect_numa.o -o $(BUILD_DIR)/data_collect_numa
	$(info [CC] Building data_collect_numa)

data_collect_opa: data_collect_opa.o
	$(shell mkdir -p $(BUILD_DIR))
	@$(CC) data_collect_opa.o -o $(BUILD_DIR)/data_collect_opa
	$(info [CC] Building data_collect_opa)

data_collect_nv_power: data_collect_nv_power.o
	$(shell mkdir -p $(BUILD_DIR))
	@$(CC) data_collect_nv_power.o -o $(BUILD_DIR)/data_collect_nv_power
	$(info [CC] Building data_collect_nv_power)

data_collect_nv_temperature: data_collect_nv_temperature.o
	$(shell mkdir -p $(BUILD_DIR))
	@$(CC) data_collect_nv_temperature.o -o $(BUILD_DIR)/data_collect_nv_temperature
	$(info [CC] Building data_collect_nv_temperature)

data_collect_gpu: data_collect_gpu.o
	$(shell mkdir -p $(BUILD_DIR))
	@$(CC) data_collect_gpu.o -L/lib64 -lnvidia-ml -o $(BUILD_DIR)/data_collect_gpu
	$(info [CC] Building data_collect_gpu)

data_collect_gpu.o: data_collect_gpu.c
	@$(CC) $(CCFLAGS) $(DEPFLAGS) -L/lib64 -I${NVHPC_ROOT}/cuda/include -lnvidia-ml -c $<

%.o: %.c
	@$(CC) $(CCFLAGS) $(DEPFLAGS) -c $<

clean:
	@rm -rf $(OBJECTS)

clean-all:
	rm -rf $(OBJECTS) $(BINARIES)

.PHONY : all  clean clean-all
