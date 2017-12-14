#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#
# Copyright © 2017 Benoit Rapidel <benoit.rapidel+devs@exmachina.fr>
#
# Distributed under terms of the MIT license.

"""

"""

from PyQt5.QtWidgets import QGraphicsView, QGraphicsScene, QGraphicsObject
from PyQt5.QtWidgets import QGraphicsLineItem, QGraphicsEllipseItem
from PyQt5.QtWidgets import QDialog, QFormLayout, QDialogButtonBox
from PyQt5.QtWidgets import QLabel, QComboBox, QDoubleSpinBox
from PyQt5.QtCore import pyqtSlot, pyqtSignal, QTimer
from PyQt5.QtGui import QPen, QColor, QFont
from PyQt5.QtCore import Qt, QObject, QPointF, QLineF, QRectF

from math import sqrt


class OptionDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)

        self.parent = parent

        if self.parent:
            opts = self.parent.getCurrentOptions()

        layout = QFormLayout(self)

        self.baudrateCombo = QComboBox(self)
        l = QLabel('Baudrate', self)

        for b in self.BAUDRATES:
            self.baudrateCombo.addItem(str(b))

        if opts.get('serial_baud', None) in self.BAUDRATES:
            self.baudrateCombo.setCurrentIndex(self.BAUDRATES.index(opts['serial_baud']))

        layout.addRow(l, self.baudrateCombo)

        self.maximumSpeed = QDoubleSpinBox(self)
        self.maximumSpeed.setValue(float(opts.get('maximum_speed', 0)))
        l = QLabel('Maximum Speed', self)
        self.maximumSpeed.setEnabled(False)
        layout.addRow(l, self.maximumSpeed)

        self.speedFactor = QDoubleSpinBox(self)
        self.speedFactor.setValue(float(opts.get('speed_factor', 0)))
        l = QLabel('Speed Factor', self)
        self.speedFactor.setEnabled(False)
        layout.addRow(l, self.speedFactor)

        # OK and Cancel buttons
        self.buttons = QDialogButtonBox(
            QDialogButtonBox.Ok | QDialogButtonBox.Cancel,
            Qt.Horizontal, self)
        layout.addWidget(self.buttons)

        self.buttons.accepted.connect(self.accept)
        self.buttons.rejected.connect(self.reject)

    # get options from dialog
    def options(self):
        return {
                'serial_baud': self.baudrateCombo.currentText(),
                'maximum_speed': float(self.maximumSpeed.value()),
                'speed_factor': float(self.speedFactor.value()),
                }

    # static method to create the dialog and return options dict
    @staticmethod
    def getOptions(parent=None):
        dialog = OptionDialog(parent)
        result = dialog.exec_()
        opts = dialog.options()
        return (opts, result == QDialog.Accepted)


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
    lineChanged = pyqtSignal()

    def __init__(self, *args, **kwargs):
        self.npoints = kwargs.pop('npoints', 2)
        self.xFactor = kwargs.pop('xfactor', 1)
        self.yFactor = kwargs.pop('yfactor', 1)

        super().__init__(*args, **kwargs)

        self._lines = []
        self._linesF = []
        self._points = []
        self._pointsF = []

        self._circleDia = 1
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

        for i in range(1, self.npoints):
            e = QGraphicsCircleItem(diameter=self._circleDia)
            e.setPen(self._circlePen)
            self._points.append(e)

        for i in range(self.npoints):
            self.setPointX(i, 0)
            self.setPointY(i, 0)

        self.setPointX(0, 0)
        self.setPointY(0, 0)

        self.setPointX(self.npoints - 1, 10)
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

        if p > 0 and p < self.npoints:
            self._points[p- 1].setCenter(point)

        self.updateAdjacentPointsX(p, x)
        self.lineChanged.emit()

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

        self.lineChanged.emit()

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


