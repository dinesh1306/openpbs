# coding: utf-8

# Copyright (C) 1994-2018 Altair Engineering, Inc.
# For more information, contact Altair at www.altair.com.
#
# This file is part of the PBS Professional("PBS Pro") software.
#
# Open Source License Information:
#
# PBS Pro is free software. You can redistribute it and/or modify it under the
# terms of the GNU Affero General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# PBS Pro is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.
# See the GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Commercial License Information:
#
# For a copy of the commercial license terms and conditions,
# go to: (http://www.pbspro.com/UserArea/agreement.html)
# or contact the Altair Legal Department.
#
# Altair’s dual-license business model allows companies, individuals, and
# organizations to create proprietary derivative works of PBS Pro and
# distribute them - whether embedded or bundled with other software -
# under a commercial license agreement.
#
# Use of Altair’s trademarks, including but not limited to "PBS™",
# "PBS Professional®", and "PBS Pro™" and Altair’s logos is subject to Altair's
# trademark licensing policies.

from tests.functional import *


class TestPbsHookAlarmLargeMultinodeJob(TestFunctional):

    """
    This test suite contains hooks test to verify that a large
    multi-node job does not slow down hook execution and cause an alarm.
    """

    @timeout(400)
    def setUp(self):

        TestFunctional.setUp(self)

        a = {'resources_available.mem': '1gb',
             'resources_available.ncpus': '1'}
        self.server.create_vnodes(self.mom.shortname, a, 5000, self.mom)

    def test_begin_hook(self):
        """
        Create an execjob_begin hook, import a hook content with a small
        alarm value, and test it against a large multi-node job.
        """
        hook_name = "testhook"
        hook_event = "execjob_begin"
        hook_body = """
import pbs
e=pbs.event()
pbs.logmsg(pbs.LOG_DEBUG, "executing begin hook %s" % (e.hook_name,))
"""
        a = {'event': hook_event, 'enabled': 'True',
             'alarm': '15'}
        self.server.create_import_hook(hook_name, a, hook_body)

        j = Job(TEST_USER)
        a = {'Resource_List.select': '5000:ncpus=1:mem=1gb',
             'Resource_List.walltime': 10}

        j.set_attributes(a)

        jid = self.server.submit(j)

        self.server.expect(JOB, {'job_state': 'R'},
                           jid, max_attempts=15, interval=2)
        self.mom.log_match(
            "pbs_python;executing begin hook %s" % (hook_name,), n=100,
            max_attempts=5, interval=5, regexp=True)

        self.mom.log_match(
            "Job;%s;alarm call while running %s hook" % (jid, hook_event),
            n=100, max_attempts=5, interval=5, regexp=True, existence=False)

        self.mom.log_match("Job;%s;Started, pid" % (jid,), n=100,
                           max_attempts=5, interval=5, regexp=True)

    def test_prolo_hook(self):
        """
        Create an execjob_prologue hook, import a hook content with a
        small alarm value, and test it against a large multi-node job.
        """
        hook_name = "testhook"
        hook_event = "execjob_prologue"
        hook_body = """
import pbs
e=pbs.event()
pbs.logmsg(pbs.LOG_DEBUG, "executing prologue hook %s" % (e.hook_name,))
"""
        a = {'event': hook_event, 'enabled': 'True',
             'alarm': '15'}
        self.server.create_import_hook(hook_name, a, hook_body)

        j = Job(TEST_USER)
        a = {'Resource_List.select': '5000:ncpus=1:mem=1gb',
             'Resource_List.walltime': 10}

        j.set_attributes(a)

        jid = self.server.submit(j)

        self.server.expect(JOB, {'job_state': 'R'},
                           jid, max_attempts=15, interval=2)

        self.mom.log_match(
            "pbs_python;executing prologue hook %s" % (hook_name,), n=100,
            max_attempts=5, interval=5, regexp=True)

        self.mom.log_match(
            "Job;%s;alarm call while running %s hook" % (jid, hook_event),
            n=100, max_attempts=5, interval=5, regexp=True, existence=False)

    def test_epi_hook(self):
        """
        Create an execjob_epilogue hook, import a hook content with a small
        alarm value, and test it against a large multi-node job.
        """
        hook_name = "testhook"
        hook_event = "execjob_epilogue"
        hook_body = """
import pbs
e=pbs.event()
pbs.logmsg(pbs.LOG_DEBUG, "executing epilogue hook %s" % (e.hook_name,))
"""
        a = {'event': hook_event, 'enabled': 'True',
             'alarm': '15'}
        self.server.create_import_hook(hook_name, a, hook_body)

        j = Job(TEST_USER)
        a = {'Resource_List.select': '5000:ncpus=1:mem=1gb',
             'Resource_List.walltime': 10}

        j.set_attributes(a)

        jid = self.server.submit(j)

        self.server.expect(JOB, {'job_state': 'R'},
                           jid, max_attempts=15, interval=2)
        self.mom.log_match(
            "pbs_python;executing epilogue hook %s" % (hook_name,), n=100,
            max_attempts=5, interval=5, regexp=True)

        self.mom.log_match(
            "Job;%s;alarm call while running %s hook" % (jid, hook_event),
            n=100, max_attempts=5, interval=5, regexp=True, existence=False)

        self.mom.log_match("Job;%s;Obit sent" % (jid,), n=100,
                           max_attempts=5, interval=5, regexp=True)

    def test_end_hook(self):
        """
        Create an execjob_end hook, import a hook content with a small
        alarm value, and test it against a large multi-node job.
        """
        hook_name = "testhook"
        hook_event = "execjob_end"
        hook_body = """
import pbs
e=pbs.event()
pbs.logmsg(pbs.LOG_DEBUG, "executing end hook %s" % (e.hook_name,))
"""
        a = {'event': hook_event, 'enabled': 'True',
             'alarm': '15'}
        self.server.create_import_hook(hook_name, a, hook_body)

        j = Job(TEST_USER)
        a = {'Resource_List.select': '5000:ncpus=1:mem=1gb',
             'Resource_List.walltime': 10}

        j.set_attributes(a)

        jid = self.server.submit(j)

        self.server.expect(JOB, {'job_state': 'R'},
                           jid, max_attempts=15, interval=2)
        self.mom.log_match(
            "pbs_python;executing end hook %s" % (hook_name,), n=100,
            max_attempts=5, interval=5, regexp=True)

        self.mom.log_match(
            "Job;%s;alarm call while running %s hook" % (jid, hook_event),
            n=100, max_attempts=5, interval=5, regexp=True, existence=False)

        self.mom.log_match("Job;%s;Obit sent" % (jid,), n=100,
                           max_attempts=5, interval=5, regexp=True)
