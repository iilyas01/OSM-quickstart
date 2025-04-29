CXX = g++
CXXFLAGS = -std=c++2a `xml2-config --cflags`

all: main highways graph extract

main: main.o
	$(CXX) main.o -o main `xml2-config --libs`

graph: graph.o
	$(CXX) graph.o -o graph `xml2-config --libs`

highways: highways.o
	$(CXX) highways.o -o highways `xml2-config --libs`

extract: extract.o
	$(CXX) extract.o -o extract `xml2-config --libs`

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp

highways.o: highways.cpp
	$(CXX) $(CXXFLAGS) -c highways.cpp

graph.o: graph.cpp
	$(CXX) $(CXXFLAGS) -c graph.cpp

extract.o: extract.cpp
	$(CXX) $(CXXFLAGS) -c extract.cpp

clean:
	rm -f *.o graph main highways extract
