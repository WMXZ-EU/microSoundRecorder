# Andreas Boesch, 04/2013
# Monitor, control and record the status of the Microwave Generator
 
# import packages
import Tkinter as tk    # for the GUI
import ttk              # for nicer GUI widgets
import tkMessageBox     # for GUI testbox
import serial           # for communication with serial port
import time             # for time stuff
import threading        # for parallel computing
 
 
# A thread that continously request the status of the MWG
class myThread (threading.Thread):
    # initialize class
    def __init__(self, name, ser):
        threading.Thread.__init__(self)
        # Name of thread
        self.name = name
        # Serial port information
        self.ser  = ser
        # the request commands
        self.request_dict = {"a": '', "b": '', "c": '', "d": '', "e": '', "f": '', "g": '', "h": '', "k": '', "l": '', "m": '', "n": ''}
 
    # gets called when thread is started with .start()
    def run(self):
        # counter of the while loop
        self.update_count = 0
        while self.ser.isOpen():
            # increase counter ...
            self.update_count += 1
            # ... and set variable for label shown on the GUI
            UpdateC.set(self.update_count)
            # for all request commands, send command
            for request in self.request_dict:
                try:
                    # send command
                    self.ser.write(request)
                    # wait for MWG to answer
                    time.sleep(0.1)
                    # create string for the answer
                    self.request_dict[request] = ''
                    # as long as an answer byte is waiting, read the byte
                    while self.ser.inWaiting() > 0:
                        self.request_dict[request] += self.ser.read(1)
                except:
                    # do nothing if command could not be send
                    pass
 
            # set the label variables with the answeres receiced
            a_var.set(self.request_dict["a"])
            b_var.set(self.request_dict["b"])
            c_var.set(self.request_dict["c"])
            d_var.set(self.request_dict["d"])
            e_var.set(self.request_dict["e"])
            f_var.set(self.request_dict["f"])
            g_var.set(self.request_dict["g"])
            h_var.set(self.request_dict["h"])
            k_var.set(self.request_dict["k"])
            l_var.set(self.request_dict["l"])
            m_var.set(self.request_dict["m"])
            n_var.set(self.request_dict["n"])
 
            time.sleep(1)
 
            # write status into file if requested
            if (rec_stat.get() == "recording") and (self.update_count % 2 == 0):
                timestamp = time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime())
                f.write(timestamp+" "+
                        self.request_dict["a"]+" "+
                        self.request_dict["b"]+" "+
                        self.request_dict["c"]+" "+
                        self.request_dict["d"]+" "+
                        self.request_dict["e"]+" "+
                        self.request_dict["f"]+" "+
                        self.request_dict["g"]+" "+
                        self.request_dict["h"]+" "+
                        self.request_dict["k"]+" "+
                        self.request_dict["l"]+" "+
                        self.request_dict["m"]+" "+
                        self.request_dict["n"]+"\n")
             
 
# an exit procedure
def mQuit():
    # ask yes/no question to confirm exit
    mExit = tkMessageBox.askyesno(title = "Quit", message = "Do you really want to quit?")
    if mExit > 0:
        # close port
        ser.close()
        # detsroy GUI
        root.destroy()
        return
 
# sending commands to the MWG
def mSend(command):
    try:
        ser.write(command)
    except:
        print("Could not send command. Port closed?")
 
    # clear all entry widgets on the GUI
    A_sendvar.set('')
    B_sendvar.set('')
    C_sendvar.set('')
    D_sendvar.set('')
    E_sendvar.set('')
    F_sendvar.set('')
    G_sendvar.set('')
    H_sendvar.set('')
    I_sendvar.set('')
    Z_sendvar.set('')
     
    return
 
def mSave(decision):
    if decision =="A":
        if rec_file.get() == '':
            rec_stat.set("Enter a file name!")
        else:
            rec_stat.set("recording")
            global f
            f = open(rec_file.get(), 'a')
    elif decision == "B":
            rec_stat.set("not recording")
            rec_file.set('')
            try:
                f.close()
            except:
                print("No open file to close")
    return
         
# ===========================
# Begin of the main program
# ===========================
 
