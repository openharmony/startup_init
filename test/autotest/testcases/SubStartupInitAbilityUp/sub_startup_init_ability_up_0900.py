# Copyright (c) 2024 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
from time import sleep
from devicetest.core.test_case import Step, TestCase
from hypium import UiParam


class SubStartupInitAbilityUp0900(TestCase):

    def __init__(self, controllers):
        self.tag = self.__class__.__name__
        self.tests = [
            "test_step1"
        ]
        TestCase.__init__(self, self.tag, controllers)
        self.driver = UiDriver(self.device1)

    def setup(self):
        Step("预置工作:初始化手机开始.................")
        Step(self.devices[0].device_id)

    def test_step1(self):
        Step("重命名系统文件 ability.cfg..........................")
        self.driver.hdc("target mount")
        self.driver.shell(
            "cp /system/etc/init/ability.cfg /system/etc/init/ability.cfg_bak")
        Step("把系统文件 ability.cfg 拉到本地..........................")
        devpath = "/system/etc/init/ability_ability.cfg"
        self.driver.pull_file(devpath, "testFile/SubStartupInitAbilityUp/ability_ability.cfg")
        Step("修改本地系统文件..........................")
        lines1 = []
        path2 = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))),
                             "testFile\SubStartupInitAbilityUp")
        localpath = os.path.join(path2, "ability_ability.cfg")
        with open(localPath, "r") as y:
            for line in y:
                lines1.append(line)
            y.close()
        line = lines1.index('            "secon" : "u:r:ability:s0",\n')
        res1 = line
        Step(f"目标索引：{res1}")
        index_str1 = lines1[res1]
        Step(f"需要修改的目标：{index_str1}")
        lines1.remove(index_str1)
        Step(f"移除目标之后：{lines1}")
        list_item = '            "secon" : "u:r:abilityly:s0",\n'
        lines1.insert(line, list_item)
        s = ''.join(lines1)
        Step(f"添加修改之后的文件内容：{s}")
        with open(localPath, "w") as z:
            z.write(s)
            z.close()
        Step("上传修改后的本地系统文件..........................")
        self.driver.push_file(localPath, devpath)
        Step("执行重启..........................")
        self.driver.System.reboot()
        sleep(3)
        self.driver.System.wait_for_boot_complete()
        Step("执行命令begetctl dump_service..........................")
        result = self.driver.shell('begetctl dump_service ability')
        Step(result)
        pid = self.driver.System.get_pid('ability')
        Step(pid)

    def teardown(self):
        Step("收尾工作.................")
        Step("恢复系统文件ability.cfg..........................")
        self.driver.hdc("target mount")
        self.driver.shell("rm -rf /system/etc/init/ability.cfg")
        self.driver.shell(
            "mv /system/etc/init/ability.cfg_bak /system/etc/init/ability.cfg")
        Step("执行重启..........................")
        self.driver.System.reboot()
        sleep(3)
        self.driver.System.wait_for_boot_complete()
