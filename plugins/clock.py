#!/usr/bin/env python3

import sys
from PyQt6.QtWidgets import *
from PyQt6.QtGui import *
from PyQt6.QtCore import Qt, QTimer, QTime

class Window(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowFlag(Qt.WindowType.FramelessWindowHint)
        self.setGeometry(0, 0, 100, 30)

        self.button = QPushButton(QTime.currentTime().toString('hh:mm:ss'), self)
        self.button.clicked.connect(lambda:self.close())        

        timer = QTimer(self)
        timer.timeout.connect(self.updateTime)
        timer.start(1000)

    def updateTime(self):
        self.button.setText(QTime.currentTime().toString('hh:mm:ss'))

def main():
    App = QApplication(sys.argv)
    window = Window()
    window.show()
    sys.exit(App.exec())

if __name__ == '__main__':
    main()
