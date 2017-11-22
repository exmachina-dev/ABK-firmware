import sys
from functools import partial

from PyQt5.QtWidgets import QApplication, QMainWindow, QFileDialog, QMessageBox
from PyQt5.QtWidgets import QDialog, QVBoxLayout, QDialogButtonBox
from PyQt5.QtWidgets import QPushButton, QComboBox, QSpinBox, QLineEdit
from PyQt5.QtWidgets import QGraphicsView, QGraphicsScene, QGraphicsObject
from PyQt5.QtWidgets import QGraphicsLineItem, QGraphicsEllipseItem
from PyQt5.QtGui import QPen, QColor
from PyQt5.QtCore import Qt, QObject, QPointF, QLineF, QRectF
from PyQt5.QtCore import pyqtSlot, pyqtSignal, QTimer

import PyQt5.uic as uic

import serial
from serial.tools import list_ports

import threading
import time
import os.path

ABK_STATE = {
        0: 'STANDBY',
        1: 'NOT_CONFIGURED',
        2: 'CONFIGURED',
        3: 'READY',
        4: 'RUN',
        5: 'SLOWFEED',
        6: 'RESET',
        }

ABK_ERROR = {
        0x01: 'EMERGENCY_STOP',
        0x02: 'NOT_CONFIGURED',
        0x04: 'INVALID_CONFIG',
        0x08: 'VFD_ERROR',
        }

def ABK_get_status_text(status_code):
    if not isinstance(status_code, int):
        status_code = int(status_code, 16)
    return '0x{:x} - {}'.format(status_code, ABK_STATE.get(status_code, 'UNKNOWN'))

def ABK_get_error_text(error_code):
    if not isinstance(error_code, int):
        error_code = int(error_code, 16)

    if error_code != 0:
        errors = []
        for i, err in ABK_ERROR.items():
            if (error_code & i) == i and i != 0:
                errors.append(err)

        errors.reverse()
        text = ', '.join(errors)
    else:
        text = 'NONE'
    return '0x{:x} - {}'.format(error_code, text)


class QGraphicsCircleItem(QGraphicsEllipseItem):
    def __init__(self, *args, **kwargs):
        self.diameter = kwargs.pop('diameter', 2)

        super().__init__(*args, **kwargs)

        self.setRect(0.0, 0.0, self.diameter, self.diameter)

    def setCenter(self, p):
        self.prepareGeometryChange()

        r = self.rect()
        r.moveCenter(p)
        self.setRect(r)


