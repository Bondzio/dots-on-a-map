#! /usr/bin/env python

import os
import sys
import copy
import time
import shutil
import logging
import argparse
import threading
import pyharness.test as pyharness
from FileProcessing.SetDefine import SetDefine
from builder import Builder, Cleanup

def ConfigIterator(config_matrix):
    cfg_defines = {}

    define_list          = []
    define_possibilities = {}

    for define in config_matrix:
        cfg_defines[define]          = config_matrix[define][0]
        define_possibilities[define] = len(config_matrix[define])
        define_list.append(define)
    #end for

    define_list.sort()
    total_iterations = 1

    for define in define_possibilities:
        total_iterations *= define_possibilities[define]
    # end for

    print('total_iterations = %d' % total_iterations)

    for ix in range(0, total_iterations):
        quotient = ix
        for define in define_list:
            quotient, rest = divmod(quotient, define_possibilities[define])

            cfg_defines[define] = config_matrix[define][rest]
        # end for
        yield (cfg_defines, ix)
    # end for
# end ConfigIterator()

def PrintConfig(config):
    longest_config_name = 0
    define_names        = []
    print_config        = ''

    for define in config:
        define_names.append(define)
        if (len(define) > longest_config_name):
            longest_config_name = len(define)
        #end if
    # end for

    define_names.sort()

    for define in define_names:
        extra_space = longest_config_name - len(define)

        print_config += '%s%s %s' % (define, ' ' * extra_space, config[define]) + '\n'
    # end for

    return (print_config)
# end PrintConfig()

def OnFailure(builder, out, err, return_code):
    error_lines = err.split(os.linesep)

    for line in error_lines:
        if (line.startswith('#error')):
            return (True)
        # end if
    # end for

    return (False)
# end OnFailure()

def RunMatrixTest(args):
    test_case_list = []
    builder = Builder(args)
    builder.FailureCallbackSet(OnFailure)

    config_matrix = {
        'LIB_MEM_CFG_ARG_CHK_EXT_EN'     : ['DEF_DISABLED', 'DEF_ENABLED'],
        'LIB_MEM_CFG_OPTIMIZE_ASM_EN'    : ['DEF_DISABLED', 'DEF_ENABLED'],
        'LIB_MEM_CFG_DBG_INFO_EN'        : ['DEF_DISABLED', 'DEF_ENABLED'],
        'LIB_MEM_CFG_HEAP_PADDING_ALIGN' : ['LIB_MEM_PADDING_ALIGN_NONE', '4u'],
        'LIB_STR_CFG_FP_EN'              : ['DEF_DISABLED', 'DEF_ENABLED'],
        'LIB_STR_CFG_FP_MAX_NBR_DIG_SIG' : ['LIB_STR_FP_MAX_NBR_DIG_SIG_MIN', 'LIB_STR_FP_MAX_NBR_DIG_SIG_DFLT', 'LIB_STR_FP_MAX_NBR_DIG_SIG_MAX'],
        'LIB_MEM_CFG_HEAP_SIZE'          : ['0u', '4u'],
                    }

    for lib_cfg_defines, ix in ConfigIterator(config_matrix):
        if (args.cfg_num):
            if (int(args.cfg_num) != ix):
                continue
            # end if
        # end if

        print(PrintConfig(lib_cfg_defines))

        local_vars = {
                      'builder'         : builder,
                      'SetDefine'       : SetDefine,
                      'lib_cfg_defines' : lib_cfg_defines,
                     }

        local_test_case_list = pyharness.RunTestFile(args,
                                                     local_vars,
                                                     class_name="uC-LIB",
                                                     report_directory=args.report_dir,
                                                     produce_results=False,
                                                     test_postfix=' %d' % ix
                                                     )

        for test_case in local_test_case_list:
            if (test_case.success == False):
                test_case.error.append(PrintConfig(lib_cfg_defines) + '\n')
            # end if
        # end for

        test_case_list += local_test_case_list
    # end if

    Cleanup(args, builder)
    pyharness.PrintSummary(test_case_list)
    pyharness.CreateXML_Report(test_case_list, os.path.join(args.report_dir, "uC-LIB_Results.xml"), 1.0, 'uC-LIB')
#end RunMatrixTest()

def RunTests(args):
    builder = Builder(args)

    local_vars = {
                  'builder'      : builder,
                  'SetDefine'    : SetDefine,
                 }

    pyharness.RunTestFile(args,
                          local_vars,
                          class_name="uC-LIB",
                          report_directory=args.report_dir)

    Cleanup(args, builder)
#end RunTests()

if __name__ == '__main__':
    global args
    parser = argparse.ArgumentParser()

    parser.add_argument("test_file",  help="Path to build   JSON test file.")
    parser.add_argument("builder",    help="Builder to build project.", choices=['ecs', 'iar', 'iarmake', 'make', 'lint'])
    parser.add_argument('report_dir', help='Path to Report directory where xml are dropped.')
    parser.add_argument('--matrix',   help='Run configuration matrix test', action="store_true")
    parser.add_argument('--log',      help='Log Level.', default=logging.CRITICAL)
    parser.add_argument('--number',   help='Run specified test number only.', default=None)
    parser.add_argument('--last',     help='Run the last test only.', action='store_true')
    parser.add_argument('--cfg_num',  help='Run specific configuration iteration only.', default=None)

    args, unknown = parser.parse_known_args()

    if (args.builder == 'ecs'):
        parser.add_argument('eclipse_manager_path', help='Path to EclipseManager.jar.')
        parser.add_argument('project_path',         help='Path to Project Directory.')
        parser.add_argument('eclipse_path',         help='Path to Eclipse.exe.')
        parser.add_argument('workspace_path',       help='Path to Workspace.')
        parser.add_argument('build_cfg',            help='Project Build Configuration.')
    elif (args.builder == 'iar'):
        parser.add_argument('iar_path',             help='Path to iarbuild.exe')
        parser.add_argument('project_path',         help='Path to EWP file.')
        parser.add_argument('build_cfg',            help='Project Build Configuration.')
    elif ((args.builder == 'make') or (args.builder == 'lint') or (args.builder == 'iarmake')):
        parser.add_argument('makefile',             help='Path to makefile.')
        parser.add_argument('--make-thread',        help='makefile -j param.', default=None)
    # end if

    parser.add_argument("--run_test_file", help="Path to runtime JSON test file.")

    args, unknown = parser.parse_known_args()

    if (args.run_test_file):
        parser.add_argument('--DUT_HOSTNAME', help='IP address of host.',       required=True)
        parser.add_argument('--net_if',       help='Interface of the DUT.',     required=True)
        parser.add_argument('--exe',          help='Path to built executable.', required=True)
        parser.add_argument('--sub_number',   help='Run specified runtime test number only.', default=None)
    #end if

    args          = parser.parse_args()
    numeric_level = getattr(args, 'log')

    logging.basicConfig(format='%(levelname)s %(module)s.%(funcName)s():%(lineno)d %(message)s', level=numeric_level)

    if (args.matrix):
        RunMatrixTest(args)
    else:
        RunTests(args)
    # end if
# end if