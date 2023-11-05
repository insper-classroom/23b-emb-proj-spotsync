import pyautogui
import serial
import argparse
import time
import logging

class MyControllerMap:
    def __init__(self):
        self.button = {'A': 'left', 'B': 'SPACE', 'C': 'right', 'D': 'up', 'E': 'down', 'F': 's'} 

class SerialControllerInterface:
    # Protocolo
    # byte 1 -> Botão 1 (estado - Apertado 1 ou não 0)
    # byte 2 -> EOP - End of Packet -> valor reservado 'X'

    def __init__(self, port, baudrate):
        self.ser = serial.Serial(port, baudrate=baudrate)
        self.mapping = MyControllerMap()
        self.incoming = '0'
        pyautogui.PAUSE = 0  ## remove delay
    
    def update(self):
        ## Sync protocol
        while self.incoming != b'X':
            self.incoming = self.ser.read()
            logging.debug("Received INCOMING: {}".format(self.incoming))

        data = self.ser.read()
        print(data)
        logging.debug("Received DATA: {}".format(data))

        if data == b'1':
            print("Musica anterior")
            logging.info("KEYDOWN A")
            pyautogui.hotkey('ctrl', self.mapping.button['A'])
        if data == b'2':
            print("Play/Pause")
            logging.info("KEYDOWN B")
            pyautogui.keyDown(self.mapping.button['B'])
        if data == b'3':
            print("Musica Seguinte")
            logging.info("KEYDOWN C")
            pyautogui.hotkey('ctrl', self.mapping.button['C'])
        if data == b'4':
            print("Shuffle playlist")
            logging.info("KEYDOWN F")
            pyautogui.hotkey('ctrl', self.mapping.button['F'])
        if data == b'5':
            print("Volume +")
            pyautogui.hotkey('ctrl', self.mapping.button['D'])
        if data == b'6':
            print("Volume -")
            pyautogui.hotkey('ctrl', self.mapping.button['E'])
        if data == b'0':
            logging.info("KEYUP A")
            pyautogui.keyUp(self.mapping.button['A'])
            logging.info("KEYUP B")
            pyautogui.keyUp(self.mapping.button['B'])
            logging.info("KEYUP C")
            pyautogui.keyUp(self.mapping.button['C'])
            logging.info("KEYUP D")
            pyautogui.keyUp(self.mapping.button['D'])
            logging.info("KEYUP E")
            pyautogui.keyUp(self.mapping.button['E'])
            logging.info("KEYUP F")
            pyautogui.keyUp(self.mapping.button['F'])

        self.incoming = self.ser.read()


class DummyControllerInterface:
    def __init__(self):
        self.mapping = MyControllerMap()

    def update(self):
        time.sleep(1)
        # pyautogui.keyDown(self.mapping.button['A'])
        pyautogui.hotkey('ctrl', self.mapping.button['A'])
        time.sleep(1)
        pyautogui.keyUp(self.mapping.button['A'])
        time.sleep(1)
        pyautogui.keyDown(self.mapping.button['B'])
        time.sleep(1)
        pyautogui.keyUp(self.mapping.button['B'])
        time.sleep(1)
        pyautogui.hotkey('ctrl', self.mapping.button['C'])
        time.sleep(1)
        pyautogui.keyUp(self.mapping.button['C'])
        time.sleep(1)
        pyautogui.hotkey('ctrl', self.mapping.button['D'])
        time.sleep(1)
        pyautogui.keyUp(self.mapping.button['D'])
        time.sleep(1)
        pyautogui.hotkey('ctrl', self.mapping.button['E'])
        time.sleep(1)
        pyautogui.keyUp(self.mapping.button['E'])
        time.sleep(1)
        pyautogui.hotkey('ctrl', self.mapping.button['F'])
        time.sleep(1)
        pyautogui.keyUp(self.mapping.button['F'])
        time.sleep(1)


if __name__ == '__main__':
    interfaces = ['dummy', 'serial']
    argparse = argparse.ArgumentParser()
    # argparse.add_argument('serial_port', type=str)
    argparse.add_argument('-b', '--baudrate', type=int, default=115200)
    argparse.add_argument('-c', '--controller_interface', type=str, default='serial', choices=interfaces)
    argparse.add_argument('-d', '--debug', default=False, action='store_true')
    args = argparse.parse_args()
    if args.debug:
        logging.basicConfig(level=logging.DEBUG)

    print("Connection to {} using {} interface ({})".format("COM4", args.controller_interface, args.baudrate))
    if args.controller_interface == 'dummy':
        controller = DummyControllerInterface()
    else:
        controller = SerialControllerInterface(port="COM4", baudrate=args.baudrate)

    while True:
        controller.update()
