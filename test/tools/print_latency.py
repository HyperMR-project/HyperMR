'''
Copyright (c) . 
All rights reserved. 

This file is covered by the LICENSE.txt license file in the root directory.

Description: 


'''
# Import the os module for file and directory operations
import os

# Open the input.txt file and read the first line as the algorithm name and the second line as the list of dataset names
with open("input.txt", "r") as f:
    algorithm = f.readline().strip() # Remove the newline character
    datasets = []
    line = f.readline().strip()
    while line != "":
      datasets.append(line)
      line = f.readline().strip()

# Define an empty list to store the output results
results = []

file_path = os.path.join("..", "output", algorithm, "MVM.txt")
with open(file_path, "r") as f:
    line = f.readline().split()
    while len(line) != 0:
       results.append(int(line[1]))
       line = f.readline().split()

# Print the result list
for x in results:
  print(x)