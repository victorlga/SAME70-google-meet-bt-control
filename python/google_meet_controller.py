import pyautogui
import serial
import argparse
import time
import logging
import pyaudio

class MyControllerMap:
    def __init__(self):
        self.button = {'C': 'ctrl', 'E': 'e', 'D': 'd','A':'alt','H':'h', 'W':'w'}

class SerialControllerInterface:
    # Protocolo
    # byte 1 -> Botão 1 (estado - Apertado 1 ou não 0)
    # byte 2 -> EOP - End of Packet -> valor reservado 'X'

    def __init__(self, port, baudrate):
        self.ser = serial.Serial(port, baudrate=baudrate)
        self.mapping = MyControllerMap()
        self.incoming = '0'
        pyautogui.PAUSE = 0  ## remove delay
    
    def handshake(self):
        print("handshake")

        self.incoming = b''
        while self.incoming != b'H':
            print("Entrou no while")
            self.incoming = self.ser.read()
            print(self.incoming)
            logging.debug("Received INCOMING: {}".format(self.incoming))
            print("waiting for H (Hello) from uC")

        self.incoming = b''
        while self.incoming != b'S':
            self.ser.write(b'O')
            print("sent O (OK)")
            self.incoming = self.ser.read()
            logging.debug("Received INCOMING: {}".format(self.incoming))
            print("waiting for S (Start) from uC")


    
    def update(self):
        ## Sync protocol
        print("update")
        while self.incoming != b'Z':
            self.incoming = self.ser.read()
            logging.debug("Received INCOMING: {}".format(self.incoming))
            print("lendo")

        data = self.ser.read()
        logging.debug("Received DATA: {}".format(data))
        print(data)
        
        if data == b'1':
            # Abrir/fechar microfone
            print("datab1")
            logging.info("Ctrl + D")
            pyautogui.keyDown(self.mapping.button['C'])
            pyautogui.press(self.mapping.button['D'])
            pyautogui.keyUp(self.mapping.button['C'])
            self.ser.write(b'1')
        elif data == b'2':
            # Abrir/fechar camera
            print("datab2")
            logging.info("Ctrl + E")
            pyautogui.keyDown(self.mapping.button['C'])
            pyautogui.press(self.mapping.button['E'])
            pyautogui.keyUp(self.mapping.button['C'])
            self.ser.write(b'2')
        elif data == b'3':
            # Levantar/abaixar mão
            print("datab3")
            logging.info("Ctrl + Alt + H")
            pyautogui.keyDown(self.mapping.button['C'])
            pyautogui.keyDown(self.mapping.button['A'])
            pyautogui.press(self.mapping.button['H'])
            pyautogui.keyUp(self.mapping.button['A'])
            pyautogui.keyUp(self.mapping.button['C'])
            self.ser.write(b'3')
        elif data == b'4':
            # Sair da chamada (Control W)
            print("datab4")
            logging.info("Ctrl + W")
            pyautogui.keyDown(self.mapping.button['C'])
            pyautogui.press(self.mapping.button['W'])
            pyautogui.keyUp(self.mapping.button['C'])
            self.ser.write(b'4')
            return 1
        elif data == b'5':
            print("datab4")
            logging.info("Click")
            pyautogui.click()
            self.ser.write(b'5')
        elif data == b'R':
            print("databR")
            logging.info("Mouse right")
            pyautogui.move(45,0)
        elif data == b'U':
            print("databU")
            logging.info("Mouse upper")
            pyautogui.move(0,45)
        elif data == b'D':
            print('databD')
            logging.info("Mouse down")
            pyautogui.move(0,-45)
        elif data == b'L':
            print("databL")
            logging.info("Mouse left")
            pyautogui.move(-45,0)
        else:
            print("Unknown or unexisting command")
        
        # else:
        #     print("Unknown or unexisting command")
        ##pyautogui.move(position_x, position_y)
        self.incoming = self.ser.read()
        return 0


class DummyControllerInterface:
    def __init__(self):
        self.mapping = MyControllerMap()

    def update(self):
        pyautogui.keyDown(self.mapping.button['A'])
        time.sleep(0.1)
        pyautogui.keyUp(self.mapping.button['A'])
        logging.info("[Dummy] Pressed A button")
        time.sleep(1)


if __name__ == '__main__':
    interfaces = ['dummy', 'serial']
    argparse = argparse.ArgumentParser()
    argparse.add_argument('serial_port', type=str)
    argparse.add_argument('-b', '--baudrate', type=int, default=115200)
    argparse.add_argument('-c', '--controller_interface', type=str, default='serial', choices=interfaces)
    argparse.add_argument('-d', '--debug', default=False, action='store_true')
    args = argparse.parse_args()
    if args.debug:
        logging.basicConfig(level=logging.DEBUG)

    print("Connection to {} using {} interface ({})".format(args.serial_port, args.controller_interface, args.baudrate))
    if args.controller_interface == 'dummy':
        controller = DummyControllerInterface()
    else:
        controller = SerialControllerInterface(port=args.serial_port, baudrate=args.baudrate)

    controller.handshake()

    while True:
        status = controller.update()
        if status == 1:
            break
