#
# Copyright (c) 2025 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# -*- coding: utf-8 -*-
import shutil
import os
from hypium import UiDriver
from devicetest.core.test_case import Step, TestCase
from aw import Common


class SubStartupToyboxTouchtest1200(TestCase):

    def __init__(self, controllers):
        self.tag = self.__class__.__name__
        self.tests = [
            "test_step1"
        ]
        TestCase.__init__(self, self.TAG, controllers)
        self.usr_workspace = "/data/local/tmp"

    def setup(self):
        Step("预置工作:初始化PC开始")
        Step(self.devices[0].device_id)
        self.driver = UiDriver(self.device1)
        if os.path.exists("test") != True:
            os.mkdir("test")

    def test_step1(self):
        Step("显示帮助命令")
        result = self.driver.shell(f"help tee")
        self.driver.Assert.contains(result, "usage: tee [-ai] [file...]")
        self.driver.Assert.contains(result, "Copy stdin to each listed file, and also to stdout.")
        self.driver.Assert.contains(result, "-a	Append to files")
        self.driver.Assert.contains(result, "-i	Ignore SIGINT")

        result = self.driver.shell(f"tee --help")
        self.driver.Assert.contains(result, "usage: tee [-ai] [file...]")
        self.driver.Assert.contains(result, "Copy stdin to each listed file, and also to stdout.")
        self.driver.Assert.contains(result, "-a	Append to files")
        self.driver.Assert.contains(result, "-i	Ignore SIGINT")

        # 创建测试文件
        Common.writeDateToFile("test/original.txt", "Hello, World!")
        self.driver.push_file(f"{os.getcwd()}/test", self.usr_workspace)

        Step("覆盖原有文件内容，内容为标准输出内容")
        data = "A gentle breeze carried the scent of fresh flowers through the meadow"
        self.driver.shell(f"echo {data} | tee {self.usr_workspace}/test/original.txt")
        result = self.driver.shell(f"cat {self.usr_workspace}/test/original.txt")
        self.driver.Assert.contains(result, "A gentle breeze carried the scent of fresh flowers through the meadow")
        self.driver.Assert.is_true("Hello, World!" not in result)

        Step("文件不存在")
        data = "A gentle breeze carried the scent of fresh flowers through the meadow"
        self.driver.shell(f"echo {data} | tee {self.usr_workspace}/test/original2.txt")
        result = self.driver.shell(f"cat {self.usr_workspace}/test/original2.txt")
        self.driver.Assert.contains(result, "A gentle breeze carried the scent of fresh flowers through the meadow")

        Step("追加内容到文件")
        data = "Every morning, she brewed a cup of coffee while watching the sunrise."
        self.driver.shell(f"echo {data} | tee -a {self.usr_workspace}/test/original.txt")
        result = self.driver.shell(f"cat {self.usr_workspace}/test/original.txt")
        self.driver.Assert.contains(result, "A gentle breeze carried the scent of fresh flowers through the meadow")
        self.driver.Assert.contains(result, "Every morning, she brewed a cup of coffee while watching the sunrise.")

        Step("将内容写入多个文件")
        data = "She painted vibrant murals on the walls of her cozy studio."
        self.driver.shell(f"echo {data} | tee {self.usr_workspace}/test/original.txt "
                          f"{self.usr_workspace}/test/original2.txt")
        result = self.driver.shell(f"cat {self.usr_workspace}/test/original.txt")
        self.driver.Assert.contains(result, "She painted vibrant murals on the walls of her cozy studio.")
        result = self.driver.shell(f"cat {self.usr_workspace}/test/original2.txt")
        self.driver.Assert.contains(result, "She painted vibrant murals on the walls of her cozy studio.")

        Step("重复执行")
        data = "She painted vibrant murals on the walls of her cozy studio."
        self.driver.shell(f"echo {data} | tee {self.usr_workspace}/test/original.txt "
                          f"{self.usr_workspace}/test/original2.txt")
        result = self.driver.shell(f"wc -l {self.usr_workspace}/test/original.txt")
        self.driver.Assert.contains(result, "1")

        result = self.driver.shell(f"cat {self.usr_workspace}/test/original.txt")
        self.driver.Assert.contains(result, "She painted vibrant murals on the walls of her cozy studio.")
        result = self.driver.shell(f"wc -l {self.usr_workspace}/test/original2.txt")
        self.driver.Assert.contains(result, "1")

        result = self.driver.shell(f"cat {self.usr_workspace}/test/original2.txt")
        self.driver.Assert.contains(result, "She painted vibrant murals on the walls of her cozy studio.")

    def teardown(self):
        self.driver.shell(f"rm -rf {self.usr_workspace}/test/")
        shutil.rmtree("test")
        Step("收尾工作")

