#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#
# Copyright © 2017 Benoit Rapidel <benoit.rapidel+devs@exmachina.fr>
#
# Distributed under terms of the MIT license.

"""

"""

HSRV_STATE = {
        0: 'STANDBY',
        1: 'NOT_CONFIGURED',
        2: 'CONFIGURED',
        3: 'READY',
        4: 'RUN',
        5: 'SLOWFEED',
        6: 'RESET',
        }


HSRV_ERROR = {
        0x01: 'EMERGENCY_STOP',
        0x02: 'NOT_CONFIGURED',
        0x04: 'INVALID_CONFIG',
        0x08: 'VFD_ERROR',
        }


def HSRV_get_status_text(status_code):
    if not isinstance(status_code, int):
        status_code = int(status_code, 16)
    return '0x{:x} - {}'.format(status_code, HSRV_STATE.get(status_code, 'UNKNOWN'))


def HSRV_get_error_text(error_code):
    if not isinstance(error_code, int):
        error_code = int(error_code, 16)

    if error_code != 0:
        errors = []
        for i, err in HSRV_ERROR.items():
            if (error_code & i) == i and i != 0:
                errors.append(err)

        errors.reverse()
        text = ', '.join(errors)
    else:
        text = 'NONE'
    return '0x{:x} - {}'.format(error_code, text)
