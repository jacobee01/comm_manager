CXXFLAGS := -std=c++11 -I./include -fpermissive

TARGET = comm_mcu


all: $(TARGET)

clean:
	rm -f *.a *.o $(TARGET) *.lo

$(TARGET): $(C_INCS) $(C_SRCS) communication_main.lo radar_control.lo comm_mcu.lo common.lo comm_uart.lo comm_function.lo socket_main.lo socket_diagnosis.lo sys_version.lo
	$(CC) -lpthread $(CFLAGS) -o $@ $(C_SRCS) communication_main.lo radar_control.lo comm_mcu.lo common.lo comm_uart.lo comm_function.lo socket_main.lo socket_diagnosis.lo sys_version.lo


communication_main.lo: src/communication_main.c
	$(CXX) -c $(CXXFLAGS) -o $@ src/communication_main.c

radar_control.lo: src/radar_control.c
	$(CXX) -c $(CXXFLAGS) -o $@ src/radar_control.c

comm_mcu.lo: src/comm_mcu.c
	$(CXX) -c $(CXXFLAGS) -o $@ src/comm_mcu.c

common.lo: src/common.c
	$(CXX) -c $(CXXFLAGS) -o $@ src/common.c

comm_uart.lo: src/comm_uart.c
	$(CXX) -c $(CXXFLAGS) -o $@ src/comm_uart.c

comm_function.lo: src/comm_function.c
	$(CXX) -c $(CXXFLAGS) -o $@ src/comm_function.c

socket_main.lo: src/socket_main.c
	$(CXX) -c $(CXXFLAGS) -o $@ src/socket_main.c
	
socket_diagnosis.lo: src/socket_diagnosis.c
	$(CXX) -c $(CXXFLAGS) -o $@ src/socket_diagnosis.c
	
sys_version.lo: src/sys_version.c
	$(CXX) -c $(CXXFLAGS) -o $@ src/sys_version.c
	

