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
f.write("""/*
 * Copyright (c) 2013-2014 Kajetan Swierk <k0zmo@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

""")
f.write("\r\n".join(file_list))
f.close()