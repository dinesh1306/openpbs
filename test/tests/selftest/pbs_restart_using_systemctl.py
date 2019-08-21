# coding: utf-8

# Copyright (C) 1994-2019 Altair Engineering, Inc.
# For more information, contact Altair at www.altair.com.
#
# This file is part of the PBS Professional ("PBS Pro") software.
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

from tests.selftest import *
from ptl.utils.plugins.ptl_test_runner import TimeOut


class TestPBSRestartSyatemctl(TestSelf):
    """
    Test suite to verify PBS operation is working peoperly using systemctl.
    """

    def setUp(self):
        TestSelf.setUp(self)
        # Skip tests if systemctl command is not found.
        is_systemctl = self.du.which(exe='systemctl')
        if is_systemctl == 'systemctl':
            self.skipTest("Systemctl command not found")

    def check_daemon_status(self):
        """
        Function for check All daemons are running.
        """

        rv = self.server.isUp()
        self.assertTrue(rv)
        rv = self.scheduler.isUp()
        self.assertTrue(rv)
        rv = self.mom.isUp()
        self.assertTrue(rv)
        rv = self.comm.isUp()
        self.assertTrue(rv)

    def test_pbs_restart(self):
        """
        Verify that PBS is restart successfully using systemctl.
        """

        self.server.restart()
        self.check_daemon_status()

    def test_pbs_stop(self):
        """
        Verify that PBS is stop successfully using systemctl.
        """

        if not self.server.isUp():
            self.server.start()

        self.server.stop()
        rv = self.server.isUp()
        self.assertFalse(rv)
        rv = self.scheduler.isUp()
        self.assertFalse(rv)
        rv = self.mom.isUp()
        self.assertFalse(rv)
        rv = self.comm.isUp()
        self.assertFalse(rv)

        self.server.start()

    def test_pbs_start(self):
        """
        Verify that PBS is start successfully using systemctl.
        """

        if self.server.isUp():
            self.server.stop()

        self.server.start()
        self.check_daemon_status()
