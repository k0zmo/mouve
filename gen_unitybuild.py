import os

root_dir = os.getcwd() #sys.argv[1]
output_name = 'UnityBuild.cpp' #sys.argv[2]

file_list = []
ext_filter = ['cpp']

for root, sub_folders, files in os.walk(root_dir):
    for file_name in files:
        if file_name[-3:] in ext_filter and output_name not in file_name:
            if root != root_dir:
                full_path = os.path.join(root, file_name)
                file_list.append('#include "{}"'.format(os.path.relpath(full_path, root_dir).replace('\\', '/')))
            else:
                file_list.append('#include "{}"'.format(file_name))            

# write include list to a file
f = open(output_name, 'wb')
f.write("\r\n".join(file_list))
f.close()