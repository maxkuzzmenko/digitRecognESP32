import os
from pathlib import Path

directory = "./dataset/"

"""
variables:

filename_raw 
file_path
file_path_no_ext

"""
for i in range(0, 10):
    for file in os.listdir(directory + str(i)):
        filename_raw = os.fsdecode(file)
        file_path = os.path.join(directory, str(i), filename_raw)
        file_path_no_ext = file_path.rpartition('.')[0]
        if filename_raw.endswith(".png"):
            print(f"{file_path} -> {file_path_no_ext}_{i}.png")
            os.rename(file_path, f"{file_path_no_ext}_{i}.png")