# provide information for serial port
ser = serial.Serial()
ser.port = 'COM3'
ser.baudrate = 38400
ser.timeout = 0
# open port if not already open
if ser.isOpen() == False:
    ser.open()
 
# set up root window
root = tk.Tk()
root.geometry("600x500")
root.title("Microwave Generator Control")
 
# variables (explanation see below)
a_var = tk.StringVar()
b_var = tk.StringVar()
c_var = tk.StringVar()
d_var = tk.StringVar()
e_var = tk.StringVar()
f_var = tk.StringVar()
g_var = tk.StringVar()
h_var = tk.StringVar()
k_var = tk.StringVar()
l_var = tk.StringVar()
m_var = tk.StringVar()
n_var = tk.StringVar()
 
A_sendvar = tk.StringVar()
B_sendvar = tk.StringVar()
C_sendvar = tk.StringVar()
D_sendvar = tk.StringVar()
E_sendvar = tk.StringVar()
F_sendvar = tk.StringVar()
G_sendvar = tk.StringVar()
H_sendvar = tk.StringVar()
I_sendvar = tk.StringVar()
Z_sendvar = tk.StringVar()
 
UpdateC   = tk.StringVar()
rec_file  = tk.StringVar()
rec_stat  = tk.StringVar()
rec_stat.set("not recording")
row_start = 4
 
#global f       # save file
 
# RS232 controls
# Requests:
# a: FP value
# b: RP value
# c: FP set point
# d: Maximum RP set point
# e: Ramp time (s)
# f: Overshoot time (x10ms)
# g: Timer activation
# h: Timer duration (s or ms)
# k: start mode
# l: generator state
# m: microwave state
# n: frequency
# r: power supply state
# s: system state
# Commands:
# A: Microwave On
# B: Microwave Off
# C: Maximum RP set point
# D: Maximum FP set point
# E: Ramp duration
# F: Overshoot duration
# G: Start mode
# H: Timer activation
# I: Timer duration (s or 10ms)
# Z: Reset fault
 
# headlines
mHeader    = ttk.Label(root, text = "MICROWAVE GENERATOR CONTROL").grid(row=0, column=1)
 
mStatus    = ttk.Label(root, text = "Serial port open: "+str(ser.isOpen())).grid(row=1, column=1)
mQButton   = ttk.Button(root, text = "close port and quit", command = mQuit).grid(row=1, column=2)
mUpdateC   = ttk.Label(root, textvariable = UpdateC).grid(row=1,column=3)
mSeperate  = ttk.Label(root, text = "===========================").grid(row=2, column=1)
mSubhead_l = ttk.Label(root, text = "STATUS").grid(row=3, column=1)
mSubhead_r = ttk.Label(root, text = "SET").grid(row=3, column=2)
 
# Status table
a_Label = ttk.Label(root, text = "FP value").grid(row=row_start, column=0)
a_Show  = ttk.Label(root, textvariable = a_var).grid(row=row_start, column=1)
     
b_Label = ttk.Label(root, text = "RP value").grid(row=row_start+1, column=0)
b_Show  = ttk.Label(root, textvariable = b_var).grid(row=row_start+1, column=1)
     
c_Label  = ttk.Label(root, text = "FP set point").grid(row=row_start+2, column=0)
c_Show   = ttk.Label(root, textvariable = c_var).grid(row=row_start+2, column=1)
c_Entry  = ttk.Entry(root, textvariable = D_sendvar).grid(row=row_start+2, column=2)
c_Button = ttk.Button(root, text ="send", command = lambda: mSend("D"+D_sendvar.get())).grid(row=row_start+2, column=3)
 
d_Label = ttk.Label(root, text = "Max RP set point").grid(row=row_start+3, column=0)
d_Show  = ttk.Label(root, textvariable = d_var).grid(row=row_start+3, column=1)
d_Entry = ttk.Entry(root, textvariable = C_sendvar).grid(row=row_start+3, column=2)
d_Button = ttk.Button(root, text ="send", command = lambda: mSend("C"+C_sendvar.get())).grid(row=row_start+3, column=3)
 
