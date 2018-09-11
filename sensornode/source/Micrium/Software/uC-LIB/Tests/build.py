#! /usr/bin/env python

import sys
import argparse
import logging
import unittest
import xmlrunner

#from xmlrunner import testcase

from builder.make   import MakeBuilder
from builder.lint   import LintBuilder
from FileProcessing.SetDefine import SetDefine

global args

TEST_CASE_LOG_LIMIT = (1024 * 1024)

class TestBuild(xmlrunner.TestCase):
    def set_params(self, builder, makefile, jobs, log_limit=TEST_CASE_LOG_LIMIT, compiler=None):
        self.lib_config = {}

        self.makefile   = makefile
        self.jobs       = jobs
        self.log_limit  = log_limit

        if (builder == 'make'):
            self.builder = MakeBuilder(self.makefile, self.jobs)
        elif (builder == 'lint'):
            self.builder = LintBuilder(self.makefile, self.jobs)
        else:
            raise Exception('Invalid Builder: %s.' % builder)
        # end if

        make_vars = {'LD' : 'rm'}

        if (compiler):
            make_vars['CC'] = compiler
        # end if
        
        self.builder.SetVars(make_vars)
    # end

    def test_build(self):
        SetDefine('../Cfg/Template/lib_cfg.h', 'lib_cfg.h', self.lib_config)

        configuration = str(self.lib_config)

        self.builder.Clean()
        self.builder.ProjectBuild()

        if (type(self.builder) == MakeBuilder):
            message = 'configuration:\n%s\n%s' % (configuration, self.builder.cmdErr)
        elif (type(self.builder) == LintBuilder):
            message = 'configuration:\n%s\n%s' % (configuration, self.builder.cmdOut)
        else:
            raise Exception('Invalid Builder: %s.' % str(type(self.builder)))
        # end if

        message_len = len(message)

        if (message_len > self.log_limit):
            message = message[0:self.log_limit-1]
            message += '\n\n *** Log truncated. Showing %d bytes out of %d.' % (self.log_limit, message_len)
        # end if

        self.assertTrue(self.builder.build_success, msg=message)
    # end test_build()
# end class TestBuild


class TestSkip(xmlrunner.TestCase):
    def test_skip(self):
        print('Skipped with --skip arg.')
        self.assertTrue(True, msg='Skipped with --skip arg.')
    # end test_skip()
# end


def load_tests(loader, tests, pattern):
    suite = unittest.TestSuite()

    lib_config = {}

    with open(args.config, 'r') as f:
        for lib_config in f:
            distinguisher = args.builder + ' ' + lib_config.strip()
            
            if (args.name):
                distinguisher += ' ' + args.name
            # end if
            
            test_case = TestBuild("test_build", distinguisher=distinguisher)
            test_case.set_params(args.builder, args.makefile, args.jobs, args.log_limit, args.CC)
            test_case.lib_config = lib_config

            suite.addTest(test_case)
        # end for
    # end with

    return (suite)
# end CreateSuite()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("config",  help="Configuration file.")
    parser.add_argument("builder", choices=['make', 'lint'])
    parser.add_argument("makefile")
    parser.add_argument("--name", default=None, help='Test name')
    parser.add_argument('--CC', default=None, help='Compiler.')
    parser.add_argument("-j", "--jobs",  default=None, type=int, help="Allow N jobs at once; infinite jobs with no arg")
    parser.add_argument("--skip", action='store_true', help='Skip test and mark as all pass.')
    parser.add_argument("--log-limit", type=int, default=TEST_CASE_LOG_LIMIT, help='Byte log limit per testcase.')

    parser.add_argument("--log", default=logging.CRITICAL, help="Set logging level.")
    parser.add_argument('unittest_args', nargs='*')

    global args
    args = parser.parse_args()
    
    print(args)

    logging.basicConfig(level=args.log)

    if (args.skip):
        suite = unittest.TestSuite()
        suite.addTest(TestSkip("test_skip"))
    else:
        suite = load_tests(None, None, None)
    # end if

    sys.argv[1:] = args.unittest_args
    xmlrunner.XMLTestRunner(output='test-reports',
                            failfast=False,
                            buffer=False,
                            verbosity=2).run(suite)
# end if
