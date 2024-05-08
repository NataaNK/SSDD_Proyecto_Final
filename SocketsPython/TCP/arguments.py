import sys

# Count the arguments
arguments = len(sys.argv)

print(arguments)

# Output argument-wise
i = 1;

sys.argv.pop(0)
for a in sys.argv:
    print ("Parameter: ", str(i),  a)
    i = i + 1
