.SUFFIXES: .cpp .o

.cpp.o:
	g++ -c $<

all: btrfs-weekly-rotating-snapshot

btrfs-weekly-rotating-snapshot: btrfs-weekly-rotating-snapshot.o
	g++ -o $@ $^ -lbtrfsutil

clean:
	rm -f *.o btrfs-weekly-rotating-snapshot
