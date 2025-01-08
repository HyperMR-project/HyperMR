'''
Copyright (c) . 
All rights reserved. 

This file is covered by the LICENSE.txt license file in the root directory.

Description: 


'''
import os
import numpy as np
from skimage import io, filters

path = 'images'
mtx_path = 'mtx'
img_height = 4096
img_width = 4096

if not os.path.exists(mtx_path):
    os.makedirs(mtx_path)

for filename in os.listdir(path):
    if not filename.endswith('.jpg') and not filename.endswith('.jpeg') and not filename.endswith('.png'):
        continue

    # load as the gray image
    gray_image = io.imread(os.path.join(path, filename), as_gray=True)
    # Otsu's Adaptive Binarization
    threshold = filters.threshold_otsu(gray_image)

    binary_image = gray_image > threshold

    img_array = np.array(binary_image)
    non_zero = np.sum(img_array == 0) # no zero in matrix
    sparsity = 100 - float(non_zero) / (img_height * img_width) * 100

    matrix_name = filename[:-3] + "mtx"
    with open(os.path.join(mtx_path, matrix_name), "w") as f:
      f.write(str(img_height) + " " + str(img_width) + " " + str(non_zero) + "\n")
      for i in range(len(img_array)):
          for j in range(len(img_array[i])):
              if img_array[i][j] == 0:
                  f.write(str(i+1) + " " + str(j+1) + " " + str(1) + "\n")

    print('Image:', filename)
    print('Non-zero Item:', non_zero)
    print('Sparsity:', sparsity)