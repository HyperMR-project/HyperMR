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

# Loop through the list of dataset names
for dataset in datasets:
    # Concatenate the file path, assuming that the file name is the dataset name plus ".txt" suffix
    file_path = os.path.join("..", "output", algorithm, dataset + ".mtx")
    # Open the file and read the fourth value in the first line, convert it to float type and append it to the result list
    with open(file_path, "r") as f:
        value = float(f.readline().split()[3])
        results.append(int(value))

# Print the result list
for x in results:
  print(x)