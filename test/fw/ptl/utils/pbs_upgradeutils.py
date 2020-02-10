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


import subprocess
import os
import shutil
import pip
import pkgutil
import importlib
import re
import ptl.utils.pbs_dshutils
import ptl.lib.pbs_testlib
from ptl.lib.pbs_testlib import *
from ptl.utils.pbs_dshutils import *


class PBSUpgradeUtils(object):

    def __init__(self, base_pbs_path, upgrade_pbs_path,
                 base_ptl_path, upgrade_ptl_path, base_lic,
                 upgrade_lic, server, scheduler, mom, comm):
        self.du = DshUtils()
        self.server = server
        # conf_file = self.du.get_pbs_conf_file(socket.gethostname())
        self.mom = mom
        self.scheduler = scheduler
        self.comm = comm
        self.base_pbs_path = base_pbs_path
        self.upgrade_pbs_path = upgrade_pbs_path
        self.base_ptl_path = base_ptl_path
        self.upgrade_ptl_path = upgrade_ptl_path
        self.base_lic = base_lic
        self.upgrade_lic = upgrade_lic

    def ptl_lib_path(self):
        ptl_bin = self.du.which(exe='pbs_benchpress')
        path_lst = ptl_bin.split("/bin")
        lib_path = os.path.join(path_lst[0], 'lib',
                                'python3.6',
                                'site-packages')
        print(str(lib_path))
        return str(lib_path)

    @staticmethod
    def reload_ptl():
        importlib.reload(ptl.lib.pbs_testlib)
        importlib.reload(ptl.utils.pbs_dshutils)
        importlib.reload(ptl.utils.pbs_testsuite)

    @staticmethod
    def find_rpm_name(path, daemon):
        files = os.listdir(path)
        find_file = "pbspro-" + daemon
        for file_name in files:
            if re.match(find_file, file_name):
                return file_name

    def install_ptl(self, path):
        p_path = os.path.join(path, 'fw')
        pbs_exec = self.server.pbs_conf['PBS_EXEC']
        ptl_cmd = os.path.join(pbs_exec, 'python', 'bin', 'python')
        ptl_path = self.ptl_lib_path()
        ptl_cmd += " -m pip install -U " + p_path + " --target=" + ptl_path
        status = self.du.run_cmd(hosts=self.server.shortname, cmd=ptl_cmd)

    def pbs_operation(self, op, path=None):
        if path is not None:
            os.chdir(path)
            inst_rpm = PBSUpgradeUtils.find_rpm_name(path, "server")
            inst_mom_rpm = PBSUpgradeUtils.find_rpm_name(path, "execution")
            if op == "install":
                cmd = 'rpm -i ' + inst_rpm
                status = self.du.run_cmd(hosts=self.server.hostname,
                                         cmd=cmd, sudo=True)
                self.server.pi.initd(op='start', daemon='all')
                self.install_ptl(self.base_ptl_path)
            elif op == "upgrade":
                cmd = 'rpm -U ' + inst_rpm
                status = self.du.run_cmd(hosts=self.server.hostname,
                                         cmd=cmd, sudo=True)
                self.install_ptl(self.upgrade_ptl_path)
        else:
            inst_pbs = ''
            cmd = 'rpm -qa'
            status = self.du.run_cmd(hosts=self.server.hostname,
                                     cmd=cmd, sudo=True)
            for pkg in status['out']:
                if re.match("pbspro", pkg):
                    inst_pkg = pkg

            cmd = 'rpm -e ' + inst_pkg
            status = self.du.run_cmd(hosts=self.server.hostname,
                                     cmd=cmd, sudo=True)
            shutil.rmtree(self.server.pbs_conf['PBS_HOME'])

    @classmethod
    def tearDownClass(cls):
        pass
