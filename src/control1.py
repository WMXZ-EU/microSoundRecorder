#!/usr/bin/env python3

# import libraries for serial port and Tkinter GUI
import serial
import Tkinter
 
# Open serial port
ser = serial.Serial('COM3', baudrate=9600, timeout=1)
 
# Create the root window
root = Tkinter.Tk()
root.geometry('400x200+100+100')
root.title('Serial Keystoke Sender')
 
# Create a keystroke handler
def key(event):
    if (event.char == 'q'):
        root.quit()
    elif event.char >= '0' and event.char <= '9':
        print 'keystroke:', repr(event.char)
        ser.write(event.char)
 
# Create a label with instructions
label = Tkinter.Label(root, width=400, height=300, text='Press 0-9 to send a number, or "q" to quit')
label.pack(fill=Tkinter.BOTH, expand=1)
label.bind('<Key>', key)
label.focus_set()
 
# Hand over to the Tkinter event loop
root.mainloop()
 
# Close serial port
ser.close()
