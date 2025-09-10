All:
	g++ main.cpp -fopenmp -O3 -march=native -mtune=native -Wall -o model.linux
	x86_64-w64-mingw32-g++ -fopenmp -O3 -Wall main.cpp -static -o model.exe
