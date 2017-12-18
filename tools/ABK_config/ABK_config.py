import sys
from functools import partial

from PyQt5.QtWidgets import QApplication, QMainWindow, QFileDialog, QMessageBox
from PyQt5.QtWidgets import QPushButton, QComboBox, QSpinBox, QDoubleSpinBox, QLineEdit
from PyQt5.QtWidgets import QGraphicsView, QGraphicsScene, QGraphicsObject
from PyQt5.QtCore import pyqtSlot, pyqtSignal, QTimer

import PyQt5.uic as uic

import time
import os.path
from serial.tools import list_ports

from ABK.utils import ABK_STATE, ABK_ERROR, ABK_get_status_text, ABK_get_error_text
from GUI.utils import QGraphicsCircleItem, LinkedLines, Fabric, ABKFabric, OptionDialog
from GUI.utils import QTimeScene, QFabricScene
from serial_utils import SerialThread, QSerial

VERSION = '2.1'

class ABKConfig(QMainWindow):
    UIFILE = '.\ABK_config.ui'
    OPTIONSFILE = '.\ABK_config.ini'
    NTIMEPOINTS = 6
    BAUDRATES = (9600, 19200, 38400, 57600, 115200)

    configChanged = pyqtSignal(object)

    def __init__(self):
        super().__init__()

        OptionDialog.BAUDRATES = ABKConfig.BAUDRATES
        QTimeScene.NTIMEPOINTS = ABKConfig.NTIMEPOINTS

        self._maximum_speed = None
        self._speed_factor = None
        self._serial_baud = None
        self.initUi()

    def initUi(self):
        self.setWindowTitle('ABK Configuration tool')

        _path = os.path.dirname(os.path.realpath(__file__))
        self.OPTIONSFILE = _path + self.OPTIONSFILE
        self.main = uic.loadUi(_path + self.UIFILE)
        self.setCentralWidget(self.main)

        fileMenu = self.menuBar().addMenu('File')

        openAction = fileMenu.addAction('Load from file')
        saveAction = fileMenu.addAction('Save to file')
        fileMenu.addSeparator()
        quitAction = fileMenu.addAction('Quit')

        openAction.setShortcut('Ctrl+O')
        saveAction.setShortcut('Ctrl+S')
        quitAction.setShortcut('Ctrl+Q')
        quitAction.setShortcut('Alt+F4')

        openAction.triggered.connect(self.doOpen)
        saveAction.triggered.connect(self.doSave)
        quitAction.triggered.connect(self.doQuit)

        aboutAction = self.menuBar().addAction('About')
        aboutAction.triggered.connect(self.doAbout)

        # Define a QPalette to hilight out of bounds values
        # self._warningPalette = QPalette

        self.doOptionLoad()

        self.setStatusMessage('Disconnected')

        self.portCombo = self.main.findChild(QComboBox, 'portCombo')

        self._port_list = list_ports.comports()

        self.portCombo.addItem('')

        for p in self._port_list:
            self.portCombo.addItem(p[0])

        self.timeView = self.main.findChild(QGraphicsView, 'profileView')
        self.fabricView = self.main.findChild(QGraphicsView, 'fabricView')

        self.graphPoints = list()

        for i in range(self.NTIMEPOINTS + 1):
            self.graphPoints.append((
                self.main.findChild(QSpinBox, 'x%dSpinBox' % i),
                self.main.findChild(QSpinBox, 'y%dSpinBox' % i),))

        self.fabricWidth = self.main.findChild(QDoubleSpinBox, 'fabricWidthDoubleSpinBox')
        self.fabricHeight = self.main.findChild(QDoubleSpinBox, 'fabricHeightDoubleSpinBox')
        self.fabricSurface = self.main.findChild(QDoubleSpinBox, 'fabricSurfaceDoubleSpinBox')
        self.fabricLength = self.main.findChild(QDoubleSpinBox, 'fabricLengthDoubleSpinBox')

        self.fabricDensity = self.main.findChild(QDoubleSpinBox, 'fabricDensityDoubleSpinBox')
        self.fabricWeight = self.main.findChild(QDoubleSpinBox, 'fabricWeightDoubleSpinBox')

        self.cableLength = self.main.findChild(QDoubleSpinBox, 'cableLengthDoubleSpinBox')
        self.pickupPointX = self.main.findChild(QSpinBox, 'pickupPointXSpinBox')
        self.pickupPointY = self.main.findChild(QSpinBox, 'pickupPointYSpinBox')

        self.nominalSpeed = self.main.findChild(QDoubleSpinBox, 'nominalSpeedDoubleSpinBox')
        self.startDelay = self.main.findChild(QSpinBox, 'startDelaySpinBox')
        self.accelTime = self.main.findChild(QSpinBox, 'accelerationTimeSpinBox')
        self.acceleration = self.main.findChild(QDoubleSpinBox, 'accelerationDoubleSpinBox')
        self.decelTime = self.main.findChild(QSpinBox, 'decelerationTimeSpinBox')
        self.deceleration = self.main.findChild(QDoubleSpinBox, 'decelerationDoubleSpinBox')

        self.fabric = ABKFabric()
        self.doFabricConfig()

        self.initPreview()

        self.disableActions()

        self.initSerialCommands()

        self.show()

    def initPreview(self):
        self.timeScene = QTimeScene(self.timeView)
        self.fabricScene = QFabricScene(self.fabricView)

        self.timeScene.timeLine.pointXChanged.connect(self.setGraphPointValue)

        for i, gp in enumerate(self.graphPoints):
            if gp[0] is not None:
                gp[0].valueChanged[int].connect(partial(self.timeScene.timeLine.setPointX, i))
                gp[0].setKeyboardTracking(False)

            if gp[1] is not None:
                gp[1].valueChanged[int].connect(partial(self.timeScene.timeLine.setPointY, i))

        self.fabricWidth.valueChanged[float].connect(self.fabricScene.fabricPreview.setWidth)
        self.fabricWidth.valueChanged[float].connect(self.fabric.setWidth)
        self.fabricHeight.valueChanged[float].connect(self.fabricScene.fabricPreview.setHeight)
        self.fabricHeight.valueChanged[float].connect(self.fabric.setHeight)

        self.fabricDensity.valueChanged[float].connect(self.fabric.setDensity)

        self.cableLength.valueChanged[float].connect(self.fabric.setCableLength)
        self.pickupPointX.valueChanged[int].connect(self.fabricScene.fabricPreview.setPickupPointX)
        self.pickupPointX.valueChanged[int].connect(self.fabric.setPickupPointX)
        self.pickupPointY.valueChanged[int].connect(self.fabricScene.fabricPreview.setPickupPointY)
        self.pickupPointY.valueChanged[int].connect(self.fabric.setPickupPointY)

        self.fabric.surfaceChanged.connect(self.fabricSurface.setValue)
        self.fabric.lengthChanged.connect(self.fabricLength.setValue)

        self.fabric.weightChanged.connect(self.fabricWeight.setValue)

        self.nominalSpeed.valueChanged[float].connect(self.fabric.setNominalSpeed)
        self.accelTime.valueChanged[int].connect(self.fabric.setAccelTime)
        self.decelTime.valueChanged[int].connect(self.fabric.setDecelTime)
        self.startDelay.valueChanged[int].connect(self.fabric.setStartDelay)

        self.fabric.accelerationChanged.connect(self.acceleration.setValue)
        self.fabric.decelerationChanged.connect(self.deceleration.setValue)

        self.fabric.profileChanged.connect(self.setCurrentConfig)
        self.configChanged.connect(self.doFabricConfig)

        self.fabric.surfaceChanged.connect(self._surfaceCheck)
        self.fabric.weightChanged.connect(self._weightCheck)


    def initSerialCommands(self):
        self._serial_port = None
        self._serial_object = None
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
        if not self._serial_baud:
            self._serial_baud = 115200

        if not self._serial_port or not self._serial_baud:
            self.setStatusMessage('Invalid parameters')
            return

        self.setStatusMessage('Connecting...')
        self.main.findChild(QPushButton, 'connectButton').setText('Connecting')

        k = {
            'port': self._serial_port,
            'baudrate': self._serial_baud,
        }

        try:
            self._serial_object = QSerial(self)
            self._serial_object.open(serial_kwargs=k)
            self._serial_object.dataAvailable.connect(self.parseSerialData)

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
            self._serial_object.send(data)
        except Exception as e:
            self.setStatusMessage('Unable to send data: {!s}'.format(e))

    def doDisconnect(self):
        self.doDeviceSlowfeed(0, 0)

        if self._serial_object:
            self._serial_object.close()
            del self._serial_object
        self._serial_object = None
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

        surface = self.fabric.surface
        weight = self.fabric.weight

        surface_status = self._surfaceCheck(surface, paint=False)
        weight_status = self._weightCheck(weight, paint=False)
        approve = QMessageBox.Yes

        if not surface_status or not weight_status:
            approve = QMessageBox.question(self,'Confirmation',
                "Surface and/or weight exceeds maximum capicity, do you want to continue (not recommended)?",
                QMessageBox.Yes | QMessageBox.No)

        if approve == QMessageBox.Yes:
            self.setStatusMessage('Writting config to device...')
            self.disableActions()

            i, j = 0, len(self.getCurrentConfig())
            for k, v in self.getCurrentConfig().items():
                data = 'set {} {:d}'.format(k, v).encode() + b'\n'
                QTimer.singleShot(500*i, partial(self.setStatusMessage, 'Writting config to device... {:d}%'.format(int(i/j*100))))
                QTimer.singleShot(500*i, partial(self.serialSend, data))

                i += 1

            QTimer.singleShot((500*i) + 100, partial(self.serialSend, b'save\n'))
            QTimer.singleShot((500*i) + 200, self.doDeviceReset)
            self.setStatusMessage('Config written to device.')
            QMessageBox.information(self,'Notice',
                "Parameters saved to device")
            self.enableActions()
        else:
            return

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
        '''ABK configuration tool — version: v''' + str(VERSION) + '''

Copyright ExMachina 2017 — Released under MIT license

Source code cound be found on Github at
https://github.com/exmachina-dev/ABK-firmware/tree/master/tools
        ''', QMessageBox.Ok)

    def doOptionLoad(self):
        try:
            with open(self.OPTIONSFILE, 'r') as f:
                cfg = {}
                for l in f.readlines():
                    try:
                        k, v = l.split(' = ')
                        cfg[k] = v
                    except Exception:
                        pass

            self.setCurrentOptions(cfg)
        except OSError:
            self.doOptionSave()

    def doOptionSave(self):
        with open(self.OPTIONSFILE, 'w') as f:
            for k, v in self.getCurrentOptions().items():
                f.write('{} = {}\n'.format(k, v))

    def doFabricConfig(self, cfg=None):
        if not cfg:
            cfg = {}
        self.fabric.maximum_speed = float(cfg.get('maximum_speed',
            self._maximum_speed or 7.5))
        self.fabric.speed_factor = float(cfg.get('speed_factor',
            self._speed_factor or 3.5))
        self.fabric.minimum_acceltime = float(cfg.get('minimum_acceltime',
            self._minimum_acceltime or 200.0))

        self.nominalSpeed.setMinimum(0)
        self.nominalSpeed.setMaximum(self.fabric.maximum_speed)
        self.accelTime.setMinimum(self.fabric.minimum_acceltime)
        self.decelTime.setMinimum(self.fabric.minimum_acceltime)

    def doQuit(self):
        self.doOptionSave()
        QApplication.quit()

    def setCurrentConfig(self, cfg):
        ind = {
            'start': (1, 0),
            'p1.time': (2, 0),
            'p1.speed': (2, 1),
            'p2.time': (3, 0),
            'p2.speed': (3, 1),
            'p3.time': (4, 0),
            'p3.speed': (4, 1),
            'stop': (5, 0)
        }

        for k, v in cfg.items():
            if k in ind.keys():
                self.graphPoints[ind[k][0]][ind[k][1]].setValue(int(v))
            else:
                self.setStatusMessage('Unrecognized config key: {}'.format(k))

    def getCurrentConfig(self):
        cfg = {}
        cfg['start'] = int(self.graphPoints[1][0].value())
        cfg['p1.time'] = int(self.graphPoints[2][0].value())
        cfg['p1.speed'] = int(self.graphPoints[2][1].value())
        cfg['p2.time'] = int(self.graphPoints[3][0].value())
        cfg['p2.speed'] = int(self.graphPoints[3][1].value())
        cfg['p3.time'] = int(self.graphPoints[4][0].value())
        cfg['p3.speed'] = int(self.graphPoints[4][1].value())
        cfg['stop'] = int(self.graphPoints[5][0].value())

        return cfg

    def setCurrentOptions(self, cfg):
        opts = ('serial_baud', 'maximum_speed', 'speed_factor',
                'maximum_surface', 'maximum_weight', 'minimum_acceltime', )

        for k, v in cfg.items():
            if k in opts:
                setattr(self, '_' + k, v)

        self._serial_baud = float(self._serial_baud)
        self._speed_factor = float(self._speed_factor)
        self._maximum_speed = float(self._maximum_speed)
        self._maximum_surface = float(self._maximum_surface)
        self._maximum_weight = float(self._maximum_weight)
        self._minimum_acceltime = float(self._minimum_acceltime)

    def getCurrentOptions(self):
        cfg = {}
        cfg['serial_baud'] = self._serial_baud or 115200
        cfg['maximum_speed'] = self._maximum_speed or 7.5
        cfg['speed_factor'] = self._speed_factor or 3.5
        cfg['maximum_surface'] = self._maximum_surface or 100.0
        cfg['maximum_weight'] = self._maximum_weight or 15
        cfg['minimum_acceltime'] = self._minimum_acceltime or 200.0

        return cfg

    def serialPort(self, text):
        if text:
            self._serial_port = text

    def serialBaud(self, text):
        if text:
            self._serialBaud = int(text)

    def setGraphPointValue(self, point, value):
        try:
            if self.graphPoints[point][0]:
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

        if self.connectedVersion and self.connectedVersion[0] >= 1:
            self.main.findChild(QPushButton, 'slowfeedForwardButton').setEnabled(True)
            self.main.findChild(QPushButton, 'slowfeedRewindButton').setEnabled(True)
            self.main.findChild(QPushButton, 'statusButton').setEnabled(True)

    def _surfaceCheck(self, surface, paint=True):
        is_ok = False
        if 0 <= surface <= self._maximum_surface:
            is_ok = True

        if paint:
            if is_ok:
                self.fabricSurface.setStyleSheet("")
            else:
                self.fabricSurface.setStyleSheet("background-color : red ; color : black")
        
        return is_ok
        

    def _weightCheck(self, weight, paint=True):
        is_ok = False
        if 0 <= weight <= self._maximum_weight:
            is_ok = True

        if paint:
            if is_ok:
                self.fabricWeight.setStyleSheet("")
            else:
                self.fabricWeight.setStyleSheet("background-color : red ; color : black")
        
        return is_ok


if __name__ == '__main__':

    app = QApplication(sys.argv)

    w = ABKConfig()

    sys.exit(app.exec_())
