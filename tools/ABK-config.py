import sys
from functools import partial

from PyQt5.QtWidgets import QApplication, QMainWindow, QGroupBox, QComboBox, QDoubleSpinBox
from PyQt5.QtWidgets import QGraphicsView, QGraphicsScene, QGraphicsItem, QGraphicsLineItem, QGraphicsEllipseItem
from PyQt5.QtGui import QPen, QColor
from PyQt5.QtCore import Qt, QPointF, QLineF, QRectF

import PyQt5.uic as uic
from serial.tools import list_ports


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

class LinkedLines(QGraphicsItem):
    def __init__(self, *args, **kwargs):
        self.npoints = kwargs.pop('npoints', 2)

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

        self.setPointX(self.npoints - 1, 250)
        self.setPointY(self.npoints - 1, 0)

    def setPointX(self, p, x):
        self.prepareGeometryChange()

        point = self._pointsF[p]
        point.setX(float(x))

        if p < self.npoints - 1:
            self._linesF[p].setP1(point)
            self._lines[p].setLine(self._linesF[p])
        if p > 0:
            self._linesF[p-1].setP2(point)
            self._lines[p-1].setLine(self._linesF[p-1])

        if p > 0 and p < self.npoints - 1:
            self._points[p-1].setCenter(point)

    def setPointY(self, p, y):
        self.prepareGeometryChange()

        point = self._pointsF[p]
        point.setY(float(-y))

        if p < self.npoints - 1:
            self._linesF[p].setP1(point)
            self._lines[p].setLine(self._linesF[p])
        if p > 0:
            self._linesF[p-1].setP2(point)
            self._lines[p-1].setLine(self._linesF[p-1])

        if p > 0 and p < self.npoints - 1:
            self._points[p-1].setCenter(point)

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

        minx = min(x)
        miny = min(y)

        rw, rh = [], []
        for r in range(len(self._lines)):
            rw.append((x[r] - minx) + w[r])
            rh.append((y[r] - miny) + h[r])

        return QRectF(minx, miny, max(rw), max(rh))


class ABKConfig(QMainWindow):
    UIFILE = '.\ABK-config.ui'
    NTIMEPOINTS = 5
    BAUDRATES = (9600, 19200, 38400, 57600, 115200)

    def __init__(self):
        super().__init__()

        self.initUi()

    def initUi(self):
        self.setWindowTitle('ABK Configuration tool')
        self.main = uic.loadUi(self.UIFILE)
        self.setCentralWidget(self.main)

        self.main.statusBar.showMessage('Disconnected')

        self.portCombo = self.main.findChild(QComboBox, 'portCombo')
        self.baudrateCombo = self.main.findChild(QComboBox, 'baudrateCombo')

        self._port_list = list_ports.comports()

        self.portCombo.addItem('')

        for p in self._port_list:
            self.portCombo.addItem(p[0])

        self.baudrateCombo.addItem('')
        for b in self.BAUDRATES:
            self.baudrateCombo.addItem(str(b))

        self.timeView = self.main.findChild(QGraphicsView, 'timePreview')

        self.graphPoints = list()

        for i in range(self.NTIMEPOINTS + 1):
            self.graphPoints.append((
                self.main.findChild(QDoubleSpinBox, 'x%dDoubleSpinBox' % i),
                self.main.findChild(QDoubleSpinBox, 'y%dDoubleSpinBox' % i),
                ))

        self.initPreview()

        self.show()

    def initPreview(self):
        self.timeScene = QGraphicsScene()

        self.timeView.setScene(self.timeScene)

        self.timeView.setSceneRect(1.0, -101, 122.0, 102.0)
        self.timeView.scale(2.0, 2.0)

        self.timeLine = LinkedLines(npoints=self.NTIMEPOINTS + 1)

        self.timeScene.addItem(self.timeLine)

        for i, gp in enumerate(self.graphPoints):
            if gp[0] is not None:
                gp[0].valueChanged[float].connect(partial(self.timeLine.setPointX, i))

            if gp[1] is not None:
                gp[1].valueChanged[float].connect(partial(self.timeLine.setPointY, i))


if __name__ == '__main__':

    app = QApplication(sys.argv)

    w = ABKConfig()

    sys.exit(app.exec_())