class LinkedLines(QGraphicsObject):
    pointXChanged = pyqtSignal(int, int)

    def __init__(self, *args, **kwargs):
        self.npoints = kwargs.pop('npoints', 2)
        self.xFactor = kwargs.pop('xfactor', None)
        self.yFactor = kwargs.pop('yfactor', None)

        super().__init__(*args, **kwargs)

        self._lines = []
        self._linesF = []
        self._points = []
        self._pointsF = []

        self._circleDia = 2.0
        self._linePen = QPen(QColor(135, 208, 80))
        self._linePen.setCapStyle(Qt.RoundCap)
        self._circlePen = QPen(QColor(66, 66, 66))

        self._linePen.setCapStyle(Qt.RoundCap)
        self._circlePen.setCapStyle(Qt.RoundCap)

        self._linePen.setWidth(1)
        self._circlePen.setWidth(2)

        for i in range(self.npoints - 1):
            l = QGraphicsLineItem()
            l.setPen(self._linePen)
            self._lines.append(l)
            self._linesF.append(QLineF())

        for i in range(self.npoints):
            self._pointsF.append(QPointF())

        for i in range(1, self.npoints - 1):
            e = QGraphicsCircleItem(diameter=self._circleDia)
            e.setPen(self._circlePen)
            self._points.append(e)

        for i in range(self.npoints):
            self.setPointX(i, 0)
            self.setPointY(i, 0)

        self.setPointX(0, -10)
        self.setPointY(0, 0)

        self.setPointX(self.npoints - 1, 1000000)
        self.setPointY(self.npoints - 1, 0)

    def setPointX(self, p, x):
        self.prepareGeometryChange()

        if self.xFactor:
            x = x * self.xFactor

        point = self._pointsF[p]
        point.setX(float(x))

        if p < self.npoints - 1:
            self._linesF[p].setP1(point)
            self._lines[p].setLine(self._linesF[p])
        if p > 0:
            self._linesF[p - 1].setP2(point)
            self._lines[p - 1].setLine(self._linesF[p - 1])

        if p > 0 and p < self.npoints - 1:
            self._points[p - 1].setCenter(point)

        self.updateAdjacentPointsX(p, x)

    def setPointY(self, p, y):
        self.prepareGeometryChange()

        if self.yFactor:
            y = y * self.yFactor

        point = self._pointsF[p]
        point.setY(float(-y))

        if p < self.npoints - 1:
            self._linesF[p].setP1(point)
            self._lines[p].setLine(self._linesF[p])
        if p > 0:
            self._linesF[p - 1].setP2(point)
            self._lines[p - 1].setLine(self._linesF[p - 1])

        if p > 0 and p < self.npoints - 1:
            self._points[p - 1].setCenter(point)

    def updateAdjacentPointsX(self, p, x):
        # Update next points if value exceed next point value
        if p < self.npoints - 1:
            if x > self._pointsF[p + 1].x():
                if self.xFactor:
                    x = x / self.xFactor
                self.setPointX(p + 1, x)
                self.pointXChanged.emit(p + 1, x)

        # Update previous points if previous point value exceed value
        if p > 0:
            if x < self._pointsF[p - 1].x():
                if self.xFactor:
                    x = x / self.xFactor
                self.setPointX(p - 1, x)
                self.pointXChanged.emit(p - 1, x)

    def paint(self, *args, **kwargs):
        for l in self._lines:
            l.paint(*args, **kwargs)
        for p in self._points:
            p.paint(*args, **kwargs)

    def boundingRect(self):
        rects = [l.boundingRect().getRect() for l in self._lines]
        x = [r[0] for r in rects]
        y = [r[1] for r in rects]
        w = [r[2] for r in rects]
        h = [r[3] for r in rects]

        minx = min(x) - self._circleDia
        miny = min(y) - self._circleDia

        rw, rh = [], []
        for r in range(len(self._lines)):
            rw.append((x[r] - minx) + w[r])
            rh.append((y[r] - miny) + h[r])

        return QRectF(minx, miny, max(rw) + self._circleDia, max(rh) + self._circleDia)


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

class OptionDialog(QDialog):
    def __init__(self, parent = None):
        super().__init__(parent)

        layout = QVBoxLayout(self)

        self.baudrateCombo = QComboBox(self)

        self.baudrateCombo.addItem('')
        for b in self.BAUDRATES:
            self.baudrateCombo.addItem(str(b))

        layout.addWidget(self.baudrateCombo)

        # OK and Cancel buttons
        self.buttons = QDialogButtonBox(
            QDialogButtonBox.Ok | QDialogButtonBox.Cancel,
            Qt.Horizontal, self)
        layout.addWidget(self.buttons)

        self.buttons.accepted.connect(self.accept)
        self.buttons.rejected.connect(self.reject)

    # get baudrate from dialog
    def baudrate(self):
        return self.baudrateCombo.currentText()

    # static method to create the dialog and return baudrate
    @staticmethod
    def getBaudrate(parent = None):
        dialog = OptionDialog(parent)
        result = dialog.exec_()
        baudrate = dialog.baudrate()
        return (baudrate, result == QDialog.Accepted)


