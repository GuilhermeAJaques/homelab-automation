import gpiod
from gpiod.line import Direction, Value

class GPIO_driver:
    # Just declare to match with other drivers
    def __init__(self, chip):
        try:
            self.chip = chip
        except Exception as e:
            print("Error connectiong to gpiochip0: {}".format(e)) 

    def read_variable(self, gpio_number):
        try:
            with gpiod.request_lines(self.chip, 
                                     consumer="reader",
                                     config={gpio_number: gpiod.LineSettings(direction=Direction.INPUT)}) as request:
                value = request.get_value(gpio_number)
                return value
        except Exception as e:
            print("Error to read {}: {}".format(gpio_number, e)) 
            return None

    def write_variable(self, gpio_number, value):
        try:
            with gpiod.request_lines(self.chip,
                                     consumer="writer", 
                                     config={gpio_number: gpiod.LineSettings(direction=Direction.OUTPUT)}) as request:
                if (bool(value)):
                    request.set_value(gpio_number, Value.ACTIVE)
                else:
                    request.set_value(gpio_number, Value.INACTIVE)

        except Exception as e:
            print("Error to write {}: {}".format(gpio_number, e)) 

    # Just declare to match with other drivers, otherwise will throw error
    def connect(self):
        print("GPIO driver connected")

    # Just declare to match with other drivers, otherwise will throw error
    def disconnect(self):
        print("GPIO driver disconnected")