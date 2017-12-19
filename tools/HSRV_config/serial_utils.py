#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#
# Copyright © 2017 Benoit Rapidel <benoit.rapidel+devs@exmachina.fr>
#
# Distributed under terms of the MIT license.

"""

"""

from PyQt5.QtCore import Qt, QObject
from PyQt5.QtCore import pyqtSlot, pyqtSignal, QTimer

import threading
import time
import serial

class SerialThread(threading.Thread):
    def __init__(self, *args, **kwargs):
        self._exit_ev = threading.Event()
        self._newlines_ev = threading.Event()
        self._commands = list()
        self._lines = list()
        self._buffer = b''
        self._serial = None

        self.serial_kwargs = kwargs.pop('serial_kwargs')

        super().__init__(*args, **kwargs)

        try:
            self._serial = serial.Serial(timeout=0.5, **self.serial_kwargs)
            self._serial.blocking = True
        except KeyError as e:
            raise RuntimeError('Missing required argument: {}'.format(e))
        except serial.serialutil.SerialException as e:
            raise RuntimeError('Unable to open serial port.')

    def run(self, *args, **kwargs):
        tries = 0
        while not self._exit_ev.is_set():
            try:
                rdata = self._serial.read(self._serial.in_waiting)
                tries = 0
            except Exception as e:
                print('Unable to read: {!s}'.format(e))
                tries += 1

                if tries >= 5:
                    self._exit_ev.set()
                    print('Closing serial connection.')

            if not rdata:
                self._exit_ev.wait(0.5)
                continue
            self._buffer += (rdata)
            l = self._find_lines()
            if l:
                self._lines += l
                self._newlines_ev.set()

        if self._serial:
            self._serial.close()

    def _find_lines(self):
        packets = list()
        pos = self._buffer.find(b'\n')
        while pos > 0:
            pos = self._buffer.find(b'\n')
            if pos >= 0:
                packet, self._buffer = self._buffer[:pos + 1], self._buffer[pos + 1:]
                packets.append(packet)

        return packets

    def write(self, buf):
        if isinstance(buf, str):
            buf = buf.encode()

        self._serial.write(buf)

    def read(self):
        lines = tuple(self._lines)
        self._lines = list()
        self._newlines_ev.clear()
        return lines

    def readlines(self, timeout=5.0):
        self._newlines_ev.wait(timeout)
        l = self.read()
        return l

    def reset_input_buffer(self):
        self.read()

    def join(self, timeout=1.0):
        self._exit_ev.set()
        threading.Thread.join(self, timeout)
        self._serial.close()
        self._serial = None

    def close(self):
        self.join(5)


class QSerial(QObject):
    dataAvailable = pyqtSignal(tuple)

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._opened = False
        self._serialThread = None
        self._timer = 0

    def open(self, **kwargs):
        if self._opened:
            self.close()

        try:
            self._serialThread = SerialThread(**kwargs)
            self._serialThread.start()

            self._timer = self.startTimer(5)
        except RuntimeError as e:
            self.parent().setStatusMessage('Unable to launch serial: ' + str(e))
        else:
            self._opened = True
        finally:
            self._opened = False

    def send(self, data):
        if self._serialThread:
            self._serialThread.write(data)

    def close(self):
        if self._serialThread:
            self._serialThread.close()
            del self._serialThread
            self._opened = False

        if self._timer:
            self.killTimer(self._timer)

    def timerEvent(self, ev):
        if ev.timerId() == self._timer:
            if self._serialThread and self._serialThread.is_alive():
                try:
                    data = self._serialThread.read()
                except Exception as e:
                    self.parent().setStatusMessage('Unable to read data: {!s}'.format(e))
                if data:
                    self.dataAvailable.emit(data)
            else:
                self.parent().doDisconnect()
        else:
            super().timerEvent(ev)
