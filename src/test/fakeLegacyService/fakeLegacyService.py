# Copyright (C) 2013 - 2015 BMW Group
# Author: Manfred Bathelt (manfred.bathelt@bmw.de)
# Author: Juergen Gehring (juergen.gehring@bmw.de)
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import sys
import traceback
import gobject
import math
import dbus
import dbus.service
import dbus.mainloop.glib

loop = gobject.MainLoop()
dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

class FakeLegacyService(dbus.service.Object):
  def __init__(self, loop):
    busName = dbus.service.BusName('fake.legacy.service.LegacyInterface_fake.legacy.service', bus = dbus.SessionBus())
    dbus.service.Object.__init__(self, busName, '/fake/legacy/service')
    #self.properties = {'RestartReason': 1, 'ShutdownReason': 2, 'WakeUpReason' :3, 'BootMode' :4}
    self.ABus=""
    self.APath=""
    self.loop=loop

  @dbus.service.method(dbus_interface='fake.legacy.service.Introspectable', out_signature = 's')
  def Introspect(self):
	f = open('fake.legacy.service.xml', "r")
	text = f.read()
	return text
 
  @dbus.service.method(dbus_interface='fake.legacy.service.LegacyInterface', in_signature = 'i', out_signature = 'ii')
  def TestMethod(self, input):
    val1=input - 5
    val2=input + 5
    return val1, val2

  @dbus.service.method(dbus_interface='fake.legacy.service.LegacyInterface', out_signature = 'si')
  def OtherTestMethod(self):
    greeting='Hello'
    identifier=42
    return greeting, identifier

  @dbus.service.method(dbus_interface='fake.legacy.service.LegacyInterface')
  def finish(self): 
	self.loop.quit()
	return 0

nsm = FakeLegacyService(loop)
loop.run()