e_Label = ttk.Label(root, text = "Ramp time (s)").grid(row=row_start+4, column=0)
e_Show  = ttk.Label(root, textvariable = e_var).grid(row=row_start+4, column=1)
e_Entry = ttk.Entry(root, textvariable = E_sendvar).grid(row=row_start+4, column=2)
e_Button = ttk.Button(root, text ="send", command = lambda: mSend("E"+E_sendvar.get())).grid(row=row_start+4, column=3)
 
f_Label = ttk.Label(root, text = "Overshoot time (x10ms)").grid(row=row_start+5, column=0)
f_Show  = ttk.Label(root, textvariable = f_var).grid(row=row_start+5, column=1)
f_Entry = ttk.Entry(root, textvariable = F_sendvar).grid(row=row_start+5, column=2)
f_Button = ttk.Button(root, text ="send", command = lambda: mSend("F"+F_sendvar.get())).grid(row=row_start+5, column=3)
 
g_Label = ttk.Label(root, text = "Timer activation").grid(row=row_start+6, column=0)
g_Show  = ttk.Label(root, textvariable = g_var).grid(row=row_start+6, column=1)
g_Entry = ttk.Entry(root, textvariable = H_sendvar).grid(row=row_start+6, column=2)
g_Button = ttk.Button(root, text ="send", command = lambda: mSend("H"+H_sendvar.get())).grid(row=row_start+6, column=3)
 
h_Label = ttk.Label(root, text = "Timer duration (s or ms)").grid(row=row_start+7, column=0)
h_Show  = ttk.Label(root, textvariable = h_var).grid(row=row_start+7, column=1)
h_Entry = ttk.Entry(root, textvariable = I_sendvar).grid(row=row_start+7, column=2)
h_Button = ttk.Button(root, text ="send", command = lambda: mSend("I"+I_sendvar.get())).grid(row=row_start+7, column=3)
 
k_Label = ttk.Label(root, text = "start mode").grid(row=row_start+8, column=0)
k_Show  = ttk.Label(root, textvariable = k_var).grid(row=row_start+8, column=1)
k_Entry = ttk.Entry(root, textvariable = G_sendvar).grid(row=row_start+8, column=2)
k_Button = ttk.Button(root, text ="send", command = lambda: mSend("G"+G_sendvar.get())).grid(row=row_start+8, column=3)
 
l_Label = ttk.Label(root, text = "generator state").grid(row=row_start+9, column=0)
l_Show  = ttk.Label(root, textvariable = l_var).grid(row=row_start+9, column=1)
     
m_Label = ttk.Label(root, text = "microwave state").grid(row=row_start+10, column=0)
m_Show  = ttk.Label(root, textvariable = m_var).grid(row=row_start+10, column=1)
m_ButtonA = ttk.Button(root, text ="power ON", command = lambda: mSend("A")).grid(row=row_start+10, column=2)
m_ButtonB = ttk.Button(root, text ="power OFF", command = lambda: mSend("B")).grid(row=row_start+10, column=3)
 
n_Label = ttk.Label(root, text = "Frequency").grid(row=row_start+11, column=0)
n_Show  = ttk.Label(root, textvariable = n_var).grid(row=row_start+11, column=1)
 
rec_Label = ttk.Label(root, text = "Save values: File name").grid(row=row_start+12, column=0)
rec_Entry = ttk.Entry(root, textvariable = rec_file).grid(row=row_start+12, column=1)
rec_ButtonA = ttk.Button(root, text = "Save on", command = lambda: mSave("A")).grid(row=row_start+12, column=2)
rec_ButtonB = ttk.Button(root, text = "Save off", command = lambda: mSave("B")).grid(row=row_start+12, column=3)
rec_LabelS  = ttk.Label(root, textvariable = rec_stat).grid(row=row_start+13, column=1)
rec_LabelF  = ttk.Label(root, textvariable = rec_file).grid(row=row_start+13, column=2)
 
# wait
time.sleep(1)
# call and start update-thread
thread1 = myThread("Updating", ser)
thread1.start()
 
# start GUI
root.mainloop()
 
