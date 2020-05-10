#makefile for count.c

CC=gcc
CGLAGS = -g -Wall

all: count

count: count.c
	$(CC) $(CFLAGS) -o count count.c

clean:
	rm count
# 	@ ls | grep out | xargs rm

# run: all
# 	./count ./testfiles/example1 "Sly Fox" example1out
# 	./count ./testfiles/example1 the example1out
# 	./count ./testfiles/example2 tomato example2out
# 	./count ./testfiles/example2 potato example2out
# 	./count ./testfiles/example3 the out5
# 	./count ./testfiles/example3 "I want" out6
# 	./count ./testfiles/example3 "I want to" out7
# 	./count ./testfiles/example4 $$'f\x0A' out8
# 	./count ./testfiles/foo.jpg Apple out9
# 	./count ./testfiles/foo.jpg bplist out9
# 	./count ./testfiles/foo.jpg Apple out9
# 	# error cases
# 	./count "  " "Sly Fox" errCaseout
# 	./count ./testfiles/example "Sly Fox" errCaseout
# 	./count ./testfiles/example "Sly Fox"
# 	./count ./testfiles/example1 "Sly Fox" "/"