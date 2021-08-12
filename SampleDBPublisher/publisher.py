# Copyright (c) 2021 Intel Corporation.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
"""Database publisher example
"""

import time
import eii.msgbus as mb
import os
import json
from distutils.util import strtobool
import cfgmgr.config_manager as cfg
from util.util import Util


def start_publisher():
    publisher = None

    try:
        ctx = cfg.ConfigMgr()
        if ctx.get_num_publishers() == -1:
            raise "No publisher instances found, exiting..."
        pub_ctx = ctx.get_publisher_by_index(0)
        msgbus_cfg = pub_ctx.get_msgbus_config()

        print('[INFO] Initializing message bus context')
        msgbus_pub = mb.MsgbusContext(msgbus_cfg)

        topics = pub_ctx.get_topics()
        print(f'[INFO] Initializing publisher for topic \'{topics[0]}\'')
        publisher = msgbus_pub.new_publisher(topics[0])

        app_cfg = ctx.get_app_config()
        print(f'App Config is  \'{app_cfg}\'')

        print('[INFO] Running...')
        while True:
            blob = b'\x22' * 10
            meta = {
                'dataPersist': 'true',
                'test_msg': 1234 
            }

            publisher.publish((meta, blob,))
            print(f'[INFO] Msg published by publisher :  \'{meta}\'')
            time.sleep(int(app_cfg["loop_interval"]))
    except KeyboardInterrupt:
        print('[INFO] Quitting...')
    finally:
        if publisher is not None:
            publisher.close()
