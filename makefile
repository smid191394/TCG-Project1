.PHONY: learn stats judge
all: threes
threes:
	g++ -std=c++11 -O3 -g -Wall -fmessage-length=0 -o threes threes.cpp
learn: threes 
	./threes --total=100000 --block=1000 --limit=1000 --learn
stats: threes
	./threes --total=1000 --save=stats.txt
judge: stats
	./threes-judge --load=stats.txt --judge="version=2 speed=50000"
clean:
	rm -f threes stats.txt td0.txt