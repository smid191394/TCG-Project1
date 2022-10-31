.PHONY: stats judge
all: threes
threes:
	g++ -std=c++11 -O3 -g -Wall -fmessage-length=0 -o threes threes.cpp
stats: threes
	./threes --total=1000 --save=stats.txt
judge: stats
	./threes-judge --load stats.txt
clean:
	rm -f threes stats.txt