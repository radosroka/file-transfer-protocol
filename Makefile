CL = client
SRV = server

CXXFLAGS = -O2 -g -Wall -Wextra -pedantic -std=c++11 -g
LDFLAGS = -Wl,-rpath=/usr/local/lib/gcc49/
CXX = g++

all: $(CL) $(SRV)

default: $(CL) $(SRV)

$(CL): $(CL).cpp
$(SRV: $(SRV).cpp

clean:
	rm client
