#! /usr/bin/env python

import os
import xmlrunner
import argparse

from builder.make import CompilingSuite, LinkingSuite, CreateMakeFile

def GenerateMakefile(include_list=[], exclude_list=[], makefile='', include_paths=[], extension_list=['.c']):
    source_list = []

    if (exclude_list == None):
        exclude_list = []
    # end if

    for include in include_list:
        if (os.path.isdir(include)):
            for dirName, subdirList, fileList in os.walk(include):
                if '.git' in subdirList:
                    subdirList.remove('.git')
                # end if

                #print('Found directory: %s' % dirName)
                for fname in fileList:
                    if (os.path.splitext(fname)[-1] not in extension_list):
                        continue
                    # end if

                    file_path = os.path.join(dirName, fname)

                    flag = ' ' * 8

                    excluded = False
                    for exclude in exclude_list:
                        result = os.path.relpath(file_path, exclude)

                        if (not result.startswith('..')):
                            excluded = True
                        # end if
                    # end for

                    if (excluded):
                        continue
                    # end if

                    source_list.append(file_path)
                    #print('%s%s' % (flag, file_path))
                # end for
            # end for
        elif (os.path.isfile(include)):
            source_list.append(include)
        # end if
    # end for

    if (include_paths == None):
        include_paths = []
    # end if

    compiling_suite_c = CompilingSuite('c', 'C')
    compiling_suite_c.source_list  = source_list
    compiling_suite_c.include_list = include_paths
    compiling_suite_c.flags        = [' -O0', '-g3', '-Wall', '-c', '-fmessage-length=0']
    
    compiling_suite_a = CompilingSuite('s', 'S')
    compiling_suite_a.source_list  = source_list
    compiling_suite_a.include_list = include_paths
    compiling_suite_a.flags        = [' -O0', '-g3', '-Wall', '-c', '-fmessage-length=0']

    linking_suite = LinkingSuite()
    #linking_suite.flags = ['-L../Dev/Ether/WinPcap/Lib', '-lpacket', '-lwinmm', '-lwpcap']

    CreateMakeFile([compiling_suite_c, compiling_suite_a],
                   linking_suite,
                   makefile)

# end GenerateMakefile()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    #nargs='+',
    parser.add_argument('makefile', help='Path to lint makefile.')
    parser.add_argument('-i', '--include',       action='append', help='Include directory for recursive source search.')
    parser.add_argument('-e', '--exclude',       action='append', help='Exclude directory for recursive source search.')
    parser.add_argument('-I', '--include-paths', action='append', help='Source file include paths.')

    args = parser.parse_args()

    #print(args)

    GenerateMakefile(args.include, args.exclude, args.makefile, args.include_paths, ['.c', '.s', '.S'])
# end if
