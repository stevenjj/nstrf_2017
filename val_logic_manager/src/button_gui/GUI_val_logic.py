#!/usr/bin/env python

# Dreamer simple GUI
from GUI_params import *

import rospy
import sys
#import rospkg
import yaml
from std_msgs.msg import String
from std_msgs.msg import Int8
from PyQt4 import QtGui, QtCore

class ValGui(QtGui.QWidget):

  def __init__(self):
      QtGui.QWidget.__init__(self)
      newFont = QtGui.QFont("Helvetica", 18, QtGui.QFont.Bold)

      # Add a main layout
      mainLayout = QtGui.QVBoxLayout(self)
      #mainLayout->setMeubBar(menuBar)
      # Add buttons with the commands
      grid = QtGui.QGridLayout()
      grid.setSpacing(20)

      # Initialize rosnode
      rospy.init_node("simple_val_gui") 

      #rospack = rospkg.RosPack()
      default_pub_topic = GUI_CMD_TOPIC

      # Set Commands
      self.commands = [GO_NEUTRAL_POS_GUI_STRING,
                       SEND_SINGLE_IK_GUI_STRING, 
                       ] 
      
      positions = [(i,j) for i in range(len(self.commands)) for j in range(3)]
           
      for position, name in zip(positions, self.commands):
          button = QtGui.QPushButton(name)
          button.setObjectName('%s' % name)
          button.setFont(newFont)
          button.setStyleSheet("background-color: #FFA500")
          button.clicked.connect(self.handleButton)
          grid.addWidget(button, *position)

      mainLayout.addLayout(grid)
      mainLayout.addStretch()
      
      # Show the GUI 
      self.adjustSize()
      self.setWindowTitle("GUI Val Logic")
      self.move(400,400)
      self.show()
      self.raise_()

      # # Create the publisher to publish the commands to
      self.pub = rospy.Publisher(default_pub_topic, String, queue_size=1)

      rospy.loginfo("Finished initializing GUI Val Logic")

  # Button handler after its clicked
  def handleButton(self):
      clicked_button = self.sender()

      string_cmd = INVALID_CMD
      send_command = False
      # # Publish everytime a command is selected from the combo box
      command = str(clicked_button.objectName())

      if command in self.commands:
        send_command = True


      if command == GO_NEUTRAL_POS_GUI_STRING: 
        string_cmd = GO_NEUTRAL_POS

      elif command == SEND_SINGLE_IK_GUI_STRING: 
        string_cmd = SEND_SINGLE_IK 
     
      else:
        string_cmd = INVALID_CMD
      
      rospy.loginfo(command)

      if send_command:
        msg = String()
        msg.data = string_cmd
        self.pub.publish(msg)

def gui_start():
    app = QtGui.QApplication(sys.argv)
    sg = ValGui()
    sys.exit(app.exec_())


gui_start()