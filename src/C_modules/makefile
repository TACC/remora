CC = gcc
CXXFLAGS=-O3

OBJECTS += collect_cpu.o collect_eth.o collect_ib.o collect_memory.o collect_numa.o collect_opa.o collect_power.o collect_temperature.o

BINARIES += collect_cpu collect_eth collect_ib collect_memory collect_numa collect_opa collect_power collect_temperature

BUILD_DIR=../binaries

all: $(BINARIES)
	@make clean

collect_cpu: collect_cpu.o
	$(shell mkdir -p $(BUILD_DIR))
	@$(CC) collect_cpu.o -o $(BUILD_DIR)/collect_cpu
	$(info [CC] Building collect_cpu)

collect_eth: collect_eth.o
	$(shell mkdir -p $(BUILD_DIR))
	@$(CC) collect_eth.o -o $(BUILD_DIR)/collect_eth
	$(info [CC] Building collect_eth)

collect_ib: collect_ib.o
	$(shell mkdir -p $(BUILD_DIR))
	@$(CC) collect_ib.o -o $(BUILD_DIR)/collect_ib
	$(info [CC] Building collect_ib)

collect_memory: collect_memory.o
	$(shell mkdir -p $(BUILD_DIR))
	@$(CC) collect_memory.o -o $(BUILD_DIR)/collect_memory
	$(info [CC] Building collect_memory)

collect_numa: collect_numa.o
	$(shell mkdir -p $(BUILD_DIR))
	@$(CC) collect_numa.o -o $(BUILD_DIR)/collect_numa
	$(info [CC] Building collect_numa)

collect_opa: collect_opa.o
	$(shell mkdir -p $(BUILD_DIR))
	@$(CC) collect_opa.o -o $(BUILD_DIR)/collect_opa
	$(info [CC] Building collect_opa)

collect_power: collect_power.o
	$(shell mkdir -p $(BUILD_DIR))
	@$(CC) collect_power.o -o $(BUILD_DIR)/collect_power
	$(info [CC] Building collect_power)

collect_temperature: collect_temperature.o
	$(shell mkdir -p $(BUILD_DIR))
	@$(CC) collect_temperature.o -o $(BUILD_DIR)/collect_temperature
	$(info [CC] Building collect_temperature)

%.o: %.c
	@$(CC) $(CCFLAGS) $(DEPFLAGS) -c $<

clean:
	@rm -rf $(OBJECTS)

clean-all:
	rm -rf $(OBJECTS) $(BINARIES)

.PHONY : all  clean clean-all