class Fabric(QGraphicsObject):
    sizeChanged = pyqtSignal(float, float)
    pickupPointChanged = pyqtSignal(float, float)

    def __init__(self, *args, **kwargs):
        self.width = kwargs.pop('width', 1)
        self.height = kwargs.pop('height', 1)
        self.pickup_point = kwargs.pop('pickup_point', False)
        self.draw_corners = kwargs.pop('draw_corners', False)
        self.scale = kwargs.pop('scale', 1)

        super().__init__(*args, **kwargs)

        self._lines = []
        self._linesF = []
        self._points = []
        self._pointsF = []

        self.pickupPointX = 50
        self.pickupPointY = 50

        self._circleDia = 2.0
        self._linePen = QPen(QColor(135, 208, 80))
        self._linePen.setCapStyle(Qt.RoundCap)
        self._circlePen = QPen(QColor(66, 66, 66))
        self._pickupPen = QPen(QColor(135, 66, 66))

        self._linePen.setCapStyle(Qt.RoundCap)
        self._circlePen.setCapStyle(Qt.RoundCap)
        self._pickupPen.setCapStyle(Qt.RoundCap)

        self._linePen.setWidth(1)
        self._circlePen.setWidth(2)
        self._pickupPen.setWidth(2)

        for i in range(4):
            l = QGraphicsLineItem()
            l.setPen(self._linePen)
            self._lines.append(l)
            self._linesF.append(QLineF())

        for i in range(4):
            self._pointsF.append(QPointF(0.0, 0.0))

        if self.draw_corners:
            for i in range(4):
                e = QGraphicsCircleItem(diameter=self._circleDia)
                e.setPen(self._circlePen)
                self._points.append(e)

        if self.pickup_point:
            self._pickupPointF = QPointF()
            self._pickupPoint = QGraphicsCircleItem(diameter=self._circleDia)
            self._pickupPoint.setPen(self._pickupPen)

    def setWidth(self, w):
        self.prepareGeometryChange()
        self.width = float(w)

        self._pointsF[2].setX(self.width * self.scale)
        self._pointsF[3].setX(self.width * self.scale)

        self.updateLines()

        self.sizeChanged.emit(self.width, self.height)

    def setHeight(self, h):
        self.prepareGeometryChange()
        self.height = float(h)

        self._pointsF[1].setY(self.height * self.scale)
        self._pointsF[2].setY(self.height * self.scale)

        self.updateLines()

        self.sizeChanged.emit(self.width, self.height)

    def updateLines(self):
        for i, lf in enumerate(self._linesF):
            lf.setP1(self._pointsF[i])
            if i < 3:
                lf.setP2(self._pointsF[i + 1])
            else:
                lf.setP2(self._pointsF[0])

        if self.draw_corners:
            for i, p in enumerate(self._pointsF):
                self._points[i].setCenter(p)

        for i, l in enumerate(self._linesF):
            self._lines[i].setLine(l)

        if self.pickup_point:
            self._pickupPointF.setX((self.pickupPointX / 100.0) * self.width * self.scale)
            self._pickupPointF.setY((self.pickupPointY / 100.0) * self.height * self.scale)
            self._pickupPoint.setCenter(self._pickupPointF)

    def setPickupPointX(self, x):   # x is in percent
        self.prepareGeometryChange()
        self.pickupPointX = max(0, min(100, x))
        self.updateLines()

    def setPickupPointY(self, y):   # y is in percent
        self.prepareGeometryChange()
        self.pickupPointY = max(0, min(100, y))
        self.updateLines()

    def paint(self, *args, **kwargs):
        for l in self._lines:
            l.paint(*args, **kwargs)

        if self.draw_corners:
            for p in self._points:
                p.paint(*args, **kwargs)

        if self.width != 0 and self.height != 0:
            self._pickupPoint.paint(*args, **kwargs)

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


class QTimeScene(QGraphicsScene):
    LEFTMARGIN = 30
    BOTTOMMARGIN = 30

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.parentView = args[0]

        self.parentView.setScene(self)

        self.timeLine = LinkedLines(npoints=self.NTIMEPOINTS, yfactor=1, xfactor=0.05)

        self._axesPen = QPen(QColor(132, 132, 132))
        self.xaxis = self.addLine(-2, 0, 200, 0) # X axis
        self.yaxis = self.addLine(0, 2, 0, -100) # Y axis
        self.xticks = [self.addLine(-2, 0, 0, 0) for x in range(self.NTIMEPOINTS)]
        self.yticks = [self.addLine(-2, -100, 0, -100) for x in range(self.NTIMEPOINTS)]
        self.xmarks = [self.addSimpleText('') for x in range(self.NTIMEPOINTS)]
        self.ymarks = [self.addSimpleText('') for x in range(self.NTIMEPOINTS)]
        self.addItem(self.timeLine)

        mark_font = QFont()
        mark_font.setPointSize(4)
        self.xaxis.setPen(self._axesPen)
        self.yaxis.setPen(self._axesPen)
        for t in self.xticks:
            t.setPen(self._axesPen)
        for t in self.yticks:
            t.setPen(self._axesPen)

        for m in self.xmarks:
            m.setRotation(90)
            m.setFont(mark_font)
        for m in self.ymarks:
            m.setFont(mark_font)

        self.timeLine.lineChanged.connect(self.updateView)
        self.timeLine.lineChanged.connect(self.updateAxis)

        self.updateView()

    def updateAxis(self, *args):
        points = self.timeLine._pointsF[1:]
        for i, p in enumerate(points):
            self.xticks[i].setLine(p.x(), 2, p.x(), 0)
            self.yticks[i].setLine(-2, p.y(), 0, p.y())

            xm = self.xmarks[i]
            ym = self.ymarks[i]
            xm.setText('{:d} ms'.format(int(p.x() / self.timeLine.xFactor)))
            xm.setPos(p.x()+3, 3)
            if i > 0 and xm.sceneBoundingRect().intersects(self.xmarks[i-1].sceneBoundingRect()):
                xm.hide()
            else:
                xm.show()

            ym.setText('{:d}%'.format(int(-p.y() / self.timeLine.yFactor)))
            ym.setPos(-4-self.ymarks[i].boundingRect().width(), p.y()-3)
            if i < len(points) and ym.sceneBoundingRect().intersects(self.ymarks[i+1].sceneBoundingRect()):
                ym.hide()
            else:
                ym.show()

        self.xaxis.setLine(-2, 0, self.sceneRect().getCoords()[2], 0)

    def updateView(self, *args):
        r = self.timeLine.boundingRect()
        r.setHeight(100 + self.BOTTOMMARGIN)
        r.setWidth(r.width() + self.LEFTMARGIN)
        r.moveBottom(self.BOTTOMMARGIN)
        r.moveLeft(-self.LEFTMARGIN)
        self.parentView.fitInView(r, Qt.KeepAspectRatio)


