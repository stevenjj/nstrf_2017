#!/usr/bin/env python

# Val simple GUI
# Based on Andrea Thomaz's lab's class code in UT Austin
# Modified for personal use. 
#     stevenjj@utexas.edu

#from GUI_params import *

# GUI Command list
GUI_CMD_TOPIC = 'gui_object_registration_manager/operator_command'
INVALID_CMD = "invalid_cmd"

GET_CLOUD_IN_BOX, GET_CLOUD_IN_BOX_GUI_STRING  = "get_cloud_in_box", "Get Boxed Cloud"
STORE_CACHED_CLOUD, STORE_CACHED_CLOUD_GUI_STRING   = "store_cached_cloud", "Store Cached Cloud"
INPUT_FILENAME, INPUT_FILENAME_GUI_STRING   = "input_filename", "Set Filename"


# ----- Start ------
import signal
import sys


import rospy
#import rospkg
import yaml
from std_msgs.msg import String
from std_msgs.msg import Int8
from PyQt4.QtCore import QTimer
from PyQt4 import QtGui, QtCore
from PyQt4.QtGui import QInputDialog

class ValGui(QtGui.QWidget):

  def __init__(self):
      QtGui.QWidget.__init__(self)
      self.setWindowFlags(QtCore.Qt.WindowStaysOnTopHint)
      newFont = QtGui.QFont("Helvetica", 14, QtGui.QFont.Bold)

      # Add a main layout
      mainLayout = QtGui.QVBoxLayout(self)
      #mainLayout->setMeubBar(menuBar)
      # Add buttons with the commands
      grid = QtGui.QGridLayout()
      grid.setSpacing(3)

      # Initialize rosnode
      rospy.init_node("simple_object_reg_gui") 

      #rospack = rospkg.RosPack()
      default_pub_topic = GUI_CMD_TOPIC

      # Set Commands
      self.commands = [GET_CLOUD_IN_BOX_GUI_STRING,
                       STORE_CACHED_CLOUD_GUI_STRING
                       ] 
      
      positions = [(i,j) for i in range(len(self.commands)) for j in range(1)]
           
      for position, name in zip(positions, self.commands):
          button = QtGui.QPushButton(name)
          button.setObjectName('%s' % name)
          button.setFont(newFont)
          button.setStyleSheet("background-color: #00ffb2")
          button.clicked.connect(self.handleButton)
          grid.addWidget(button, *position)

      mainLayout.addLayout(grid)
      mainLayout.addStretch()
      
      # Show the GUI 
      self.adjustSize()
      self.setWindowTitle("GUI Object Registration")
      self.move(200,100)
      self.show()
      self.raise_()

      # # Create the publisher to publish the commands to
      self.pub = rospy.Publisher(default_pub_topic, String, queue_size=1)

      rospy.loginfo("Finished initializing GUI Object Registration")

  def gettext(self):
    text, ok = QInputDialog.getText(self, 'Set a text', 'Enter a filename for saving this pointcloud data:')

    if ok:
      print "    Your filename input:", text
      new_filename = str(text)
      if (len(new_filename) == 0):
        print("    You entered an empty filename. using default name as: boxed_cloud.pcd")
        new_filename = "boxed_cloud.pcd"

      if not(".pcd" in new_filename):
        print "    Your filename did not contain .pcd, the program will manually add it"
        new_filename = str(text) + ".pcd"



      old_name = rospy.get_param('/object_registration_interest_box/filename', "no_param_found")
      print "    Filename was previously set to:", old_name
      rospy.set_param('/object_registration_interest_box/filename', new_filename)
      new_name = rospy.get_param('/object_registration_interest_box/filename', "no_param_found")
      print "    Filename is now set to:", new_name


  # Button handler after its clicked
  def handleButton(self):
      clicked_button = self.sender()

      string_cmd = INVALID_CMD
      send_command = False
      # # Publish everytime a command is selected from the combo box
      command = str(clicked_button.objectName())

      if command in self.commands:
        send_command = True

      if command == GET_CLOUD_IN_BOX_GUI_STRING: 
        string_cmd = GET_CLOUD_IN_BOX
      elif command == STORE_CACHED_CLOUD_GUI_STRING: 
        self.gettext()
        string_cmd = STORE_CACHED_CLOUD
      elif command == INPUT_FILENAME_GUI_STRING:
        #string_cmd = INPUT_FILENAME
        send_command = False
        self.gettext()
      else:
        string_cmd = INVALID_CMD
      
      rospy.loginfo(command)

      if send_command:
        msg = String()
        msg.data = string_cmd
        self.pub.publish(msg)

# def gui_start():
#     app = QtGui.QApplication(sys.argv)
#     sg = ValGui()
#     sys.exit(app.exec_())


def sigint_handler(*args):
    """Handler for the SIGINT signal."""
    sys.stderr.write('\r')
    QtGui.QApplication.quit()

if __name__ == "__main__":
  #gui_start
  signal.signal(signal.SIGINT, sigint_handler)

  app = QtGui.QApplication(sys.argv)
  timer = QTimer()
  timer.start(100)  # You may change this if you wish.
  timer.timeout.connect(lambda: None)  # Let the interpreter run each 100 ms.

  sg = ValGui()
  sys.exit(app.exec_())