class ABKConfig(QMainWindow):
    UIFILE = '.\ABK-config.ui'
    NTIMEPOINTS = 5
    BAUDRATES = (9600, 19200, 38400, 57600, 115200)

    def __init__(self):
        super().__init__()

        OptionDialog.BAUDRATES = ABKConfig.BAUDRATES
        self.initUi()

    def initUi(self):
        self.setWindowTitle('ABK Configuration tool')
        _path = os.path.dirname(os.path.realpath(__file__))
        self.main = uic.loadUi(_path + self.UIFILE)
        self.setCentralWidget(self.main)

        fileMenu = self.menuBar().addMenu('File')

        openAction = fileMenu.addAction('Load from file')
        saveAction = fileMenu.addAction('Save to file')
        fileMenu.addSeparator()
        optionAction = fileMenu.addAction('Options')
        fileMenu.addSeparator()
        quitAction = fileMenu.addAction('Quit')

        openAction.setShortcut('Ctrl+O')
        saveAction.setShortcut('Ctrl+S')
        quitAction.setShortcut('Ctrl+Q')

        openAction.triggered.connect(self.doOpen)
        saveAction.triggered.connect(self.doSave)
        optionAction.triggered.connect(self.doOption)
        quitAction.triggered.connect(QApplication.quit)

        aboutAction = self.menuBar().addAction('About')
        aboutAction.triggered.connect(self.doAbout)

        self.setStatusMessage('Disconnected')

        self.portCombo = self.main.findChild(QComboBox, 'portCombo')

        self._port_list = list_ports.comports()

        self.portCombo.addItem('')

        for p in self._port_list:
            self.portCombo.addItem(p[0])

        self.timeView = self.main.findChild(QGraphicsView, 'timePreview')

        self.graphPoints = list()

        for i in range(self.NTIMEPOINTS + 1):
            self.graphPoints.append((
                self.main.findChild(QSpinBox, 'x%dSpinBox' % i),
                self.main.findChild(QSpinBox, 'y%dSpinBox' % i),))

        self.initPreview()

        self.disableActions()

        self.initSerialCommands()

        self.show()

    def initPreview(self):
        self.timeScene = QGraphicsScene()

        self.timeView.setScene(self.timeScene)

        self.timeView.setSceneRect(-3.0, -105, 66.0, 110.0)
        self.timeView.scale(2.0, 2.0)

        self.timeLine = LinkedLines(npoints=self.NTIMEPOINTS + 1, xfactor=0.004)

        self.timeScene.addItem(self.timeLine)

        self.timeLine.pointXChanged.connect(self.setGraphPointValue)

        for i, gp in enumerate(self.graphPoints):
            if gp[0] is not None:
                gp[0].valueChanged[int].connect(partial(self.timeLine.setPointX, i))
                gp[0].setKeyboardTracking(False)

            if gp[1] is not None:
                gp[1].valueChanged[int].connect(partial(self.timeLine.setPointY, i))

    def initSerialCommands(self):
        self._serialPort = None
        self._serialBaud = None
        self._serialObject = None
        self._connected = False
        self._connectedVersion = None

        self.main.findChild(QPushButton, 'connectButton').clicked.connect(self.doConnect)

        self.main.findChild(QPushButton, 'slowfeedRewindButton').pressed.connect(partial(self.doDeviceSlowfeed, 0, 1))
        self.main.findChild(QPushButton, 'slowfeedForwardButton').pressed.connect(partial(self.doDeviceSlowfeed, 1, 1))
        self.main.findChild(QPushButton, 'slowfeedRewindButton').released.connect(partial(self.doDeviceSlowfeed, 0, 0))
        self.main.findChild(QPushButton, 'slowfeedForwardButton').released.connect(partial(self.doDeviceSlowfeed, 1, 0))

        self.main.findChild(QPushButton, 'getConfigButton').clicked.connect(self.doDeviceGet)
        self.main.findChild(QPushButton, 'saveConfigButton').clicked.connect(self.doDeviceSave)
        self.main.findChild(QPushButton, 'resetButton').clicked.connect(self.doDeviceReset)
        self.main.findChild(QPushButton, 'eraseButton').clicked.connect(self.doDeviceErase)
        self.main.findChild(QComboBox, 'portCombo').currentTextChanged.connect(self.serialPort)

        self.main.findChild(QPushButton, 'statusButton').clicked.connect(self.doDeviceStatus)

    def doConnect(self, *args):
        if not self._serialBaud:
            self._serialBaud = 115200

        if not self._serialPort or not self._serialBaud:
            self.setStatusMessage('Invalid parameters')
            return

        self.setStatusMessage('Connecting...')
        self.main.findChild(QPushButton, 'connectButton').setText('Connecting')

        k = {
            'port': self._serialPort,
            'baudrate': self._serialBaud,
        }

        try:
            self._serialObject = QSerial(self)
            self._serialObject.open(serial_kwargs=k)
            QTimer.singleShot(500, partial(self._serialObject.dataAvailable.connect, self.parseSerialData))
            QTimer.singleShot(500, partial(self.serialSend, b'version\n'))
        except Exception as e:
            self.setStatusMessage('Unable to connect: {!s}'.format(e))
            self.doDisconnect()

    @pyqtSlot(tuple)
    def parseSerialData(self, data):
        if not len(data):
            self.setStatusMessage('No response from serial device')
            return

        try:
            if data[0] == b'version\n':
                if len(data) > 1:
                    version = data[1]
                else:
                    version = None

                if version and version != b'\0':
                    self.setStatusMessage('Connected to ' + version.decode())
                    self.main.findChild(QPushButton, 'connectButton').clicked.connect(self.doDisconnect)
                    self.main.findChild(QPushButton, 'connectButton').setText('Disconnect')
                    self._connected = True
                    QTimer.singleShot(50, self.doDeviceGet)
                    QTimer.singleShot(500, self.doDeviceStatus)
                    try:
                        self._connectedVersion = [int(x) for x in version.decode().strip('v').split('.')]
                    except Exception:
                        self.doDisconnect()
                        self.setStatusMessage('Unable to connect to device.')

                    self.enableActions()
                else:
                    self.setStatusMessage('Unable to connect.')
            elif data[0] == b'set\n':
                pass
            elif data[0] == b'get\n':
                cfg = {}
                for line in data:
                    d = line.decode().strip('\r\n')
                    d = d.split()
                    if len(d) > 1:
                        cfg[d[0]] = int(d[1])

                self.setCurrentConfig(cfg)
                self.setStatusMessage('Config read from device.')
            elif data[0] == b'save\n':
                pass
            elif data[0] == b'reset\n':
                pass
            elif data[0] == b'status\n':
                d = data[1].decode().strip('\r\n').split()
                self.main.findChild(QLineEdit, 'statusLineEdit').setText(
                        ABK_get_status_text(int(d[1], 16)))
                self.main.findChild(QLineEdit, 'errorLineEdit').setText(
                        ABK_get_error_text(int(d[3], 16)))
            else:
                print(' '.join([x.decode() for x in data]))
        except UnicodeDecodeError:
            self.setStatusMessage('Unable to decode message. Try another baud setting.')
        # except AttributeError:
        #     self.setStatusMessage('Empty response from device.')
        except IndexError:
            self.setStatusMessage('Bad response from device.')

    @property
    def connectedVersion(self):
        if hasattr(self, '_connectedVersion') and self._connectedVersion:
            return self._connectedVersion
        return None

    def serialSend(self, data):
        try:
            self._serialObject.send(data)
        except Exception as e:
            self.setStatusMessage('Unable to send data: {!s}'.format(e))

    def doDisconnect(self):
        self.doDeviceSlowfeed(0, 0)

        if self._serialObject:
            self._serialObject.close()
            del self._serialObject
        self._serialObject = None
        self._connected = False
        self._connectedVersion = None
        self.setStatusMessage('Disconnected.')
        self.main.findChild(QPushButton, 'connectButton').clicked.connect(self.doConnect)
        self.main.findChild(QPushButton, 'connectButton').setText('Connect')
        self.disableActions()

    def doDeviceSlowfeed(self, direction=0, state=False):
        _cmd = b''

        if state:
            _dir = b'forward'
            if direction != 0:
                _dir = b'rewind'

            _cmd = b'slowfeed ' + _dir + b'\n'
        else:
            _cmd = b'slowfeed stop\n'
        print(_cmd.decode())
        self.serialSend(_cmd)

    def doDeviceGet(self):
        self.serialSend(b'get\n')

    def doDeviceSave(self):
        if not self._connected:
            self.setStatusMessage('Not connected to device')
            return

        self.setStatusMessage('Writting config to device...')

        i, j = 0, len(self.getCurrentConfig())
        for k, v in self.getCurrentConfig().items():
            data = 'set {} {:d}'.format(k, v).encode() + b'\n'
            self.setStatusMessage('Writting config to device... {:d}%'.format(int(i/j*100)))
            self.serialSend(data)
            time.sleep(0.5)

            i += 1

        self.serialSend(b'save\n')
        QTimer.singleShot(100, self.doDeviceReset)
        self.setStatusMessage('Config written to device.')

    def doDeviceReset(self):
        if self.connectedVersion and self.connectedVersion[0] >= 2 and self.connectedVersion[1] >= 0:
            self.serialSend(b'reset\n')
            self.doDisconnect()
            QTimer.singleShot(2000, self.doConnect)
            self.setStatusMessage('Device reset.')
        else:
            self.setStatusMessage('This version doesn\'t support reset, you nedd to powercycle the unit.')

    def doDeviceErase(self):
        confirm = QMessageBox.question(self, 'Confirmation',
            "Are you sure?\nThis will erase the memory on the device.", QMessageBox.Yes |
            QMessageBox.No, QMessageBox.No)

        if confirm == QMessageBox.Yes:
            self.serialSend(b'erase\n')
            self.setStatusMessage('Device memory erased.')

    def doDeviceStatus(self):
        self.serialSend(b'status\n')

    def doOpen(self):
        readFile, _ = QFileDialog.getOpenFileName(self, 'Load config from', '', 'Config file (*.cfg)')

        if not readFile:
            return

        with open(readFile, 'r') as f:
            cfg = {}
            for l in f.readlines():
                k, v = l.split(' = ')
                cfg[k] = int(v)

        self.setCurrentConfig(cfg)

    def doSave(self):
        writeFile, _ = QFileDialog.getSaveFileName(self, 'Save config to', '', 'Config file (*.cfg)')

        if not writeFile:
            return

        with open(writeFile, 'w') as f:
            for k, v in self.getCurrentConfig().items():
                f.write('{} = {}\n'.format(k, v))

    def doAbout(self):
        about = QMessageBox.information(self, 'About',
        '''ABK configuration tool — version: 1.0

Copyright ExMachina 2017 — Released under MIT license

Source code cound be found on Github at
https://github.com/exmachina-dev/ABK-firmware/tree/master/tools"
        ''', QMessageBox.Ok)

    def doOption(self):
        b, o = OptionDialog.getBaudrate(self.main)
        if o:
            self.serialBaud(b)

    def setCurrentConfig(self, cfg):
        ind = {
            'start': (1, 0),
            'p1.time': (2, 0),
            'p1.speed': (2, 1),
            'p2.time': (3, 0),
            'p2.speed': (3, 1),
            'stop': (4, 0)
        }

        for k, v in cfg.items():
            if k in ind.keys():
                self.graphPoints[ind[k][0]][ind[k][1]].setValue(int(v))
                print(k, ind[k], v)
            else:
                self.setStatusMessage('Unrecognized config key: {}'.format(k))

    def getCurrentConfig(self):
        cfg = {}
        cfg['start'] = int(self.graphPoints[1][0].value())
        cfg['p1.time'] = int(self.graphPoints[2][0].value())
        cfg['p1.speed'] = int(self.graphPoints[2][1].value())
        cfg['p2.time'] = int(self.graphPoints[3][0].value())
        cfg['p2.speed'] = int(self.graphPoints[3][1].value())
        cfg['stop'] = int(self.graphPoints[4][0].value())

        return cfg

    def serialPort(self, text):
        if text:
            self._serialPort = text

    def serialBaud(self, text):
        if text:
            self._serialBaud = int(text)

    def setGraphPointValue(self, point, value):
        try:
            self.graphPoints[point][0].setValue(value)
        except IndexError:
            pass

    def setStatusMessage(self, msg):
        if self.connectedVersion:
            v = '.'.join([str(x) for x in self.connectedVersion])
            self.main.statusBar.showMessage('Abrakabuki v{} - {}'.format(v, msg))
        else:
            self.main.statusBar.showMessage(msg)

    def disableActions(self):
        self.main.findChild(QPushButton, 'slowfeedForwardButton').setEnabled(False)
        self.main.findChild(QPushButton, 'slowfeedRewindButton').setEnabled(False)
        self.main.findChild(QPushButton, 'getConfigButton').setEnabled(False)
        self.main.findChild(QPushButton, 'saveConfigButton').setEnabled(False)
        self.main.findChild(QPushButton, 'resetButton').setEnabled(False)
        self.main.findChild(QPushButton, 'eraseButton').setEnabled(False)
        self.main.findChild(QPushButton, 'statusButton').setEnabled(False)

    def enableActions(self):
        self.main.findChild(QPushButton, 'getConfigButton').setEnabled(True)
        self.main.findChild(QPushButton, 'saveConfigButton').setEnabled(True)

        if self.connectedVersion and self.connectedVersion[0] >= 2 and self.connectedVersion[1] >= 0:
            self.main.findChild(QPushButton, 'resetButton').setEnabled(True)

        self.main.findChild(QPushButton, 'eraseButton').setEnabled(True)

        print(self.connectedVersion)
        if self.connectedVersion and self.connectedVersion[0] >= 1:
            self.main.findChild(QPushButton, 'slowfeedForwardButton').setEnabled(True)
            self.main.findChild(QPushButton, 'slowfeedRewindButton').setEnabled(True)
            self.main.findChild(QPushButton, 'statusButton').setEnabled(True)


if __name__ == '__main__':

    app = QApplication(sys.argv)

    w = ABKConfig()

    sys.exit(app.exec_())