class QFabricScene(QGraphicsScene):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.parentView = args[0]

        self.parentView.setScene(self)
        self.parentView.scale(2.0, 2.0)

        self.fabricPreview = Fabric(width=0, height=0, pickup_point=True, scale=10)
        # self.fabricSizeText = QText(

        self.addItem(self.fabricPreview)

        self.fabricPreview.sizeChanged.connect(self.updateView)

    def updateView(self, *args):
        self.parentView.fitInView(self.fabricPreview.boundingRect(), Qt.KeepAspectRatio)



class ABKFabric(QObject):
    surfaceChanged = pyqtSignal(float)
    weightChanged = pyqtSignal(float)
    lengthChanged = pyqtSignal(float)
    accelerationChanged = pyqtSignal(float)
    decelerationChanged = pyqtSignal(float)
    profileChanged = pyqtSignal(object)

    def __init__(self, *args, **kwargs):
        self._width = kwargs.pop('width', 0)
        self._height = kwargs.pop('height', 0)
        self._weight = kwargs.pop('grammage', 0)
        self._cable_length = kwargs.pop('cable_length', 0)
        self._pickup_point_x = kwargs.pop('pickup_point_x', 0.5)
        self._pickup_point_y = kwargs.pop('pickup_point_y', 0.5)
        self._nominal_speed = kwargs.pop('nominal_speed', 0)
        self._maximum_speed = kwargs.pop('maximum_speed', 0)
        self._speed_factor = kwargs.pop('speed_factor', 3.5)
        self._accel_time = kwargs.pop('accel_time', 1000)
        self._decel_time = kwargs.pop('decel_time', 1000)
        self._start_delay = kwargs.pop('start_delay', 0)

        super().__init__(*args, **kwargs)

    @property
    def width(self):
        return self._width

    @property
    def height(self):
        return self._height

    @property
    def grammage(self):
        return self._grammage

    @property
    def weight(self):
        return float(self.grammage * self.surface / 1000)

    @property
    def cable_length(self):
        return self._cable_length

    @property
    def pickup_point_x(self):
        return self._pickup_point_x

    @property
    def pickup_point_y(self):
        return self._pickup_point_y

    @property
    def surface(self):
        return float(self.width * self.height)

    @property
    def length(self):
        return max(self.pickup_distances)

    @property
    def points(self):
        p = list((
            (0, 0),
            (0, self.height),
            (self.width, self.height),
            (self.width, 0)
            ))
        return p

    @property
    def pickup_distances(self):
        """
        Compute distances between each corner and the pickup point.
        """
        ds = list()
        ppx = self.width * self.pickup_point_x
        ppy = self.height * self.pickup_point_y
        for p in self.points:
            d = sqrt(
                    abs(p[0] - ppx) ** 2 +
                    abs(p[1] - ppy) ** 2)
            ds.append(d)

        return ds

    @property
    def nominal_speed(self):
        return self._nominal_speed

    @property
    def maximum_speed(self):
        return self._maximum_speed

    @property
    def speed_factor(self):
        return self._speed_factor

    @property
    def accel_time(self):
        return self._accel_time

    @property
    def decel_time(self):
        return self._decel_time

    @property
    def acceleration(self):
        return self.nominal_speed / (self.accel_time / 1000)

    @property
    def deceleration(self):
        return self.nominal_speed / (self.decel_time / 1000)

    @property
    def start_delay(self):
        return self._start_delay

    @property
    def time_profile(self):
        profile = dict()

        profile['start'] = self.start_delay
        profile['p1.time'] = profile['start'] + self.accel_time

        accel_dist = (self.nominal_speed / 2) * (self.accel_time / 1000)
        decel_dist = (self.nominal_speed / 2) * (self.decel_time / 1000)
        if self.nominal_speed > 0.0:
            cable_time = (self.cable_length - accel_dist) / self.nominal_speed
            fabric_time = (self.length - decel_dist) / self.nominal_speed
        else:
            cable_time = (self.cable_length - accel_dist)
            fabric_time = self.length - decel_dist

        profile['p2.time'] = profile['p1.time'] + cable_time * 1000
        profile['p3.time'] = profile['p2.time'] + fabric_time * 1000 - self.decel_time
        profile['stop'] = profile['p3.time'] + self.decel_time

        if self.maximum_speed > 0.0:
            profile['p1.speed'] = (self.nominal_speed / self.maximum_speed) * 100
            profile['p2.speed'] = (self.nominal_speed / self.maximum_speed) * 100
            profile['p3.speed'] = (self.nominal_speed / self.maximum_speed) * 100 / self.speed_factor
        else:
            profile['p1.speed'] = self.nominal_speed
            profile['p2.speed'] = self.nominal_speed
            profile['p3.speed'] = self.nominal_speed / self.speed_factor

        return profile

    # property setters

    @width.setter
    def width(self, w):
        self._width = max(0, float(w))
        self.surfaceChanged.emit(self.surface)
        self.lengthChanged.emit(self.length)
        self.profileChanged.emit(self.time_profile)

    @height.setter
    def height(self, h):
        self._height = max(0, float(h))
        self.surfaceChanged.emit(self.surface)
        self.lengthChanged.emit(self.length)
        self.profileChanged.emit(self.time_profile)

    @grammage.setter
    def grammage(self, gr):
        self._grammage = max(0, float(gr))
        self.weightChanged.emit(self.weight)
        self.profileChanged.emit(self.time_profile)

    @cable_length.setter
    def cable_length(self, cb):
        self._cable_length = max(0, float(cb))
        self.profileChanged.emit(self.time_profile)

    @pickup_point_x.setter
    def pickup_point_x(self, x):
        self._pickup_point_x = (max(0, min(100, float(x))) / 100.0)
        self.lengthChanged.emit(self.length)
        self.profileChanged.emit(self.time_profile)

    @pickup_point_y.setter
    def pickup_point_y(self, y):
        self._pickup_point_y = (max(0, min(100, float(y))) / 100.0)
        self.lengthChanged.emit(self.length)
        self.profileChanged.emit(self.time_profile)

    @nominal_speed.setter
    def nominal_speed(self, speed):
        if self.maximum_speed > 0:
            self._nominal_speed = min(self.maximum_speed, float(speed))
        else:
            self._nominal_speed = float(speed)
        self.accelerationChanged.emit(self.acceleration)
        self.decelerationChanged.emit(self.deceleration)
        self.profileChanged.emit(self.time_profile)

    @maximum_speed.setter
    def maximum_speed(self, speed):
        self._maximum_speed = float(speed)
        self.profileChanged.emit(self.time_profile)

    @speed_factor.setter
    def speed_factor(self, sf):
        self._speed_factor = max(1, sf)
        self.profileChanged.emit(self.time_profile)

    @accel_time.setter
    def accel_time(self, accel_time):
        self._accel_time = float(accel_time)
        self.accelerationChanged.emit(self.acceleration)
        self.profileChanged.emit(self.time_profile)

    @decel_time.setter
    def decel_time(self, decel_time):
        self._decel_time = float(decel_time)
        self.decelerationChanged.emit(self.deceleration)
        self.profileChanged.emit(self.time_profile)

    @start_delay.setter
    def start_delay(self, st):
        self._start_delay = float(st)
        self.profileChanged.emit(self.time_profile)

    # pyqt slots

    @pyqtSlot(float)
    def setWidth(self, w):
        self.width = w

    @pyqtSlot(float)
    def setHeight(self, h):
        self.height = h

    @pyqtSlot(float)
    def setGrammage(self, gr):
        self.grammage = gr

    @pyqtSlot(float)
    def setCableLength(self, cb):
        self.cable_length = cb

    @pyqtSlot(int)
    def setPickupPointX(self, x):
        self.pickup_point_x = x

    @pyqtSlot(int)
    def setPickupPointY(self, y):
        self.pickup_point_y = y

    @pyqtSlot(float)
    def setNominalSpeed(self, s):
        self.nominal_speed = s

    @pyqtSlot(float)
    def setSpeedFactor(self, sf):
        self.speed_factor = sf

    @pyqtSlot(int)
    def setAccelTime(self, at):
        self.accel_time = at

    @pyqtSlot(int)
    def setDecelTime(self, dt):
        self.decel_time = dt

    @pyqtSlot(int)
    def setStartDelay(self, st):
        self.start_delay = st
